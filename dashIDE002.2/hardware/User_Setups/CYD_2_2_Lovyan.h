/**
 * @file CYD_2_2_Lovyan.h
 *
 * LovyanGFX device adapter for the eight-bit parallel ST7789 profile.
 * The bus and panel objects are members so they outlive all references installed
 * in LGFX_Device. Pin values are supplied by Setup_CYD_2_2_Amarela.h.
 */

#pragma once
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// Manufacturer Factory_samples.ino bus/panel configuration, with all board
// values supplied by Setup_CYD_2_2_Amarela.h. Supports data GPIO32/33.
class Cyd22Display : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 panel;
  lgfx::Bus_Parallel8 bus;
public:
  Cyd22Display() {
    auto wiring = bus.config();
    wiring.freq_write = CYD_LCD_PARALLEL_HZ;
    wiring.pin_wr = TFT_WR;
    wiring.pin_rd = TFT_RD;
    wiring.pin_rs = TFT_DC;
    wiring.pin_d0 = TFT_D0;
    wiring.pin_d1 = TFT_D1;
    wiring.pin_d2 = TFT_D2;
    wiring.pin_d3 = TFT_D3;
    wiring.pin_d4 = TFT_D4;
    wiring.pin_d5 = TFT_D5;
    wiring.pin_d6 = TFT_D6;
    wiring.pin_d7 = TFT_D7;
    bus.config(wiring);
    panel.setBus(&bus);
    auto config = panel.config();
    config.pin_cs = TFT_CS;
    config.pin_rst = TFT_RST;
    config.pin_busy = -1;
    config.panel_width = CYD_NATIVE_WIDTH;
    config.panel_height = CYD_NATIVE_HEIGHT;
    config.offset_x = config.offset_y = config.offset_rotation = 0;
    config.readable = false;
    config.invert = CYD_NORMAL_INVERT;
    config.rgb_order = CYD_LCD_RGB_ORDER_BGR;
    config.dlen_16bit = false;
    config.bus_shared = true;
    panel.config(config);
    setPanel(&panel);
  }
};
