#ifndef DATACACHE_H
#define DATACACHE_H

#include <Arduino.h>
#include <ArduinoJson.h>

#include "log.h"
#include "comms.h"
#include "secrets.h"
#include "datasources.h"

/*
 * datacache.h
 * -------------------------------------------------------
 * Runtime cache for the datasources, refreshed respecting each
 * one's "Refresh" interval. Read by the draw side and updated by
 * EngineDataUpdate (core 0) / loop (single core).
 *
 * Each entry mirrors the matching g_dataSources[i] element.
 * -------------------------------------------------------
 */

// Max HA entities kept in the cache per group
#define MAX_ENTITIES_CACHE 12

// Global entity->state cache (across all HA datasources). The draw
// engine reads entity states from here ("DrawHASensor").
#define MAX_ENTITY_STATES 40
struct EntityStateRec {
    char id[64];
    char state[16];
};
EntityStateRec g_entityStates[MAX_ENTITY_STATES];
int g_entityStateCount = 0;

// Upserts the state of one entity.
inline void EntityStateSet(const char* id, const char* state)
{
    for (int i = 0; i < g_entityStateCount; i++) {
        if (strcmp(g_entityStates[i].id, id) == 0) {
            strlcpy(g_entityStates[i].state, state, sizeof(g_entityStates[0].state));
            return;
        }
    }
    if (g_entityStateCount < MAX_ENTITY_STATES) {
        strlcpy(g_entityStates[g_entityStateCount].id,   id,    sizeof(g_entityStates[0].id));
        strlcpy(g_entityStates[g_entityStateCount].state, state, sizeof(g_entityStates[0].state));
        g_entityStateCount++;
    }
}

// Returns the state of an entity ("" if not seen yet).
inline const char* EntityStateGet(const char* id)
{
    for (int i = 0; i < g_entityStateCount; i++) {
        if (strcmp(g_entityStates[i].id, id) == 0) return g_entityStates[i].state;
    }
    return "";
}

// Current values of one datasource.
struct DataSourceCache {
    unsigned long lastFetchMs;   // millis() of the last successful update
    char temperature[16];
    char humidity[16];
    char pressure[16];
    char wind[16];
    int  code;                   // WMO code of the current weather
    char codeText[32];           // label of that code (from "Codes")
    char codeImage[16];          // icon name of that code
    // HA: state of each entity of the group ("" if unknown)
    int  entityCount;
    char entities[MAX_ENTITIES_CACHE][64];
    char entityState[MAX_ENTITIES_CACHE][16];
    bool valid;                  // true once at least one fetch succeeded
};

// Global cache — index-aligned with g_dataSources[].
DataSourceCache g_dataCache[MAX_DATASOURCES];

// Resolves a "datasource:<id>:<field>" reference against the cache.
// Fields: temperature, humidity, pressure, wind, code, codeText,
// code_image. Returns "--" if the datasource is unknown.
inline String DatasourceValue(const String& ref)
{
    int c1 = ref.indexOf(':', 11);   // first ':' after "datasource:"
    if (c1 < 0) return String("--");
    String id    = ref.substring(11, c1);
    String field = ref.substring(c1 + 1);

    for (int i = 0; i < g_dataSourceCount; i++) {
        if (String(g_dataSources[i].id) != id) continue;
        DataSourceCache& cacheEntry = g_dataCache[i];
        if (field == "temperature") return String(cacheEntry.temperature);
        if (field == "humidity")    return String(cacheEntry.humidity);
        if (field == "pressure")    return String(cacheEntry.pressure);
        if (field == "wind")        return String(cacheEntry.wind);
        if (field == "code")        return String(cacheEntry.code);
        if (field == "codeText" || field == "code_text") return String(cacheEntry.codeText);
        if (field == "code_image" || field == "codeImage" || field == "image")
            return String(cacheEntry.codeImage);
    }
    return String("--");
}

// ---------------------------------------------------------------
// Reads the value pointed by "path" (e.g. "current.temperature_2m")
// from a JSON document. Returns an empty String if not present.
// ---------------------------------------------------------------
inline String WeatherValue(const JsonDocument& doc, const char* path)
{
    if (!path || !path[0]) return String();

    JsonVariantConst v = doc.as<JsonVariantConst>();
    char buf[64];
    strlcpy(buf, path, sizeof(buf));

    // Walk the path step by step ("a.b.c" -> v["a"]["b"]["c"]).
    char* tok = strtok(buf, ".");
    while (tok) {
        v = v[tok];
        if (v.isNull()) return String();
        tok = strtok(nullptr, ".");
    }
    return v.as<String>();
}

// ---------------------------------------------------------------
// Refreshes the cache of a WEATHER datasource from its payload.
// ---------------------------------------------------------------
inline bool FetchWeather(DataSourceInfo& source, DataSourceCache& cacheEntry)
{
    // "quiet" silences the HTTP layer when this type is not logged,
    // so a disabled flag shows no request/status lines either.
    bool quiet = !IsDataSourceLoggingEnabled(source);
    int code = -1;
    String body = httpGet(source.url, &code, HTTP_TIMEOUT_MS, quiet);
    if (code != 200 || body.length() == 0) {
        if (!quiet) logWf("[cache] weather %s failed (HTTP %d)", source.id, code);
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        if (!quiet) logEf("[cache] weather %s invalid JSON: %s", source.id, err.c_str());
        return false;
    }

    // Read each measurement from the payload using the "Map" paths.
    strlcpy(cacheEntry.temperature, WeatherValue(doc, source.mapTemperature).c_str(), sizeof(cacheEntry.temperature));
    strlcpy(cacheEntry.humidity,    WeatherValue(doc, source.mapHumidity).c_str(),    sizeof(cacheEntry.humidity));
    strlcpy(cacheEntry.pressure,    WeatherValue(doc, source.mapPressure).c_str(),    sizeof(cacheEntry.pressure));
    strlcpy(cacheEntry.wind,        WeatherValue(doc, source.mapWind).c_str(),        sizeof(cacheEntry.wind));

    // Weather code + its label/image from the "Codes" table.
    String codestr = WeatherValue(doc, source.mapCode);
    cacheEntry.code    = codestr.toInt();
    cacheEntry.codeText[0] = '\0';
    cacheEntry.codeImage[0] = '\0';

    for (int i = 0; i < source.codeCount; i++) {
        if (source.codes[i].code == (uint16_t)cacheEntry.code) {
            strlcpy(cacheEntry.codeText,  source.codes[i].label, sizeof(cacheEntry.codeText));
            strlcpy(cacheEntry.codeImage, source.codes[i].image, sizeof(cacheEntry.codeImage));
            break;
        }
    }

    cacheEntry.valid = true;
    if (IsDataSourceLoggingEnabled(source)) logIf("[cache] weather %s: T=%s H=%s code=%d (%s)",
          source.id, cacheEntry.temperature, cacheEntry.humidity, cacheEntry.code, cacheEntry.codeText);
    return true;
}

// ---------------------------------------------------------------
// Refreshes the cache of an HA datasource (group of entities).
// Queries /api/states/<entity> for each one, one HTTP call each.
// ---------------------------------------------------------------
inline bool FetchHA(DataSourceInfo& source, DataSourceCache& cacheEntry)
{
    cacheEntry.entityCount = 0;
    bool quiet = !IsDataSourceLoggingEnabled(source);

    char buf[MAX_ENTITIES];
    strlcpy(buf, source.entities, sizeof(buf));

    char* tok = strtok(buf, ",");
    while (tok && cacheEntry.entityCount < MAX_ENTITIES_CACHE) {
        char* id = tok;
        while (*id == ' ') id++;           // trim leading spaces
        if (id[0]) {
            strlcpy(cacheEntry.entities[cacheEntry.entityCount], id, sizeof(cacheEntry.entities[0]));
            cacheEntry.entityState[cacheEntry.entityCount][0] = '\0';

            String url = String(HA_URL);
            if (!url.endsWith("/")) url += "/";
            url += "api/states/";
            url += id;

            int code = -1;
            String body = httpGetBearer(url, HA_TOKEN, &code, 8000, quiet);
            if (code == 200 && body.length() > 0) {
                JsonDocument doc;
                DeserializationError err = deserializeJson(doc, body);
                if (!err) {
                    const char* st = doc["state"] | "unknown";
                    strlcpy(cacheEntry.entityState[cacheEntry.entityCount], st,
                            sizeof(cacheEntry.entityState[0]));
                    EntityStateSet(id, st);
                    if (IsDataSourceLoggingEnabled(source)) logIf("[cache] HA %s = %s", id, st);
                }
            } else {
                if (!quiet) logWf("[cache] HA %s failed (HTTP %d)", id, code);
            }
            cacheEntry.entityCount++;
        }
        tok = strtok(nullptr, ",");
    }

    cacheEntry.valid = true;
    if (IsDataSourceLoggingEnabled(source)) logIf("[cache] HA %s: %d entities", source.id, cacheEntry.entityCount);
    return true;
}

// ---------------------------------------------------------------
// Clears every cached value (datasources + entity states). Called
// on a board reload so nothing from the previous JSON survives:
// lastFetchMs goes back to 0, so each datasource re-fetches on the
// next UpdateDataSources() tick.
// ---------------------------------------------------------------
inline void ResetDataCaches()
{
    memset(g_dataCache, 0, sizeof(g_dataCache));
    memset(g_entityStates, 0, sizeof(g_entityStates));
    g_entityStateCount = 0;
    logI("[cache] data caches reset (board reload).");
}

// ---------------------------------------------------------------
// Core 0 / loop: updates only the datasources whose Refresh elapsed.
// The very first fetch always runs (lastFetchMs == 0), even when
// millis() is still small vs. a large Refresh interval.
// ---------------------------------------------------------------
inline void UpdateDataSources()
{
    unsigned long now = millis();

    for (int i = 0; i < g_dataSourceCount; i++) {
        DataSourceInfo& source = g_dataSources[i];
        DataSourceCache& cacheEntry = g_dataCache[i];

        // Never fetched yet, or refresh interval not elapsed.
        if (cacheEntry.lastFetchMs != 0 && (now - cacheEntry.lastFetchMs) < source.refresh) continue;

        if (IsDataSourceLoggingEnabled(source)) logIf("[cache] updating datasource %s (%s)", source.id, source.type);

        cacheEntry.lastFetchMs = now;
        if (strncmp(source.type, "Weather", 7) == 0) {
            FetchWeather(source, cacheEntry);
        } else if (strncmp(source.type, "HA", 2) == 0) {
            FetchHA(source, cacheEntry);
        } else {
            logWf("[cache] unknown datasource type: %s", source.type);
        }
    }
}

#endif // DATACACHE_H
