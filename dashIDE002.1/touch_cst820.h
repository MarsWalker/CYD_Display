/**
 * @file touch_cst820.h
 *
 * CST820 capacitive-touch polling for the parallel ST7789 board.
 * This profile deliberately uses the legacy IDF I2C API rather than Wire, whose
 * new driver conflicts with the selected LovyanGFX backend. Initialization
 * writes the controller-specific no-auto-sleep command. All transactions have
 * a finite timeout so absent touch hardware cannot stall navigation forever.
 */

#pragma once
#include <driver/i2c.h>
#include "board_display.h"
#include "board_geometry.h"
#include "log.h"

// Same legacy I2C driver and CST820 register protocol as test_cyd2.2.
// Do not include Wire: its new I2C driver conflicts with LovyanGFX 1.2.0.
bool g_touchOn = false;
int g_touchX = -1, g_touchY = -1;
static bool g_cstReady = false;
static uint32_t g_cstLastTap = 0;

/**
 * Initialize the selected touch controller once during boot.
 * Failure is reported in logs/readiness state; navigation then receives no taps.
 */
inline void TouchInit() {
    i2c_config_t config = {};
    config.mode = I2C_MODE_MASTER;
    config.sda_io_num = CYD_TOUCH_SDA;
    config.scl_io_num = CYD_TOUCH_SCL;
    config.sda_pullup_en = config.scl_pullup_en = GPIO_PULLUP_ENABLE;
    config.master.clk_speed = CYD_TOUCH_I2C_HZ;
    g_cstReady = i2c_param_config(I2C_NUM_0, &config) == ESP_OK &&
        i2c_driver_install(I2C_NUM_0, config.mode, 0, 0, 0) == ESP_OK;
    if (g_cstReady) {
        /**
         * Controller-specific register write disabling automatic sleep.
         * Without it, the first touch after inactivity can be missed by polling.
         */
        const uint8_t command[] = {0xFE, 0xFF};
        g_cstReady = i2c_master_write_to_device(I2C_NUM_0, CYD_TOUCH_ADDRESS, command, sizeof(command), pdMS_TO_TICKS(CYD_TOUCH_TIMEOUT_MS)) == ESP_OK;
    }
    logIf("[touch] CST820 SDA=%d SCL=%d address=0x%02X ready=%d", CYD_TOUCH_SDA, CYD_TOUCH_SCL, CYD_TOUCH_ADDRESS, g_cstReady);
}

/**
 * Poll and validate a single contact, then map it to the active screen.
 * The accepted-tap timestamp is advanced only after a valid coordinate read.
 * Return false for debounce suppression, no contact, invalid data or bus failure.
 */
inline bool CheckTouch(int rotation) {
    if (!g_cstReady) return false;
    const uint32_t now = millis();
    if (now - g_cstLastTap < 400) return false;
    /**
     * Read touch count plus packed 12-bit X/Y coordinates in one bounded transfer.
     * The low nibble contains the count/high coordinate bits; status bits are masked.
     */
    const uint8_t reg = 0x02;
    uint8_t data[5];
    if (i2c_master_write_read_device(I2C_NUM_0, CYD_TOUCH_ADDRESS, &reg, 1, data, sizeof(data), pdMS_TO_TICKS(CYD_TOUCH_TIMEOUT_MS)) != ESP_OK) return false;
    if ((data[0] & 0x0f) == 0) return false;
    const int x = ((data[1] & 0x0f) << 8) | data[2];
    const int y = ((data[3] & 0x0f) << 8) | data[4];
    if (x >= CYD_NATIVE_WIDTH || y >= CYD_NATIVE_HEIGHT) return false;
    Cst820NativeToLogical(x, y, rotation, g_touchX, g_touchY);
    g_cstLastTap = now;
    logIf("[touch] raw(%d,%d) -> px(%d,%d) rotation=%d", x, y, g_touchX, g_touchY, rotation);
    return true;
}
inline int TouchWidth() { return tft.width(); }
