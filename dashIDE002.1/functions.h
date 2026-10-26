/**
 * @file functions.h
 *
 * Resolve JSON expressions without issuing network requests during drawing.
 * function: reads local device/time state; entity: reads an HA entity cache;
 * datasource: reads weather fields; ping: reads the ping worker's latest result;
 * power: returns the most recent valid signed watts as an integer string.
 * The boolean resolver supports fewer expressions than the text resolver.
 * In particular, arbitrary text/entity values are not implicitly truthy.
 */

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <Arduino.h>
#include <time.h>

#include "comms.h"
#include "datacache.h"
#include "datasources.h"

/*
 * functions.h
 * -------------------------------------------------------
 * Resolves the special placeholders used inside object fields:
 *
 *   - "function:Clock"        -> HH:MM:SS (NTP time, local tz with DST)
 *   - "function:Date"         -> YYYY/MM/DD
 *   - "function:Uptime"       -> HH:MM:SS since boot
 *   - "function:WifiRSSI"     -> "-45 dBm"
 *   - "function:FreeRAM"      -> "123 KB"
 *   - "function:MyIP"         -> "192.168.x.x"
 *   - "function:IsWIFIConnected" -> true/false (bool resolver)
 *   - "function:IsMQTTConnected" -> false (no MQTT in this project)
 *   - "function:IsHAConnected"   -> true if any HA datasource has
 *     a valid cached value
 *
 *   - "datasource:<id>:<field>"> value from the datasource cache
 *     (temperature, humidity, pressure, wind, code, codeText,
 *      code_image). Resolved by DatasourceValue() in datacache.h.
 *
 * ResolveTextValue() is used everywhere a display string is needed;
 * ResolveBoolValue() where the field is a true/false expression.
 * -------------------------------------------------------
 */

// =============================================================
// Timezone helpers.
//
// The JSON accepts a simple "<region>/<city>" name (e.g.
// "Europe/Lisbon", "PT/Lisbon", "Portugal/Lisbon" or "UTC").
// Internally the OS needs a POSIX TZ string
// (e.g. "WET0WEST,M3.5.0/1,M10.5.0"). timeZoneToPosix() matches
// the known cities (the part after the last "/") and returns the
// POSIX string; anything else is assumed to already be POSIX and
// is copied as-is.
// =============================================================
/**
 * Translate the supported city aliases to POSIX TZ syntax.
 * An unknown name is passed through as already-POSIX input; this is a small
 * lookup, not an IANA timezone database. outLen includes the NUL terminator.
 */
inline void timeZoneToPosix(const char* name, char* out, size_t outLen)
{
    if (!name || !*name || outLen == 0) {
        if (outLen > 0) out[0] = '\0';
        return;
    }

    static const char tzTable[][2][34] PROGMEM = {
        {"UTC",                 "UTC0"},
        {"Lisbon",              "WET0WEST,M3.5.0/1,M10.5.0"},
        {"Madeira",             "WET0WEST,M3.5.0/1,M10.5.0"},
        {"Azores",              "AZOT1AZOST,M3.5.0/0,M10.5.0/1"},
        {"London",              "GMT0BST,M3.5.0/1,M10.5.0"},
        {"Madrid",              "CET-1CEST,M3.5.0/2,M10.5.0/3"},
        {"Paris",               "CET-1CEST,M3.5.0/2,M10.5.0/3"},
        {"Berlin",              "CET-1CEST,M3.5.0/2,M10.5.0/3"},
        {"New_York",            "EST5EDT,M3.2.0/2,M11.1.0/2"},
        {"Sao_Paulo",           "BRT3BRST,M11.1.0/0,M2.3.0/0"},
    };
    const size_t n = sizeof(tzTable) / sizeof(tzTable[0]);

    // City = the part after the last "/" (mirrors "PT/Lisbon").
    const char* city = strrchr(name, '/');
    city = (city) ? city + 1 : name;

    for (size_t i = 0; i < n; i++) {
        if (strcmp_P(city, tzTable[i][0]) == 0) {
            strncpy_P(out, tzTable[i][1], outLen - 1);
            out[outLen - 1] = '\0';
            return;
        }
    }

    // Not in the table: assume it is already a POSIX string.
    strncpy(out, name, outLen - 1);
    out[outLen - 1] = '\0';
}

// =============================================================
// Small text/bool functions (implemented here in the header).
// =============================================================
inline String Clock()
{
    time_t now = time(nullptr);
    if (now < 100000) return "--:--:--";   // NTP not synced yet
    struct tm t;
    localtime_r(&now, &t);
    char b[16];
    snprintf(b, sizeof(b), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
    return String(b);
}

inline String Date()
{
    time_t now = time(nullptr);
    if (now < 100000) return "--/--/----";
    struct tm t;
    localtime_r(&now, &t);
    char b[16];
    snprintf(b, sizeof(b), "%04d/%02d/%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
    return String(b);
}

inline String Uptime()
{
    unsigned long s = millis() / 1000UL;
    char b[24];
    snprintf(b, sizeof(b), "%02lu:%02lu:%02lu", s / 3600UL, (s % 3600UL) / 60UL, s % 60UL);
    return String(b);
}

inline String WifiRSSI()
{
    return String(wifiRSSI()) + " dBm";
}

inline String FreeRAM()
{
    return String((unsigned)(ESP.getFreeHeap() / 1024UL)) + " KB";
}

inline String MyIP()
{
    return wifiIP();
}

// No MQTT support in this project — kept for parity with the
// reference resolver (always false).
inline bool IsMQTTConnected()
{
    return false;
}

// True once an HA datasource has been marked processed; not a live health check.
/**
 * Report whether an HA group has been processed and marked valid.
 * This legacy convenience flag is not a live connection test and must not be
 * used as proof that all entities are reachable or their cached states fresh.
 */
inline bool IsHAConnected()
{
    for (int i = 0; i < g_dataSourceCount; i++) {
        if (strncmp(g_dataSources[i].type, "HA", 2) == 0 && g_dataCache[i].valid) {
            return true;
        }
    }
    return false;
}

// =============================================================
// Text value resolver.
// =============================================================
/**
 * Resolve one complete expression; literal text passes through unchanged.
 * No interpolation inside arbitrary sentences is performed. Use Prefix/Suffix
 * for surrounding labels and units. Cached invalid HA state strings become --,
 * while Power retains its last valid signed value as defined by its source.
 */
inline String ResolveTextValue(String text)
{
    text.trim();

    if (text.startsWith("function:")) {
        String f = text.substring(9);
        f.trim();
        if (f.equalsIgnoreCase("Clock"))    return Clock();
        if (f.equalsIgnoreCase("Date"))     return Date();
        if (f.equalsIgnoreCase("Uptime"))   return Uptime();
        if (f.equalsIgnoreCase("WifiRSSI")) return WifiRSSI();
        if (f.equalsIgnoreCase("FreeRAM"))  return FreeRAM();
        if (f.equalsIgnoreCase("MyIP"))     return MyIP();
        return "[" + f + "?]";
    }

    // Numeric and other HA states already fetched by HA DataSources.
    if (text.startsWith("entity:")) {
        String id = text.substring(7);
        id.trim();
        const char* state = EntityStateGet(id.c_str());
        if (!state[0] || strcmp(state, "unknown") == 0 ||
            strcmp(state, "unavailable") == 0) return "--";
        return String(state);
    }
    if (text.startsWith("power:")) {
        PowerSource* source = FindPowerSource(text.substring(6).c_str());
        return source && source->history.valid ? String(long(source->history.latest)) : String("--");
    }
    if (text.startsWith("ping:")) return PingValue(text.substring(5).c_str());
    if (text.startsWith("datasource:")) {
        return DatasourceValue(text);
    }

    return text;
}

// =============================================================
// Boolean value resolver (e.g. DrawImageBool "Value").
// =============================================================
/**
 * Resolve the supported boolean expressions without string coercion.
 * ping:<Id> is true only for an explicit OK snapshot. Unknown expressions,
 * unsupported entity references and MQTT return false.
 */
inline bool ResolveBoolValue(String text)
{
    text.trim();
    if (text.startsWith("ping:")) return PingValue(text.substring(5).c_str()) == "OK";
    if (!text.startsWith("function:")) return false;

    String f = text.substring(9);
    f.trim();
    if (f.equalsIgnoreCase("IsWIFIConnected")) return wifiConnected();
    if (f.equalsIgnoreCase("IsMQTTConnected")) return IsMQTTConnected();
    if (f.equalsIgnoreCase("IsHAConnected"))   return IsHAConnected();
    return false;
}

#endif // FUNCTIONS_H