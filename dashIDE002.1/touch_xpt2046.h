/**
 * @file touch_xpt2046.h
 *
 * Resistive-touch adapter for the standard yellow ILI9341 board.
 * A separate VSPI bus polls XPT2046 while display traffic uses the configured
 * TFT bus. Calibration maps raw ADC ranges to native portrait pixels before the
 * shared rotation/mirror transform. Rejecting 0/4095 avoids obvious invalid ADC
 * endpoints. Accepted taps are throttled for 400 ms; a held finger may repeat.
 */

#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "board_display.h"
#include "board_geometry.h"
#include "log.h"

// Same portrait polling and calibration as the validated test_cyd2.8.
// Then transform coordinates into the screen's current ILI9341 rotation.
bool g_touchOn = false;
int g_touchX = -1, g_touchY = -1;
static uint32_t g_touchLastTap = 0;
SPIClass touchSPI(VSPI);
XPT2046_Touchscreen ts(CYD_TOUCH_CS);

/**
 * Initialize the selected touch controller once during boot.
 * Failure is reported in logs/readiness state; navigation then receives no taps.
 */
inline void TouchInit() {
    touchSPI.begin(CYD_TOUCH_CLK, CYD_TOUCH_MISO, CYD_TOUCH_MOSI, CYD_TOUCH_CS);
    ts.begin(touchSPI);
    ts.setRotation(CYD_TOUCH_ROTATION);
    logIf("[touch] XPT2046 polling CS=%d CLK=%d MISO=%d MOSI=%d", CYD_TOUCH_CS, CYD_TOUCH_CLK, CYD_TOUCH_MISO, CYD_TOUCH_MOSI);
}
/**
 * Poll and validate a single contact, then map it to the active screen.
 * The accepted-tap timestamp is advanced only after a valid coordinate read.
 * Return false for debounce suppression, no contact, invalid data or bus failure.
 */
inline bool CheckTouch(int rotation) {
    const uint32_t now = millis();
    if (now - g_touchLastTap < 400 || !ts.touched()) return false;
    TS_Point p = ts.getPoint();
    if (p.x <= 0 || p.x >= 4095 || p.y <= 0 || p.y >= 4095) return false;
    /**
     * Calibrate in native portrait space first, then rotate once. Calibrating
     * already-rotated coordinates would make endpoint ranges depend on the screen.
     */
    int x = constrain(map(p.x, CYD_TOUCH_RAW_MIN_X, CYD_TOUCH_RAW_MAX_X, 0, CYD_NATIVE_WIDTH - 1), 0L, (long)CYD_NATIVE_WIDTH - 1);
    int y = constrain(map(p.y, CYD_TOUCH_RAW_MIN_Y, CYD_TOUCH_RAW_MAX_Y, 0, CYD_NATIVE_HEIGHT - 1), 0L, (long)CYD_NATIVE_HEIGHT - 1);
    PinkNativeToLogical(x, y, rotation, g_touchX, g_touchY);
    g_touchLastTap = now;
    logIf("[touch] raw(%d,%d,%d) -> px(%d,%d) rotation=%d", p.x, p.y, p.z, g_touchX, g_touchY, rotation);
    return true;
}
