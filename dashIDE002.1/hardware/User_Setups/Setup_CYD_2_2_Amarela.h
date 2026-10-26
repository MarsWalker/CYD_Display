/**
 * @file Setup_CYD_2_2_Amarela.h
 *
 * Parallel ST7789 profile: eight data lines plus WR/RD/DC/CS, CST820 on I2C,
 * GPIO0 backlight and SPI SD. Constants feed both the dashboard adapter and
 * LovyanGFX setup. GPIO32/33 are data pins here, not an interchangeable SPI map.
 */

/* 
  Rui Santos & Sara Santos - Random Nerd Tutorials

  Install the "TFT_eSPI" lbirary by Bodmer to interface with the TFT Display - https://github.com/Bodmer/TFT_eSPI
  *** IMPORTANT: User_Setup.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials ***
  *** YOU MUST USE THIS User_Setup.h FILE, CHECK FOR THE LATEST VERSION IN THE LINK BELOW IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS ***
  https://RandomNerdTutorials.com/cyd/
  https://RandomNerdTutorials.com/esp32-tft/
  FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd/ or https://RandomNerdTutorials.com/esp32-tft/
*/
#pragma once
// ESP32-2432S022C, amarela2.2. Manufacturer specification and LCM/MCU schematics
// from the board documentation. Colors, touch and brightness validated
// by the user via test_cyd2.2 on 2026-10-24.
#define USER_SETUP_INFO "CYD amarela2.2 / ST7789 parallel / CST820"
#define ST7789_DRIVER
#define TFT_PARALLEL_8_BIT
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_RGB_ORDER TFT_RGB
#define TFT_CS 17
#define TFT_DC 16
#define TFT_RST -1
#define TFT_WR 4
#define TFT_RD 2
#define TFT_D0 15
#define TFT_D1 13
#define TFT_D2 12
#define TFT_D3 14
#define TFT_D4 27
#define TFT_D5 25
#define TFT_D6 33
#define TFT_D7 32
#define TFT_BL 0
#define TFT_BACKLIGHT_ON HIGH
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// Shared settings for the independent test and future dashboard profile.
#define CYD_NATIVE_WIDTH 240
#define CYD_NATIVE_HEIGHT 320
#define CYD_LCD_PARALLEL_HZ 25000000
#define CYD_LCD_RGB_ORDER_BGR 0
#define CYD_NORMAL_INVERT 0
#define CYD_TOUCH_CST820 1
#define CYD_TOUCH_SDA 21
#define CYD_TOUCH_SCL 22
#define CYD_TOUCH_ADDRESS 0x15
#define CYD_TOUCH_I2C_HZ 100000
#define CYD_TOUCH_TIMEOUT_MS 20
#define CYD_TOUCH_IRQ -1
#define CYD_BL_PIN 0
#define CYD_BL_ACTIVE_HIGH 1
#define CYD_BL_PWM_HZ 5000
#define CYD_BL_PWM_BITS 8
#define CYD_SD_CS 5
// SD bus from MCU schematic: SCK18/MOSI23/MISO19; not tested here.
