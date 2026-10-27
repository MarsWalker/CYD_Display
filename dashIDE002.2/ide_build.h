/**
 * @file ide_build.h
 * ESP32 core build_opt.h force-includes this bridge in the sketch, libraries
 * and core translation units. Unlike sketch-only #defines, these options also
 * reach TFT_eSPI.cpp. No library-global configuration file is modified.
 *
 * The response file also applies to C and preprocessed assembly: skip C++ TFT
 * configuration outside C++, and skip all declarations for assembly sources.
 * The 3.3.7 platform adds the original sketch root to every include search path,
 * so relative names remain valid after moving the whole project directory.
 */
#pragma once
#ifndef __ASSEMBLER__
#include "board_config.h"
#include <sdkconfig.h>
#if DASH_BLACK
#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "Black requires Tools > Board > ESP32S3 Dev Module."
#endif
#if !defined(BOARD_HAS_PSRAM)
#error "Black requires Tools > PSRAM > OPI PSRAM."
#endif
#else
#if !defined(CONFIG_IDF_TARGET_ESP32)
#error "This profile requires Tools > Board > ESP32 Dev Module."
#endif
#endif
#ifdef __cplusplus
#include "tft_setup.h"
#endif
#endif
