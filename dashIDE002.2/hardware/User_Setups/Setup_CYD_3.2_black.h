/**
 * @file Setup_CYD_3.2_black.h
 *
 * JC3248W535 ESP32-S3 QSPI profile. Display pins belong to the external
 * AXS15231B transport; TFT_eSPI must not initialize this hardware itself.
 * Includes native geometry, touch, PWM and one-bit SD_MMC wiring.
 */

#pragma once

// JC3248W535 / JC3248W535EN - ESP32-S3, 3.5 inch IPS, 320x480.
// Filename retained as requested; this board has a 3.5 inch panel.
// Source: manufacturer JC3248W535EN.zip, DEMO_LVGL/esp_bsp.h,
// display.h and DEMO_PIC/pincfg.h.
//
// Shared configuration for dashboard and test_black. TFT_eSPI does not implement
// the AXS15231B controller or its required QSPI transport.
// Selecting this header cannot add that support. Use the manufacturer
// esp_lcd AXS15231B driver (with or without LVGL) when adapting the project.
// Do not substitute ILI9341/ST7796 or ordinary SPI pin definitions.

#define USER_SETUP_INFO "JC3248W535 black - AXS15231B QSPI external backend"
#define CYD_TOUCH_AXS15231B 1
#define CYD_NATIVE_WIDTH 320
#define CYD_NATIVE_HEIGHT 480
#define CYD_NORMAL_INVERT 0
#define CYD_TOUCH_SDA 4
#define CYD_TOUCH_SCL 8
#define CYD_TOUCH_ADDRESS 0x3B
#define CYD_TOUCH_I2C_HZ 400000
#define CYD_BL_PIN 1
#define CYD_BL_ACTIVE_HIGH 1
#define CYD_BL_PWM_HZ 5000
#define CYD_BL_PWM_BITS 10

// Board-specific names below are reference constants, not TFT_eSPI options.
#define JC3248W535_LCD_WIDTH       320
#define JC3248W535_LCD_HEIGHT      480
#define JC3248W535_LCD_CS          45
#define JC3248W535_LCD_CLK         47
#define JC3248W535_LCD_DATA0       21
#define JC3248W535_LCD_DATA1       48
#define JC3248W535_LCD_DATA2       40
#define JC3248W535_LCD_DATA3       39
#define JC3248W535_LCD_RST         -1
#define JC3248W535_LCD_TE          38
#define JC3248W535_LCD_BL          1
#define JC3248W535_LCD_BL_ON       1

// Capacitive AXS15231B touch over I2C; not an XPT2046 SPI touchscreen.
#define JC3248W535_TOUCH_SDA       4
#define JC3248W535_TOUCH_SCL       8
#define JC3248W535_TOUCH_RST       -1
// DEMO_PIC assigns INT=3; DEMO_LVGL uses polling with INT=-1.
#define JC3248W535_TOUCH_INT       -1 // validated polling, no IRQ

// SD card uses SD_MMC, not the SPI SD wiring of the 2.8 inch CYD.
#define JC3248W535_SD_CMD          11
#define JC3248W535_SD_CLK          12
#define JC3248W535_SD_D0           13

// Stop compilation if selected, rather than silently using an invalid driver.
#ifndef CYD_EXTERNAL_AXS15231B
#error "Select this setup through the dashboard/test_black AXS15231B QSPI backend. TFT_eSPI alone cannot drive this panel."
#endif
