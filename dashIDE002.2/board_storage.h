/**
 * @file board_storage.h
 *
 * Compile-time filesystem adapter used by configuration, layout loading and logs.
 * BoardSD aliases the correct Arduino filesystem object. The black board uses
 * one-bit SD_MMC with explicit pins; other profiles use SPI SD with their CS
 * pin. Mounting is initiated lazily through sdEnsureMount() in log.h.
 */

#pragma once
#include "board_config.h"
#if DASH_BLACK
#include <SD_MMC.h>
#define BoardSD SD_MMC
inline bool BoardMountSD() {
    SD_MMC.setPins(JC3248W535_SD_CLK, JC3248W535_SD_CMD, JC3248W535_SD_D0);
    return SD_MMC.begin("/sdcard", true); // one data line
}
#else
#include <SD.h>
#define BoardSD SD
inline bool BoardMountSD() { return SD.begin(CYD_SD_CS); }
#endif
