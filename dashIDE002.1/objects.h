/**
 * @file objects.h
 *
 * Parse the active screen's JSON Objects into a fixed-capacity render list.
 * Only this screen's objects are retained; loading another screen resets the
 * list and its incremental-draw metadata. Source documents can be discarded
 * after parsing because strings and graph colors are copied into ObjectInfo.
 * The parser does not scale coordinates. Invalid/unsupported types and Touch
 * objects are skipped, and entries beyond MAX_OBJECTS are not stored.
 */

#ifndef OBJECTS_H
#define OBJECTS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "log.h"
#include "colors.h"
#include "power_graph_json.h"

/*
 * objects.h
 * -------------------------------------------------------
 * The "Objects" array of the currently loaded screen.
 *
 * When a screen is loaded (LoadScreen in dash001.ino):
 *   1. ClearObjects() zeroes the array (count = 0).
 *   2. ParseObjects(source) fills it again with the objects
 *      of that screen.
 *
 * source may be:
 *   - the JsonDocument of a downloaded screen file (multi
 *     format: "Objects" is at the document root), or
 *   - the screen JsonObject inside the board JSON (standalone:
 *     "Objects" is embedded next to "Rotation"/"Invert"/...).
 *
 * The draw engine (draw.h) walks g_objects[] on every screen
 * refresh. Touch is ignored for now: "Touch" objects are not
 * stored, and inline Touch* fields are left unparsed.
 * -------------------------------------------------------
 */

// Limits
#define MAX_OBJECTS    30
#define MAX_STATE_MAPS 6

// Object kinds supported by the draw engine.
enum ObjectType {
    OBJ_UNKNOWN = 0,
    OBJ_FILLSCREEN,
    OBJ_DRAWTEXT,
    OBJ_DRAWIMAGE,
    OBJ_DRAWIMAGEBOOL,
    OBJ_DRAWHA_SENSOR,  // "DrawHASensor" (one or more comma-separated entities)
    OBJ_DRAWPOWERGRAPH,
    OBJ_TOUCH
};

// A single object, fully described by the screen JSON.
struct ObjectInfo {
    ObjectType type;
    PowerGraphColors graphColors; // RGB565, used only by DrawPowerGraph

    // Common position.
    int x, y;       // main position (also touch zone x1/y1)
    int x2, y2;     // secondary position (touch zone x2/y2)
    int zoom;       // DrawImage zoom factor
    int labelX, labelY;  // DrawHASensor label position
    uint8_t font;   // DrawText / DrawHASensor font

    // DrawText fields. Sized for the longest resolvable references
    // (e.g. "datasource:weather_aveiro:temperature" = 37 chars + NUL).
    char text[40];
    char prefix[24];   // "Humidade: " (10 chars) fits with room to spare
    char suffix[16];
    char color[14];          // color name (text, image override, ...)

    // DrawImage fields.
    char image[40];          // image name (or datasource:xxx / function:xxx)

    // DrawImageBool fields. "function:IsWIFIConnected" = 24 chars (fit
    // with room to spare; a 24-byte buffer would drop the last char).
    char boolValue[40];      // function expression or ping:<id> (up to 31-char Id)
    char imageTrue[24];      // command "DrawImage:wifi_on" or "none"/""
    char imageFalse[24];

    // DrawHASensor fields.
    char entities[192];      // comma separated entity list (longest: 171+NUL)
    char stateKey[MAX_STATE_MAPS][16];   // "state:detected" (14+NUL), "state:open", "else", ...
    char stateCmd[MAX_STATE_MAPS][24];   // "DrawImage:person_off" (20+NUL), ...
    int  stateCount;

    // Touch fields (ignored for now — Touch objects and inline touch
    // zones are not stored; touch wiring comes later).

    // Runtime: last drawn state (for change detection between redraws).
    char lastState[40];
    uint32_t lastTextHash;
    uint16_t lastTextWidth;
    bool hasDrawnText;
};

// Global parsed objects of the current screen. The array resets
// (count = 0) every time a new screen is loaded; the entries are
// then repopulated by ParseObjects().
ObjectInfo g_objects[MAX_OBJECTS];
int g_objectCount = 0;

// Resets the object array (called when loading a new screen).
/**
 * Forget the active render-list length; the next parse initializes each slot.
 * Unreferenced previous entries are ignored rather than separately destroyed,
 * as ObjectInfo stores copied buffers and scalar drawing metadata only.
 */
inline void ClearObjects()
{
    g_objectCount = 0;
}

// Maps a JSON "Type" string to its enum.
inline ObjectType ObjectTypeFromString(const char* t)
{
    if (!t || !t[0]) return OBJ_UNKNOWN;
    if (strcmp(t, "FillScreen") == 0)            return OBJ_FILLSCREEN;
    if (strcmp(t, "DrawText") == 0)              return OBJ_DRAWTEXT;
    if (strcmp(t, "DrawImage") == 0)             return OBJ_DRAWIMAGE;
    if (strcmp(t, "DrawImageBool") == 0)         return OBJ_DRAWIMAGEBOOL;
    if (strcmp(t, "DrawHASensor") == 0)          return OBJ_DRAWHA_SENSOR;
    if (strcmp(t, "DrawHASensors") == 0)         return OBJ_DRAWHA_SENSOR;
    if (strcmp(t, "DrawHASensorMultiSensor") == 0) return OBJ_DRAWHA_SENSOR;
    if (strcmp(t, "DrawPowerGraph") == 0)        return OBJ_DRAWPOWERGRAPH;
    if (strcmp(t, "Touch") == 0)                 return OBJ_TOUCH;
    return OBJ_UNKNOWN;
}

// Copies an optional string field into a buffer (skips missing fields).
/**
 * Copy a JSON string with NUL termination, or clear the destination.
 * This does not coerce numbers/booleans into text. n includes the terminator;
 * callers choose buffers that fit the complete special expression.
 */
inline void CopyStr(char* dst, size_t n, JsonVariant value)
{
    if (value.is<const char*>()) {
        strlcpy(dst, value.as<const char*>(), n);
    } else {
        dst[0] = '\0';
    }
}

// Reads the "Objects" array from `source` and fills g_objects[].
// Never fails if absent (array just stays empty).
/**
 * Populate the render list in JSON painter order.
 * Legacy _Type-only records and unsupported Touch objects are intentionally
 * ignored. A new slot is reset before parsing so prior text hashes, dimensions
 * and state-image selections do not survive a screen change.
 */
inline void ParseObjects(JsonVariant source)
{
    ClearObjects();

    JsonArray arr = source["Objects"].as<JsonArray>();
    if (arr.isNull()) {
        logI("[objects] JSON has no Objects.");
        return;
    }

    for (JsonObject jsonObj : arr) {
        if (g_objectCount >= MAX_OBJECTS) break;

        // Object type from "Type". Entries using the legacy "_Type" key
        // are fully ignored (skipped silently — not part of the layout).
        if (!jsonObj["Type"].is<const char*>() && jsonObj["_Type"].is<const char*>()) {
            continue;
        }
        const char* t = jsonObj["Type"] | "";
        ObjectType ot = ObjectTypeFromString(t);
        if (ot == OBJ_UNKNOWN) {
            logWf("[objects] Unknown Type \"%s\".", t);
            continue;
        }
        if (ot == OBJ_TOUCH) {
            continue;   // touch is ignored for now (not stored)
        }

        ObjectInfo& obj = g_objects[g_objectCount];
        memset(&obj, 0, sizeof(obj));
        obj.type = ot;
        obj.x    = jsonObj["X"] | 0;
        obj.y    = jsonObj["Y"] | 0;
        obj.zoom = jsonObj["Zoom"] | 1;
        obj.font = jsonObj["Font"] | 1;
        CopyStr(obj.color,  sizeof(obj.color),  jsonObj["Color"]);
        CopyStr(obj.text,   sizeof(obj.text),   jsonObj["Text"]);
        CopyStr(obj.prefix, sizeof(obj.prefix), jsonObj["Preffix"]);
        if (!obj.prefix[0]) {
            CopyStr(obj.prefix, sizeof(obj.prefix), jsonObj["Prefix"]);
        }
        CopyStr(obj.suffix, sizeof(obj.suffix), jsonObj["Suffix"]);

        switch (ot) {
            case OBJ_FILLSCREEN:
                break;

            /**
             * Reuse the image buffer for Source and the secondary coordinates for size.
             * Colors are resolved once during parsing instead of storing three additional
             * color-name buffers or resolving names during every redraw.
             */
            case OBJ_DRAWPOWERGRAPH:
                obj.graphColors = ParsePowerGraphColors(jsonObj, [](const char* name) { return GetColorFromName(name); });
                CopyStr(obj.image, sizeof(obj.image), jsonObj["Source"]);
                obj.x2 = jsonObj["Width"] | 178;
                obj.y2 = jsonObj["Height"] | 90;
                break;

            case OBJ_DRAWIMAGE:
                CopyStr(obj.image, sizeof(obj.image), jsonObj["Image"]);
                break;

            case OBJ_DRAWIMAGEBOOL:
                CopyStr(obj.boolValue,  sizeof(obj.boolValue),  jsonObj["Value"]);
                CopyStr(obj.imageTrue,  sizeof(obj.imageTrue),  jsonObj["true"]);
                CopyStr(obj.imageFalse, sizeof(obj.imageFalse), jsonObj["false"]);
                break;

            case OBJ_DRAWHA_SENSOR: {
                obj.labelX = jsonObj["X"] | 0;
                obj.labelY = jsonObj["Y"] | 0;
                obj.x = jsonObj["IconX"] | obj.x;
                obj.y = jsonObj["IconY"] | obj.y;
                CopyStr(obj.entities, sizeof(obj.entities), jsonObj["entity"]);

                // Collect every state->command mapping: "state:on",
                // "state:open", "state:detected", "else", "state:", "*".
                obj.stateCount = 0;
                for (JsonPair kv : jsonObj) {
                    if (obj.stateCount >= MAX_STATE_MAPS) break;
                    const char* k = kv.key().c_str();
                    if (!k) continue;
                    bool isState = (strncmp(k, "state:", 6) == 0);
                    bool isElse  = (strcmp(k, "else") == 0);
                    bool isStar  = (strcmp(k, "*") == 0);
                    if (!isState && !isElse && !isStar) continue;

                    strlcpy(obj.stateKey[obj.stateCount], k, sizeof(obj.stateKey[0]));
                    if (kv.value().is<const char*>()) {
                        strlcpy(obj.stateCmd[obj.stateCount],
                                kv.value().as<const char*>(),
                                sizeof(obj.stateCmd[0]));
                    } else {
                        obj.stateCmd[obj.stateCount][0] = '\0';
                    }
                    obj.stateCount++;
                }
                break;
            }

            default:
                break;
        }

        logIf("[objects] #%d: type=%s x=%d y=%d",
              g_objectCount, t, obj.x, obj.y);
        g_objectCount++;
    }

    logIf("[objects] %d object(s).", g_objectCount);
}

#endif // OBJECTS_H
