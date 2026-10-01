#ifndef TOUCH_H
#define TOUCH_H

// touch.h - Touch navigation (XPT2046) for the CYD 2.8" board.
//
// The touch controller XPT2046 runs on ITS OWN SPI bus (VSPI pins
// CLK25/MISO39/MOSI32, CS33, IRQ36) and DOES NOT share wires with
// the display (HSPI), so it never collides with drawing.
//
// Touch navigation is ALWAYS active (like the upstream Example):
// a tap on the LEFT half goes to the PREVIOUS screen and a tap on
// the RIGHT half goes to the NEXT one, through WebChangeScreen()
// (the same safe navigation port used by the web-UI, which already
// applies the engine lock and LoadScreen). See dash001.ino loop().
//
// "Type":"Touch" objects continue to be ignored when reading screens
// (objects.h) — this file only provides the physical left/right
// navigation described above.
//
// The touch rotation follows the CURRENT screen rotation (sc.rotation
// from the board JSON, 1 or 4 on this CYD 2.8"). The XPT2046 library
// understands rotations 0..3 only; screen rotations >= 4 (mirrored
// landscape - e.g. the Amarela/Pink board in Rotation 4) mirror the
// X axis so that a PHYSICAL left tap keeps meaning "previous", exactly
// as the user sees it. Rotation is board specific and is the JSON
// knob to tune per board.
//
// Raw ranges are the proven ones from "CYD_Dashboard_Working" for
// this same Amarela/Pink board (TS_MIN 300 .. TS_MAX 3900). Tune
// these if taps land off-centre.

#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

#include "log.h"
#include "web.h"

#define TOUCH_CS    33
#define TOUCH_IRQ   36
#define TOUCH_CLK   25
#define TOUCH_MISO  39
#define TOUCH_MOSI  32

#define TS_MINX  300
#define TS_MAXX  3900
#define TS_MINY  300
#define TS_MAXY  3900

#define TOUCH_W   320
#define TOUCH_H   240

#define TOUCH_DEBOUNCE_MS 400       // time gate between accepted taps

// Shared touch state (used by dash001.ino).
bool  g_touchOn   = false;   // read from board JSON root "Touch"
int   g_touchX    = -1;      // last valid tap X (pixel, 0..TOUCH_W-1)
int   g_touchY    = -1;      // last valid tap Y (pixel, 0..TOUCH_H-1)
unsigned long g_touchLastTap = 0;   // millis() of the last accepted tap

// The touch runs on its own SPI bus (VSPI); the display is on HSPI,
// so the two coexist without wire conflicts on the CYD.
SPIClass touchSPI(VSPI);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

// Initialises the touch controller. Call once from setup().
inline void TouchInit()
{
    touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    ts.begin(touchSPI);
    ts.setRotation(1);
    logIf("[touch] XPT2046 ready (CS=%d IRQ=%d).", TOUCH_CS, TOUCH_IRQ);
}

// Returns true and fills g_touchX/g_touchY (in 0..320 x 0..240 logical
// screen coordinates) when a valid tap arrives. rotation is the current
// screen rotation from the board JSON (sc.rotation, 1 or 4). Rotations
// >= 4 mirror the X axis so "physical left" maps to "previous screen".
inline bool CheckTouch(int rotation)
{
    // Mirror the upstream Example's proven XPT2046 read order:
    // IRQ + SPI agree the finger is down -> debounce -> read the
    // point. No rising-edge latch and no Z filter: the 400ms debounce
    // already gates the fire rate, and this is the exact logic that
    // works in the Example on this same CYD hardware.
    if (!ts.tirqTouched() || !ts.touched()) return false;

    // Debounce before the SPI read (no wasted transfer while blocked).
    unsigned long now = millis();
    if (now - g_touchLastTap < TOUCH_DEBOUNCE_MS) return false;
    g_touchLastTap = now;

    TS_Point p = ts.getPoint();

    // Scanning noise can return an empty sample even with the finger
    // down; discard it (same gate as the Example).
    if (p.x == 0 && p.y == 0) return false;

    int x = map(p.x, TS_MINX, TS_MAXX, 0, TOUCH_W);
    int y = map(p.y, TS_MINY, TS_MAXY, 0, TOUCH_H);
    x = constrain(x, 0, TOUCH_W - 1);
    y = constrain(y, 0, TOUCH_H - 1);

    if (rotation >= 4) {
        x = TOUCH_W - 1 - x;
    }

    g_touchX = x;
    g_touchY = y;
    logIf("[touch] raw(%d,%d,%d) -> px(%d,%d) rotation=%d.",
          p.x, p.y, p.z, x, y, rotation);
    return true;
}

#endif // TOUCH_H
