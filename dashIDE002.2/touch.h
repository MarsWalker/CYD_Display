/**
 * @file touch.h
 *
 * Compile-time selector for one touch implementation with a shared UI contract.
 * TouchInit prepares the selected controller. CheckTouch returns true only for
 * an accepted, debounced tap and stores logical pixel coordinates globally.
 * The JSON touch-enable flag controls navigation in loop(); it does not select
 * hardware or prevent controller initialization. Contact edges are filtered once
 * per press. The sketch dispatches object rectangles before midpoint navigation.
 */

#pragma once
#include "ha_button_model.h"
#include "board_config.h"
#if defined(CYD_TOUCH_AXS15231B)
#include "board_display.h"
#include "log.h"

bool g_touchOn = false;
int g_touchX = -1, g_touchY = -1;
static TouchPressGate g_touchGate;
static bool g_capacitiveReady = false;

/**
 * Initialize the selected touch controller once during boot.
 * Failure is reported in logs/readiness state; navigation then receives no taps.
 */
inline void TouchInit() {
    g_capacitiveReady = BlackTouchInit();
    logIf("[touch] AXS15231B capacitive I2C ready=%d", g_capacitiveReady);
}
/**
 * Poll and validate a single contact, then map it to the active screen.
 * The accepted-tap timestamp is advanced only after a valid coordinate read.
 * Return false for debounce suppression, no contact, invalid data or bus failure.
 */
inline bool CheckTouch(int rotation) {
    if (!g_capacitiveReady) return false;
    const uint32_t now = millis();

    uint16_t x, y;
    if (!BlackTouchRead(&x, &y)) { if (BlackTouchReleased()) g_touchGate.sample(false,now); return false; }
    if (x>=320 || y>=480) return false;
    BlackNativeToLogical(x, y, rotation, g_touchX, g_touchY);
    if (!g_touchGate.sample(true,now)) return false;
    logIf("[touch] raw(%u,%u) -> px(%d,%d) rotation=%d", x, y, g_touchX, g_touchY, rotation);
    return true;
}
inline int TouchWidth() { return tft.width(); }
#elif defined(CYD_TOUCH_FT6X36)
#include "touch_pink.h"
#elif defined(CYD_TOUCH_CST820)
#include "touch_cst820.h"
#else
#include "touch_xpt2046.h"
inline int TouchWidth() { return tft.width(); }
#endif
