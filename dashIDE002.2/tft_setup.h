/**
 * @file tft_setup.h
 *
 * Shared TFT profile loaded by ide_build.h through the core build_opt.h hook.
 * Command-line builds may also force-include this file directly.
 * USER_SETUP_LOADED bypasses TFT_eSPI's global setup selector so the correct
 * pins, touch protocol and color polarity travel together with DASH_BOARD.
 * For black, TFT_eSPI is a software rasterizer only; the separate esp_lcd driver
 * owns the physical QSPI interface. For the parallel yellow board, LovyanGFX
 * consumes the same profile macros through its dedicated display adapter.
 */

#pragma once
#include "board_config.h"

// Forced into ALL C++ compilation units, including TFT_eSPI itself.
// This avoids changing the shared library's User_Setup_Select.h.
#define USER_SETUP_LOADED
#if DASH_BLACK
#define CYD_EXTERNAL_AXS15231B 1
#include "hardware/User_Setups/Setup_CYD_3.2_black.h"
// TFT_eSPI is only a font/bitmap rasterizer in PSRAM on the black board.
// Never call TFT_eSPI::init() or pushSprite(): esp_lcd owns the QSPI bus.
#include "hardware/User_Setups/Setup0_Sprite.h"
#elif DASH_BOARD == DASH_CYD_AMARELA_2_8
#include "hardware/User_Setups/Setup_CYD_2_8_Amarela.h"
#elif DASH_BOARD == DASH_CYD_AMARELA_2_2
#include "hardware/User_Setups/Setup_CYD_2_2_Amarela.h"
#else
#include "hardware/User_Setups/Setup_CYD_2_8_Pink.h"
#endif
