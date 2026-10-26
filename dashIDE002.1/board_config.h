/**
 * @file board_config.h
 *
 * Compile-time hardware identity shared by C and C++ translation units.
 * CLI builds may pass DASH_BOARD; IDE builds use selected_board.h. Both map
 * the same numeric profile to a name
 * and the DASH_BLACK feature switch. Selection happens before runtime JSON is
 * loaded: changing a layout cannot switch a physical display or touch driver.
 * IDs must stay synchronized with build.ps1 and build.sh.
 */

#pragma once

// Numeric IDs and legacy internal macro names stay stable for existing code.
// Public names and CLI aliases do not change pin assignments or backends.
#define DASH_CYD_AMARELA_2_8 1
#define DASH_CYD_PINK 2
#define DASH_JC3248_BLACK 3
#define DASH_CYD_AMARELA_2_2 4
#ifndef DASH_BOARD
#include "selected_board.h"
// The response file changes with the selection so the IDE invalidates caches.
#if defined(DASH_IDE_PROFILE) && DASH_IDE_PROFILE != DASH_SELECTED_BOARD
#error "IDE selection files disagree. Run select_board again and reopen the sketch."
#endif
#define DASH_BOARD DASH_SELECTED_BOARD
#endif
#if DASH_BOARD == DASH_JC3248_BLACK
#define DASH_BLACK 1
#define DASH_BOARD_NAME "esp32s3_axs15231b_qspi"
#else
#define DASH_BLACK 0
#if DASH_BOARD == DASH_CYD_AMARELA_2_8
#define DASH_BOARD_NAME "esp32_ili9341_xpt2046"
#elif DASH_BOARD == DASH_CYD_AMARELA_2_2
#define DASH_BOARD_NAME "esp32_st7789_parallel_cst820"
#elif DASH_BOARD == DASH_CYD_PINK
#define DASH_BOARD_NAME "esp32_ili9341_cap_touch"
#else
#error "Unknown DASH_BOARD"
#endif
#endif
