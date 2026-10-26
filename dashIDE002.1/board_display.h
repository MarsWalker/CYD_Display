/**
 * @file board_display.h
 *
 * Common drawing facade for three different physical display backends.
 * The black panel uses a PSRAM sprite and an explicit present() transfer; the
 * other boards draw directly and implement present() as a no-op. Callers use
 * this shared surface instead of invoking backend-specific initialization.
 * The global tft object is owned by the sketch. After startup, all display
 * operations must be serialized by the engine or run on the UI side.
 */

#pragma once
#include "board_config.h"
// Load Arduino's global File/FS aliases before TFT_eSPI's smooth-font
// support defines FS_NO_GLOBALS (WebServer and the existing sketch use them).
#include <FS.h>
#if DASH_BOARD == DASH_CYD_AMARELA_2_2
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "hardware/User_Setups/CYD_2_2_Lovyan.h"
#else
#include <TFT_eSPI.h>
#endif
#include "board_brightness.h"

#if DASH_BLACK
#include "black_panel.h"
#include "board_geometry.h"
#include <esp_heap_caps.h>

// Keep TFT_eSPI's exact fonts, text metrics and icon rendering in a sprite.
// The physical panel is handled exclusively by the manufacturer's esp_lcd driver.
class BoardDisplay : public TFT_eSprite {
    TFT_eSPI rasterizer;
    /**
     * DMA-capable native-width strip. The full sprite can reside in PSRAM,
     * but this reusable staging buffer must be accessible to the SPI DMA engine.
     */
    uint16_t *transfer = nullptr;
    uint8_t screenRotation = 0;
public:
    BoardDisplay() : TFT_eSprite(&rasterizer) {}
    void init() {
        if (!psramFound()) {
            Serial.println("[display] OPI PSRAM required");
            while (true) delay(1000);
        }
        BlackPanelInit();
        setBrightness(100);
        transfer = (uint16_t*)heap_caps_malloc(320 * 16 * 2, MALLOC_CAP_DMA);
        if (!transfer) abort();
        setColorDepth(16);
        setAttribute(PSRAM_ENABLE, true);
        setRotation(0);
    }
    void setRotation(uint8_t rotation) {
        screenRotation = rotation & 7;
        const int w = (rotation & 1) ? 480 : 320;
        const int h = (rotation & 1) ? 320 : 480;
        if (width() != w || height() != h || !getPointer()) {
            deleteSprite();
            if (!createSprite(w, h)) abort();
        }
        fillSprite(TFT_BLACK);
    }
    // Existing CYD layouts use Invert=true for normal colors. The AXS15231B
    // panel has the opposite polarity; normalize it in this board's backend.
    void invertDisplay(bool invert) { BlackPanelInvert(invert ? CYD_NORMAL_INVERT : !CYD_NORMAL_INVERT); }
    void setBrightness(unsigned percent) { BoardBrightness(percent); }
    /**
     * Convert logical sprite positions into native panel scan order one strip at
     * a time. BlackPanelWriteRows waits for each strip before transfer is reused.
     */
    void present() {
        const auto *pixels = (const uint16_t*)getPointer();
        if (!pixels || !transfer) return;
        // Send full native-width strips in sequence (AXS15231B RAMWR/RAMWRC).
        // Sprite RGB565 is already stored with the byte order expected by QSPI.
        for (int row = 0; row < 480; row += 16) {
            for (int dy = 0; dy < 16; ++dy) {
                for (int px = 0; px < 320; ++px) {
                    int x, y;
                    BlackNativeToLogical(px, row + dy, screenRotation, x, y);
                    transfer[dy * 320 + px] = pixels[y * width() + x];
                }
            }
            BlackPanelWriteRows(row, 16, transfer);
        }
    }
};
#elif DASH_BOARD == DASH_CYD_AMARELA_2_2
// Same physical backend as the validated test, drawing directly to conserve
// RAM for Wi-Fi/TLS. LovyanGFX provides the compatible numbered bitmap fonts.
class BoardDisplay : public Cyd22Display {
public:
    void init() {
        if (!Cyd22Display::init()) {
            Serial.println("[display] amarela2.2 init failed");
            abort();
        }
        setBrightness(100);
    }
    void setBrightness(unsigned percent) { BoardBrightness(percent); }
    void invertDisplay(bool invert) { Cyd22Display::invertDisplay(invert ? CYD_NORMAL_INVERT : !CYD_NORMAL_INVERT); }
    // TFT_eSPI's MSB-first monochrome icons, with transparent zero bits.
    void drawBitmap(int32_t x, int32_t y, const uint8_t *bitmap, int32_t w, int32_t h, uint16_t color) {
        /**
         * Rows are byte-aligned, so bitmap width need not be a multiple of eight.
         * Coalesce adjacent set bits into horizontal runs to reduce bus transactions.
         */
        const int32_t stride = (w + 7) / 8;
        startWrite();
        for (int32_t row = 0; row < h; ++row) {
            int32_t run = -1;
            for (int32_t col = 0; col <= w; ++col) {
                const bool on = col < w && (pgm_read_byte(bitmap + row * stride + col / 8) & (0x80 >> (col & 7)));
                if (on && run < 0) run = col;
                if (!on && run >= 0) { drawFastHLine(x + run, y + row, col - run, color); run = -1; }
            }
        }
        endWrite();
    }
    void present() {}
};
#else
class BoardDisplay : public TFT_eSPI {
public:
    void init() { TFT_eSPI::init(); setBrightness(100); }
    void setBrightness(unsigned percent) { BoardBrightness(percent); }
    // JSON Invert=true selects each board's tested normal color polarity.
    void invertDisplay(bool invert) { TFT_eSPI::invertDisplay(invert ? CYD_NORMAL_INVERT : !CYD_NORMAL_INVERT); }
    void present() {} // legacy boards draw directly to the display
};
#endif
extern BoardDisplay tft;
