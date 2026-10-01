#ifndef DATASOURCES_H
#define DATASOURCES_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "log.h"

/*
 * datasources.h
 * -------------------------------------------------------
 * Parses the "DataSources" of the board JSON into a global array.
 * Works with both JSON flavours — "multi" (DataSources + Screens
 * referencing external files) and "simple"/standalone (embedded
 * Objects) — in both cases the DataSources live at the document root.
 *
 * Usage:
 *   JsonDocument doc;
 *   deserializeJson(doc, json);
 *   ParseDataSources(doc);
 *   // afterwards: g_dataSources[i].id / .type / .refresh / ...
 *
 * Log flags ("Log" at the root of the JSON) gate whether each type
 * of datasource is printed to the log. Default: OFF.
 * -------------------------------------------------------
 */

// Limits
#define MAX_DATASOURCES 16
#define MAX_ENTITIES    512
#define MAX_WX_CODES    16

// A weather-code mapping: WMO code -> label + icon name
// (parsed from the "Codes" object of a Weather datasource).
struct WxCode {
    uint16_t code;      // WMO weather code (key in "Codes")
    char label[24];     // human readable text (e.g. "Chuva moderada")
    char image[16];     // icon name from images.h (e.g. "rain")
};

// A single datasource, fully described by the board JSON.
struct DataSourceInfo {
    char id[32];                  // datasource Id
    char type[16];                // "Weather" or "HA"
    unsigned long refresh;        // refresh interval in ms
    char url[256];                // external datasource URL (e.g. Weather)
    char entities[MAX_ENTITIES];  // HA datasource: entity list (comma group)

    // Weather: path in the payload for each measurement ("Map" object)
    char mapTemperature[48];
    char mapHumidity[48];
    char mapPressure[48];
    char mapWind[48];
    char mapCode[48];

    // Weather: WMO code lookup table ("Codes" object)
    WxCode codes[MAX_WX_CODES];
    int codeCount;
};

// Global parsed datasources (indexed the same way as g_dataCache).
DataSourceInfo g_dataSources[MAX_DATASOURCES];
int g_dataSourceCount = 0;

// Log flags, read from the "Log" object at the JSON root.
// Default (missing or false) = do NOT print that type of data.
bool g_logWeather = false;
bool g_logHA      = false;

// Whether the log for a given datasource is enabled (by type).
inline bool IsDataSourceLoggingEnabled(const DataSourceInfo& ds)
{
    if (strncmp(ds.type, "Weather", 7) == 0) return g_logWeather;
    if (strncmp(ds.type, "HA", 2) == 0)      return g_logHA;
    return true;   // unknown types always log
}

// Reads the "Log" object from the document root.
// Missing object -> both flags stay false (nothing logged).
inline void ParseLogFlags(JsonDocument& doc)
{
    JsonObject lg = doc["Log"];
    if (lg.isNull()) {
        logD("[datasources] No \"Log\" object in JSON (flags disabled).");
        return;
    }
    g_logWeather = lg["Weather"] | false;
    g_logHA      = lg["HA"] | false;
    logIf("[datasources] Log flags: weather=%d ha=%d", g_logWeather, g_logHA);
}

// Counts comma-separated entities in a HA datasource list.
inline int EntityCount(const char* entities)
{
    int n = 0;
    const char* p = entities;
    while (p && *p) {
        while (*p == ' ') p++;          // skip leading spaces
        if (*p) n++;                    // one more entity
        while (*p && *p != ',') p++;    // skip to next comma
        if (*p == ',') p++;
    }
    return n;
}

// Normalizes "Entities": accepts a comma String OR a JSON array, and
// stores the result (comma separated) into out[]. Returns count.
inline int ParseEntities(JsonVariant entities, char* out, size_t outSize)
{
    out[0] = '\0';
    if (entities.is<String>()) {
        strlcpy(out, entities.as<const char*>(), outSize);
        return EntityCount((char*)out);
    }
    if (entities.is<JsonArray>()) {
        bool first = true;
        for (JsonVariant e : entities.as<JsonArray>()) {
            if (!first) strlcat(out, ",", outSize);
            strlcat(out, e.as<const char*>(), outSize);
            first = false;
        }
        return EntityCount((char*)out);
    }
    return 0;
}

// Parses the Weather part (Map + Codes) of a datasource.
inline void ParseWeather(JsonObject src, DataSourceInfo& ds)
{
    // "Map": payload path for each measurement.
    JsonObject map = src["Map"];
    if (!map.isNull()) {
        strlcpy(ds.mapTemperature, map["temperature"] | "", sizeof(ds.mapTemperature));
        strlcpy(ds.mapHumidity,    map["humidity"]    | "", sizeof(ds.mapHumidity));
        strlcpy(ds.mapPressure,    map["pressure"]    | "", sizeof(ds.mapPressure));
        strlcpy(ds.mapWind,        map["wind"]        | "", sizeof(ds.mapWind));
        strlcpy(ds.mapCode,        map["code"]        | "", sizeof(ds.mapCode));
    }

    // "Codes": WMO code -> {label, image}.
    ds.codeCount = 0;
    JsonObject codes = src["Codes"];
    if (!codes.isNull()) {
        for (JsonPair kv : codes) {
            if (ds.codeCount >= MAX_WX_CODES) break;
            const char* key = kv.key().c_str();
            if (!key) continue;
            long c = strtol(key, nullptr, 10);   // key is a numeric string

            JsonObject cobj = kv.value();
            if (cobj.isNull()) continue;

            WxCode& wc = ds.codes[ds.codeCount];
            wc.code = (uint16_t)c;
            strlcpy(wc.label, cobj["label"] | "", sizeof(wc.label));
            strlcpy(wc.image, cobj["image"] | "", sizeof(wc.image));
            ds.codeCount++;
        }
    }
}

// Parses the "DataSources" array of the document. Never fails if absent.
inline void ParseDataSources(JsonDocument& doc)
{
    g_dataSourceCount = 0;

    JsonArray arr = doc["DataSources"].as<JsonArray>();
    if (arr.isNull()) {
        logI("[datasources] JSON has no DataSources.");
        return;
    }

    for (JsonObject src : arr) {
        if (g_dataSourceCount >= MAX_DATASOURCES) break;

        DataSourceInfo& ds = g_dataSources[g_dataSourceCount];
        strlcpy(ds.id,   src["Id"]   | "",  sizeof(ds.id));
        strlcpy(ds.type, src["Type"] | "",  sizeof(ds.type));
        ds.refresh = src["Refresh"] | 60000UL;
        strlcpy(ds.url,  src["url"]  | "",  sizeof(ds.url));

        // "Entities" accepts "a,b,c" (String) or ["a","b","c"] (array).
        ds.entities[0] = '\0';
        if (src["Entities"]) {
            ParseEntities(src["Entities"], ds.entities, sizeof(ds.entities));
        }

        ds.mapTemperature[0] = ds.mapHumidity[0] = ds.mapPressure[0] = '\0';
        ds.mapWind[0] = ds.mapCode[0] = '\0';
        ds.codeCount = 0;
        if (strncmp(ds.type, "Weather", 7) == 0) {
            ParseWeather(src, ds);
        }

        if (EntityCount(ds.entities) > 0) {
            logIf("DataSource #%d: id=%s type=%s refresh=%lu entities=%d",
                  g_dataSourceCount, ds.id, ds.type, ds.refresh,
                  EntityCount(ds.entities));
        } else {
            logIf("DataSource #%d: id=%s type=%s refresh=%lu url=%s codes=%d",
                  g_dataSourceCount, ds.id, ds.type, ds.refresh, ds.url,
                  ds.codeCount);
        }

        g_dataSourceCount++;
    }

    logIf("[datasources] %d datasources.", g_dataSourceCount);
}

#endif // DATASOURCES_H