/**
 * @file board_geometry.h
 *
 * Pure coordinate transforms from native portrait coordinates to logical UI
 * coordinates. Inputs are already calibrated and bounds-checked by the caller.
 * The low two rotation bits select a quarter-turn; bit 2 selects a mirror.
 * Mirror axes differ between panel families, so these functions are shared by
 * touch and framebuffer code to keep taps aligned with the displayed image.
 * constexpr permits compile-time regression checks without hardware.
 */

#pragma once

// ILI9341 TFT_eSPI MADCTL rotations for both tested pink/amarela portraits.
// Modes 4..7 mirror logical Y, as defined by ILI9341_Rotation.h.
constexpr void PinkNativeToLogical(int px, int py, unsigned rotation, int &x, int &y) {
    switch (rotation & 3) {
        case 0: x = px;       y = py;       break;
        case 1: x = py;       y = 239 - px; break;
        case 2: x = 239 - px; y = 319 - py; break;
        default:x = 319 - py; y = px;       break;
    }
    if (rotation & 4) y = ((rotation & 1) ? 240 : 320) - 1 - y;
}

// Same native-to-logical mapping for the framebuffer and touch.
// LovyanGFX ST7789's MADCTL table has the same logical mapping as the
// 240x320 ILI9341 table relative to its own validated portrait orientation.
constexpr void Cst820NativeToLogical(int px, int py, unsigned rotation, int &x, int &y) {
    PinkNativeToLogical(px, py, rotation, x, y);
}

// Same native-to-logical mapping for the framebuffer and touch.
// 0..3 = clockwise quarter turns; 4..7 additionally mirror logical X.
constexpr void BlackNativeToLogical(int px, int py, unsigned rotation, int &x, int &y) {
    switch (rotation & 3) {
        case 0: x = px;       y = py;       break;
        case 1: x = py;       y = 319 - px; break;
        case 2: x = 319 - px; y = 479 - py; break;
        default:x = 479 - py; y = px;       break;
    }
    if (rotation & 4) x = ((rotation & 1) ? 480 : 320) - 1 - x;
}
