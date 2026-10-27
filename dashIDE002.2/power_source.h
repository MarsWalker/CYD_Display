/**
 * @file power_source.h
 *
 * Home Assistant power-source adapter and owner of one bounded history buffer.
 * Configuration, history and latest value share the engine mutex with normal
 * sources and drawing. ParsePowerSources replaces the allocation on a board
 * reload; changing only the visible screen does not rebuild this source.
 * Polling uses the configured HA URL/token and only records a valid numeric
 * state. Read failures retain the last valid text value without fabricating a
 * new history point, matching the original power dashboard's behavior.
 */

#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <new>
#include "power_history.h"
#include "log.h"
#include "comms.h"
#include "config.h"

// One configurable house-power source; allocated only when present in JSON.
struct PowerSource {
    char id[32] = {};
    char entity[64] = {};
    uint32_t refresh = 2000, lastAttempt = 0;
    bool attempted = false, kilowatts = false, logging = false;
    PowerHistory history;
};
static PowerSource* g_powerSource = nullptr;

// Called with the engine mutex on reload, before engine startup on boot.
/**
 * Replace the configured source and discard its old history.
 * At most one valid Power entry is accepted. Missing/invalid entries leave the
 * source disabled, so layouts without a graph do not allocate its history.
 */
inline void ParsePowerSources(JsonDocument& doc)
{
    /**
     * A reload is an explicit history reset. No render/data operation can still
     * hold the old pointer when this function runs under the engine mutex.
     */
    delete g_powerSource;
    g_powerSource = nullptr;
    for (JsonObject src : doc["DataSources"].as<JsonArray>()) {
        if (strcmp(src["Type"] | "", "Power") != 0) continue;
        const char* id = src["Id"] | "";
        const char* entity = src["Entity"] | "";
        const char* unit = src["Unit"] | "W";
        if (!*id || strlen(id) >= 32 || !*entity || strlen(entity) >= 64 ||
            (strcmp(unit, "W") != 0 && strcmp(unit, "kW") != 0) || g_powerSource) {
            logW("[power] Invalid or excess Power source ignored (maximum one).");
            continue;
        }
        g_powerSource = new(std::nothrow) PowerSource;
        if (!g_powerSource) { logE("[power] Cannot allocate history."); return; }
        strlcpy(g_powerSource->id, id, sizeof(g_powerSource->id));
        strlcpy(g_powerSource->entity, entity, sizeof(g_powerSource->entity));
        g_powerSource->kilowatts = strcmp(unit, "kW") == 0;
        g_powerSource->refresh = constrain(src["Refresh"] | 2000UL, 2000UL, 3600000UL);
        g_powerSource->logging = doc["Log"]["Power"] | false;
    }
}

/**
 * Find the singleton source by its exact, case-sensitive Id.
 * The returned pointer is borrowed and may be invalidated by a board reload;
 * use it only inside the current engine-protected operation.
 */
inline PowerSource* FindPowerSource(const char* id)
{
    return g_powerSource && strcmp(g_powerSource->id, id) == 0 ? g_powerSource : nullptr;
}

// Existing data task / engine lock serializes polling, drawing and reload.
/**
 * Perform a due HA state request and publish a new valid sample.
 * Both successful and failed attempts advance the attempt timestamp. This
 * bounds retry frequency when HA is unavailable and preserves the original
 * minimum interval measured from request completion, not request start.
 */
inline void UpdatePowerSource()
{
    PowerSource* source = g_powerSource;
    if (!source) return;
    uint32_t now = millis();
    /**
     * History expiry is independent of successful reads. Do not reinsert the
     * last known value after an HTTP error: that would fabricate new samples.
     */
    source->history.prune(now);
    if (source->attempted && uint32_t(now - source->lastAttempt) < source->refresh) return;
    source->attempted = true;
    String url(HA_URL);
    if (!url.endsWith("/")) url += "/";
    url += "api/states/";
    url += source->entity;
    int code = -1;
    String body = httpGetBearer(url, HA_TOKEN, &code, 3000, !source->logging);
    source->lastAttempt = millis(); // minimum interval between completed attempts
    JsonDocument payload;
    bool valid = false;
    if (code == 200 && !deserializeJson(payload, body)) {
        valid = source->history.record(millis(), payload["state"] | "", source->kilowatts);
    }
    if (source->logging) {
        if (valid) logIf("[power] %s: %.1f W, samples=%u, heap=%u", source->id,
                         source->history.latest, unsigned(source->history.count), unsigned(ESP.getFreeHeap()));
        else logWf("[power] %s: failed/invalid state (HTTP %d); retaining last valid value", source->id, code);
    }
}
