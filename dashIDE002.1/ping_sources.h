/**
 * @file ping_sources.h
 *
 * Independent ICMP availability checks for up to eight JSON-configured hosts.
 * The worker copies one source under g_pingMutex, releases the mutex during
 * DNS/mDNS/ICMP operations, then publishes the result under the mutex again.
 * A generation counter prevents a completed request from updating a new list
 * after reload. This separate ownership keeps slow pings out of the engine's
 * rendering critical section. One finite ping session is active at a time.
 */

#pragma once

// ICMP is provided by the ESP32 core; no extra Ping library is required.
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ping/ping_sock.h>
#include <lwip/ip_addr.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include "log.h"

static constexpr int MAX_PING_SOURCES = 8;
struct PingSource {
    char id[32];
    char host[64];
    uint32_t refresh;
    uint32_t lastCheck;
    bool checked;
    bool online;
};
static PingSource g_pingSources[MAX_PING_SOURCES] = {};
static int g_pingCount = 0;
static uint32_t g_pingGeneration = 0;
static bool g_logPing = false;
static SemaphoreHandle_t g_pingMutex = nullptr;
static TaskHandle_t g_pingTask = nullptr;

// Callback only signals completion. The worker owns the session and context
// until the end callback, including failed / timed out echo requests.
/**
 * FreeRTOS ping-task callback, not a GPIO interrupt handler.
 * Signal the waiting worker; it owns the semaphore and deletes it only after
 * this finite session has completed. No rendering or engine locks belong here.
 */
static void PingEnded(esp_ping_handle_t, void* arg)
{
    xSemaphoreGive(static_cast<SemaphoreHandle_t>(arg));
}

/**
 * Run two IPv4 echo requests against an already resolved address.
 * Any reply means online. Resource/start failures return false. The worker
 * waits for the end callback before releasing session-related resources; this
 * function must not be called while holding the short-lived ping-state mutex.
 */
inline bool PingAddress(IPAddress address)
{
    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    config.count = 2;
    config.interval_ms = 1000;
    config.timeout_ms = 2000;
    config.task_stack_size = 3072;
    IP_ADDR4(&config.target_addr, address[0], address[1], address[2], address[3]);
    SemaphoreHandle_t done = xSemaphoreCreateBinary();
    if (!done) return false;
    esp_ping_callbacks_t callbacks = {};
    callbacks.on_ping_end = PingEnded;
    callbacks.cb_args = done;
    esp_ping_handle_t session = nullptr;
    bool online = false;
    if (esp_ping_new_session(&config, &callbacks, &session) == ESP_OK) {
        if (esp_ping_start(session) == ESP_OK) {
            // A finite two-echo session always ends after its bounded timeouts.
            // Do not free the callback context while the ping task is using it.
            xSemaphoreTake(done, portMAX_DELAY);
            uint32_t replies = 0;
            esp_ping_get_profile(session, ESP_PING_PROF_REPLY, &replies, sizeof(replies));
            online = replies > 0;
        }
        esp_ping_delete_session(session);
    }
    vSemaphoreDelete(done);
    return online;
}

/**
 * Resolve and probe one candidate name; do not append aliases here.
 * IP literals skip name resolution. A .local suffix routes to mDNS with a
 * bounded query; other names use the Arduino Wi-Fi DNS resolver. The caller
 * implements the optional second attempt with a .local alias.
 */
inline bool PingHost(String host)
{
    IPAddress address;
    if (address.fromString(host)) return PingAddress(address);
    if (host.endsWith(".local")) {
        static bool mdnsReady = false;
        if (!mdnsReady) {
            String name = "dashping-" + String((uint32_t)ESP.getEfuseMac(), HEX);
            mdnsReady = MDNS.begin(name.c_str());
        }
        if (!mdnsReady) return false;
        host.remove(host.length() - 6);
        address = MDNS.queryHost(host.c_str(), 1500);
        return (uint32_t)address != 0 && PingAddress(address);
    }
    return WiFi.hostByName(host.c_str(), address) == 1 && PingAddress(address);
}

/**
 * Poll due hosts sequentially until the device restarts.
 * Snapshot, network operation and publish are deliberately separate phases.
 * A configuration-generation mismatch discards the old result. Refresh is
 * measured from completion; a slow host can extend the overall round.
 */
static void PingWorker(void*)
{
    for (;;) {
        for (int i = 0; i < MAX_PING_SOURCES; ++i) {
            PingSource source;
            uint32_t generation;
            bool logging;
            xSemaphoreTake(g_pingMutex, portMAX_DELAY);
            if (i >= g_pingCount) {
                xSemaphoreGive(g_pingMutex);
                break;
            }
            /**
             * Copy all state needed for the network operation before releasing the lock.
             * The table can now be safely replaced by the UI's JSON reload path.
             */
            source = g_pingSources[i];
            generation = g_pingGeneration;
            logging = g_logPing;
            xSemaphoreGive(g_pingMutex);
            if (source.checked && uint32_t(millis() - source.lastCheck) < source.refresh)
                continue;
            bool online = false;
            if (WiFi.status() == WL_CONNECTED) {
                online = PingHost(source.host);
                // Raspberry Pi behaviour: try the .local alias after failure.
                // IP literals deliberately have no hostname fallback.
                IPAddress literal;
                String host(source.host);
                if (!online && !literal.fromString(host) && !host.endsWith(".local"))
                    online = PingHost(host + ".local");
            }
            xSemaphoreTake(g_pingMutex, portMAX_DELAY);
            // A reload may have replaced the list while this request ran.
            if (generation == g_pingGeneration) {
                g_pingSources[i].online = online;
                g_pingSources[i].checked = true;
                g_pingSources[i].lastCheck = millis();
            }
            xSemaphoreGive(g_pingMutex);
            if (logging && (!source.checked || source.online != online))
                logIf("[ping] %s: %s", source.host, online ? "OK" : "FAIL");
        }
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

/**
 * Atomically replace the ping list and reset its status on JSON reload.
 * Reject missing/oversized identifiers and duplicate Ids rather than silently
 * truncating them. The worker is created lazily for a non-empty list and reused
 * on later reloads, including when the list becomes empty.
 */
inline void ParsePingSources(JsonDocument& doc)
{
    if (!g_pingMutex) g_pingMutex = xSemaphoreCreateMutex();
    if (!g_pingMutex) { logE("[ping] Cannot create mutex."); return; }
    xSemaphoreTake(g_pingMutex, portMAX_DELAY);
    /**
     * Invalidate every in-flight snapshot before replacing the list. Index i
     * alone is insufficient because a new configuration can reuse the same slot.
     */
    ++g_pingGeneration;
    g_pingCount = 0;
    memset(g_pingSources, 0, sizeof(g_pingSources));
    g_logPing = doc["Log"]["Ping"] | false;
    for (JsonObject src : doc["DataSources"].as<JsonArray>()) {
        if (strcmp(src["Type"] | "", "Ping") != 0) continue;
        const char* id = src["Id"] | "";
        const char* host = src["Host"] | "";
        bool duplicate = false;
        for (int i = 0; i < g_pingCount; ++i)
            if (strcmp(g_pingSources[i].id, id) == 0) duplicate = true;
        if (!id[0] || !host[0] || strlen(id) >= 32 || strlen(host) >= 64 ||
            duplicate || g_pingCount >= MAX_PING_SOURCES) {
            logW("[ping] Invalid, duplicate or excess Ping source ignored.");
            continue;
        }
        PingSource& source = g_pingSources[g_pingCount++];
        strlcpy(source.id, id, sizeof(source.id));
        strlcpy(source.host, host, sizeof(source.host));
        source.refresh = constrain(src["Refresh"] | 10000UL, 1000UL, 3600000UL);
    }
    bool hasSources = g_pingCount > 0;
    xSemaphoreGive(g_pingMutex);
    if (hasSources && !g_pingTask &&
        xTaskCreate(PingWorker, "dashPing", 4096, nullptr, 1, &g_pingTask) != pdPASS)
        logE("[ping] Cannot create worker.");
}

/**
 * Return a display-safe snapshot: -- before a result, otherwise OK/FAIL.
 * A previously checked host cannot appear online while Wi-Fi is disconnected.
 * The mutex is held only while copying state, never during DNS or ICMP work.
 */
inline String PingValue(const char* id)
{
    String value = "--";
    if (!g_pingMutex) return value;
    xSemaphoreTake(g_pingMutex, portMAX_DELAY);
    for (int i = 0; i < g_pingCount; ++i) {
        const PingSource& source = g_pingSources[i];
        if (strcmp(source.id, id) == 0 && source.checked) {
            value = source.online && WiFi.status() == WL_CONNECTED ? "OK" : "FAIL";
            break;
        }
    }
    xSemaphoreGive(g_pingMutex);
    return value;
}
