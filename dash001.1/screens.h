#ifndef SCREENS_H
#define SCREENS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "log.h"

/*
 * screens.h
 * -------------------------------------------------------
 * Parses the "Screens" array of the board JSON into a global list.
 *
 * Two JSON flavours are handled:
 *   - "multi" (board_name.json): Screens[] only reference external
 *     files via "name" (e.g. "main.json"). The file itself (with the
 *     "Objects") is fetched at runtime from JSON_URL + name.
 *   - "simple"/standalone: the first screen already embeds "Objects"
 *     in the same JSON; nothing extra to download.
 *   - bare "sub-screen" (main.json, jpg.json): no "Screens" array —
 *     "Refresh"/"Objects" sit at the document root. Parsed as screen
 *     #1 (so the project can load a sub-screen file directly).
 *
 * Usage:
 *   JsonDocument doc;
 *   deserializeJson(doc, json);
 *   ParseScreens(doc);
 *   // afterwards: g_screenCount, g_screens[i].number / .name / ...
 * -------------------------------------------------------
 */

// Limits
#define MAX_SCREENS 16

// A single screen, taken from the "Screens" array of the board JSON.
struct ScreenInfo {
    int number;        // screen number as written in the JSON
    char name[48];     // external file name (e.g. "main.json"); empty if embedded
    bool embedded;     // true when this screen has "Objects" in the board JSON itself
    uint8_t rotation;  // requested display rotation
    bool invert;       // requested display invert
    unsigned long refresh;  // redraw interval in ms (how often to refresh)
};

// Global parsed screens (indexed by screen slot).
ScreenInfo g_screens[MAX_SCREENS];
int g_screenCount = 0;

// Redraw interval of the currently loaded screen (default 1000 ms).
// Updated by LoadScreen() from the screen "Refresh" field.
unsigned long g_screenRefresh = 1000UL;

// Returns the slot index of the screen with the given number, or -1.
inline int FindScreen(int number)
{
    for (int i = 0; i < g_screenCount; i++) {
        if (g_screens[i].number == number) return i;
    }
    return -1;
}

// Returns the JsonObject of the screen with the given number inside the
// "Screens" array (used to read the embedded "Objects" of standalone
// screens), or the document root itself for the bare sub-screen format
// (no "Screens" array, "Objects" at the root -> the single screen).
// Null object if not found.
inline JsonObject FindScreenJson(JsonDocument& doc, int number)
{
    JsonArray arr = doc["Screens"].as<JsonArray>();
    if (arr.isNull()) {
        if (!doc["Objects"].isNull() && number == 1) return doc.as<JsonObject>();
        return JsonObject();
    }
    for (JsonObject scn : arr) {
        if ((int)(scn["Number"] | 0) == number) return scn;
    }
    return JsonObject();
}

// Reads the "Screens" array of the document. Never fails if absent.
// The bare sub-screen format (Objects at the root) becomes screen #1.
inline void ParseScreens(JsonDocument& doc)
{
    g_screenCount = 0;

    JsonArray arr = doc["Screens"].as<JsonArray>();
    if (arr.isNull()) {
        if (!doc["Objects"].isNull()) {
            // A screen file (e.g. main.json) loaded directly as the board.
            ScreenInfo& sc = g_screens[0];
            sc.number   = 1;
            sc.name[0]  = '\0';
            sc.embedded = true;
            sc.rotation = doc["Rotation"] | 0;
            sc.invert   = doc["Invert"] | false;
            sc.refresh  = doc["Refresh"] | 1000UL;
            g_screenCount = 1;
            logI("[screens] Bare sub-screen format: root has Objects -> screen #1.");
            return;
        }
        logI("[screens] JSON has no Screens.");
        return;
    }

    for (JsonObject scn : arr) {
        if (g_screenCount >= MAX_SCREENS) break;

        ScreenInfo& sc = g_screens[g_screenCount];
        sc.number   = scn["Number"] | (g_screenCount + 1);
        strlcpy(sc.name, scn["name"] | "", sizeof(sc.name));
        sc.embedded = !scn["Objects"].isNull();
        sc.rotation = scn["Rotation"] | 0;
        sc.invert   = scn["Invert"] | false;
        sc.refresh  = scn["Refresh"] | 1000UL;

        logIf("Screen #%d: number=%d file=%s embedded=%d rot=%d invert=%d refresh=%lu",
              g_screenCount, sc.number,
              sc.name[0] ? sc.name : "(none)", sc.embedded,
              sc.rotation, sc.invert, sc.refresh);

        g_screenCount++;
    }

    logIf("[screens] %d screen(s).", g_screenCount);
}

#endif // SCREENS_H