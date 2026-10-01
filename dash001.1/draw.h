#ifndef DRAW_H
#define DRAW_H

#include <TFT_eSPI.h>

#include "log.h"
#include "colors.h"
#include "images.h"
#include "objects.h"
#include "functions.h"
#include "datacache.h"
#include "datasources.h"

/*
 * draw.h
 * -------------------------------------------------------
 * The draw engine: walks g_objects[] (the objects of the
 * currently loaded screen) and renders each one on the TFT.
 *
 * DrawObjects(firstDraw):
 *   - firstDraw == true  -> full redraw (also applies FillScreen);
 *   - firstDraw == false -> incremental update, only re-draws the
 *     objects whose visible content actually changed.
 * Text objects are re-rendered only when their visible value changes,
 * except function:Clock, which is refreshed on every draw cycle.
 * -------------------------------------------------------
 */

uint16_t g_bgColor = TFT_BLACK;

// ---------------------------------------------------------------
// Small image helpers: size lookup + 1-bit bitmap draw (from images.h)
// ---------------------------------------------------------------
inline int ImageSize(const char* name)
{
    int i = FindImageIndex(name);
    return (i >= 0) ? IMAGE_TABLE[i].width : 24;
}

inline void DrawImageAt(int x, int y, const char* name, uint16_t color, int zoom)
{
    int i = FindImageIndex(name);
    if (i < 0) return;

    const unsigned char* bmp = IMAGE_TABLE[i].data;
    int w = IMAGE_TABLE[i].width;
    int h = IMAGE_TABLE[i].height;

    if (zoom <= 1) {
        tft.drawBitmap(x, y, bmp, w, h, color);
        return;
    }
    for (int py = 0; py < h; py++) {
        for (int px = 0; px < w; px++) {
            int idx = py * w + px;
            if (bmp[idx >> 3] & (0x80 >> (idx & 7))) {
                tft.fillRect(x + px * zoom, y + py * zoom, zoom, zoom, color);
            }
        }
    }
}

// ---------------------------------------------------------------
// Object renderers
// ---------------------------------------------------------------

inline void DrawFillScreen(ObjectInfo& obj, bool firstDraw)
{
    if (!firstDraw) return;   // background only cleared on a full redraw
    g_bgColor = GetColorFromName(obj.color);
    tft.fillScreen(g_bgColor);
}

inline void DrawTextObject(ObjectInfo& obj, bool firstDraw)
{
    String txt  = ResolveTextValue(String(obj.text));
    String full = String(obj.prefix) + txt + String(obj.suffix);
    String source(obj.text);
    source.trim();
    bool isClock = source.equalsIgnoreCase("function:Clock");
    uint32_t textHash = 2166136261UL;
    for (size_t i = 0; i < full.length(); i++) {
        textHash ^= (uint8_t)full[i];
        textHash *= 16777619UL;
    }

    if (!firstDraw && !isClock && obj.hasDrawnText &&
        obj.lastTextHash == textHash) {
        return;
    }

    tft.setTextColor(GetColorFromName(obj.color), g_bgColor);
    tft.setTextFont(obj.font);
    tft.setTextDatum(TL_DATUM);

    int textWidth = tft.textWidth(full) + 1;
    int clearWidth = textWidth > obj.lastTextWidth ? textWidth : obj.lastTextWidth;
    tft.fillRect(obj.x, obj.y, clearWidth, tft.fontHeight(), g_bgColor);

    tft.setCursor(obj.x, obj.y);
    tft.print(full);

    obj.lastTextHash = textHash;
    obj.lastTextWidth = textWidth;
    obj.hasDrawnText = true;
}

inline void DrawImageObject(ObjectInfo& obj, bool firstDraw)
{
    // The image name may be a datasource reference
    // (e.g. "datasource:weather_aveiro:code_image").
    String name = ResolveTextValue(String(obj.image));
    name.trim();

    if (!firstDraw && strcmp(obj.lastState, name.c_str()) == 0) return;
    strlcpy(obj.lastState, name.c_str(), sizeof(obj.lastState));

    int zoom = (obj.zoom > 0) ? obj.zoom : 1;
    int w = ImageSize(name.c_str()) * zoom;
    int h = w;
    tft.fillRect(obj.x, obj.y, w, h, g_bgColor);

    int i = FindImageIndex(name.c_str());
    if (i < 0) {
        logWf(
            "[draw] image not found: source='%s' resolved='%s' x=%d y=%d",
            obj.image,
            name.c_str(),
            obj.x,
            obj.y
        );
        return;
    }
    uint16_t color = obj.color[0] ? GetColorFromName(obj.color)
                                 : IMAGE_TABLE[i].defaultColor;
    DrawImageAt(obj.x, obj.y, name.c_str(), color, zoom);
}

inline void DrawImageBoolObject(ObjectInfo& obj, bool firstDraw)
{
    bool on = ResolveBoolValue(String(obj.boolValue));
    const char* cmd = on ? obj.imageTrue : obj.imageFalse;

    const char* imageName = "";
    if (cmd && strncmp(cmd, "DrawImage:", 10) == 0) imageName = cmd + 10;

    if (!firstDraw && strcmp(obj.lastState, imageName) == 0) return;
    strlcpy(obj.lastState, imageName, sizeof(obj.lastState));

    int w = ImageSize(imageName);
    tft.fillRect(obj.x, obj.y, w, w, g_bgColor);

    int i = FindImageIndex(imageName);
    if (i >= 0) {
        DrawImageAt(obj.x, obj.y, imageName, IMAGE_TABLE[i].defaultColor, 1);
    } else if (imageName[0]) {
        logWf(
            "[draw] image not found: bool='%s' cmd='%s' image='%s' x=%d y=%d",
            obj.boolValue,
            cmd ? cmd : "",
            imageName,
            obj.x,
            obj.y
        );
    }
}

// Computes the combined entity state and the winning image command
// of a "DrawHASensor" object. "combined" is the list of
// entity states joined by '|' (used for change detection); "cmd" is
// the DrawImage:... command of the first matching state rule ("else"
// as fallback, "DrawImage:warning" as last resort).
inline void ComputeHASensorImage(ObjectInfo& obj, String& combined, const char*& cmd)
{
    char buf[sizeof(obj.entities)];
    strlcpy(buf, obj.entities, sizeof(buf));

    combined = "";
    cmd = nullptr;

    char* tok = strtok(buf, ",");
    while (tok) {
        while (*tok == ' ') tok++;
        if (tok[0]) {
            const char* st = EntityStateGet(tok);
            combined += st;
            combined += "|";

            if (cmd == nullptr) {
                char key[32];
                snprintf(key, sizeof(key), "state:%s", st);
                for (int k = 0; k < obj.stateCount; k++) {
                    if (strcmp(obj.stateKey[k], key) == 0 ||
                        strcmp(obj.stateKey[k], "*") == 0) {
                        cmd = obj.stateCmd[k];
                        break;
                    }
                }
            }
        }
        tok = strtok(nullptr, ",");
    }

    if (cmd == nullptr) {
        for (int k = 0; k < obj.stateCount; k++) {
            if (strcmp(obj.stateKey[k], "else") == 0) {
                cmd = obj.stateCmd[k];
                break;
            }
        }
    }
    if (cmd == nullptr || cmd[0] == '\0') cmd = "DrawImage:warning";
}

inline void DrawHASensorObject(ObjectInfo& obj, bool firstDraw)
{
    String combined;
    const char* cmd = nullptr;
    ComputeHASensorImage(obj, combined, cmd);

    const char* imageName = "";
    if (cmd && strncmp(cmd, "DrawImage:", 10) == 0) imageName = cmd + 10;

    if (!firstDraw && strcmp(obj.lastState, combined.c_str()) == 0) return;
    strlcpy(obj.lastState, combined.c_str(), sizeof(obj.lastState));

    int w = ImageSize(imageName);
    tft.fillRect(obj.x, obj.y, w, w, g_bgColor);

    int i = FindImageIndex(imageName);
    if (i >= 0) {
        DrawImageAt(obj.x, obj.y, imageName, IMAGE_TABLE[i].defaultColor, 1);
    } else if (imageName[0]) {
        logWf(
            "[draw] image not found: multi='%s' cmd='%s' image='%s' x=%d y=%d",
            combined.c_str(),
            cmd ? cmd : "",
            imageName,
            obj.x,
            obj.y
        );
    }

    // Text label next to the icon.
    if (obj.text[0]) {
        tft.setTextColor(GetColorFromName(obj.color), g_bgColor);
        tft.setTextFont(obj.font);
        tft.setTextDatum(TL_DATUM);
        tft.setCursor(obj.labelX, obj.labelY);
        tft.print(obj.text);
    }
}

// ---------------------------------------------------------------
// Main entry: draws every object of the current screen.
// ---------------------------------------------------------------
inline void DrawObjects(bool firstDraw)
{
    for (int i = 0; i < g_objectCount; i++) {
        ObjectInfo& obj = g_objects[i];
        switch (obj.type) {
            case OBJ_FILLSCREEN:    DrawFillScreen(obj, firstDraw);     break;
            case OBJ_DRAWTEXT:      DrawTextObject(obj, firstDraw);      break;
            case OBJ_DRAWIMAGE:     DrawImageObject(obj, firstDraw);     break;
            case OBJ_DRAWIMAGEBOOL: DrawImageBoolObject(obj, firstDraw); break;
            case OBJ_DRAWHA_SENSOR: DrawHASensorObject(obj, firstDraw);  break;
            default: break;   // OBJ_TOUCH is ignored at parse time
        }
    }
}

#endif // DRAW_H
