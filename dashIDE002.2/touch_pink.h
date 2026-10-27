/**
 * @file touch_pink.h
 *
 * FT6x36-compatible capacitive polling for the pink board profile.
 * This is not the resistive XPT2046 protocol, despite similar LCD wiring.
 * The controller supplies native pixel coordinates, so no ADC calibration is
 * required. I2C reads are bounded and only one contact is used for navigation.
 * Rotation is applied after controller coordinates have been bounds-checked.
 */

#pragma once
#include "ha_button_model.h"
#include <Wire.h>
#include "board_display.h"
#include "board_geometry.h"
#include "log.h"

// Capacitive protocol validated for the pink hardware profile.
// FT6x36-compatible capacitive protocol; no IRQ or pressure calibration.
bool g_touchOn = false;
int g_touchX = -1, g_touchY = -1;
static bool g_pinkTouchReady = false;
static TouchPressGate g_pinkGate;

/**
 * Initialize the selected touch controller once during boot.
 * Failure is reported in logs/readiness state; navigation then receives no taps.
 */
inline void TouchInit() {
    Wire.begin(CYD_TOUCH_SDA, CYD_TOUCH_SCL, CYD_TOUCH_I2C_HZ);
    Wire.setTimeOut(CYD_TOUCH_TIMEOUT_MS);
    Wire.beginTransmission(CYD_TOUCH_ADDRESS);
    g_pinkTouchReady = Wire.endTransmission() == 0;
    logIf("[touch] capacitive SDA=%d SCL=%d address=0x%02X ready=%d", CYD_TOUCH_SDA, CYD_TOUCH_SCL, CYD_TOUCH_ADDRESS, g_pinkTouchReady);
}

/**
 * Poll and validate a single contact, then map it to the active screen.
 * The accepted-tap timestamp is advanced only after a valid coordinate read.
 * Return false for debounce suppression, no contact, invalid data or bus failure.
 */
inline bool CheckTouch(int rotation) {
    if (!g_pinkTouchReady) return false;
    const uint32_t now = millis();

    Wire.beginTransmission(CYD_TOUCH_ADDRESS);
    /**
     * Start at the touch-count register. Repeated-start keeps the following
     * five-byte coordinate read in the same I2C transaction sequence.
     */
    Wire.write(0x02);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((uint8_t)CYD_TOUCH_ADDRESS, (uint8_t)5) != 5) return false;
    uint8_t data[5];
    for (int i = 0; i < 5; ++i) data[i] = Wire.read();
    if ((data[0] & 0x0f) == 0) { g_pinkGate.sample(false,now); return false; }
    const int x = ((data[1] & 0x0f) << 8) | data[2];
    const int y = ((data[3] & 0x0f) << 8) | data[4];
    if (x >= CYD_NATIVE_WIDTH || y >= CYD_NATIVE_HEIGHT) return false;
    PinkNativeToLogical(x, y, rotation, g_touchX, g_touchY);
    if (!g_pinkGate.sample(true,now)) return false;
    logIf("[touch] raw(%d,%d) -> px(%d,%d) rotation=%d", x, y, g_touchX, g_touchY, rotation);
    return true;
}
inline int TouchWidth() { return tft.width(); }
