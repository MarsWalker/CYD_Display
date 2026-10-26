/**
 * @file power_plot.h
 *
 * Small display-agnostic renderer for a power history rectangle.
 * Display must provide the subset of TFT methods used below. A fake display in
 * the native tests records the same drawing commands, so geometry and colors
 * can be verified without hardware. Drawing is immediate: there is no sprite,
 * framebuffer, asynchronous transfer ownership or chart-library allocation here.
 */

#pragma once
#include "power_history.h"
#include "power_graph_colors.h"
#include <stdio.h>

// RGB565 values equivalent to the Raspberry Pi palette. TFT_GREEN is full
// intensity (lime). No sprite/framebuffer or chart library is allocated.
/**
 * Draw a bounded graph from a borrowed history, or an empty waiting state.
 * The caller holds the engine mutex because pruning mutates the ring. This
 * routine sets the font/color/cursor it needs; subsequent text objects set
 * their own state. It does not restore a previous TFT drawing style.
 */
template<class Display>
void RenderPowerGraph(Display& display, PowerHistory* history, uint32_t now,
                      int x, int y, int width, int height,
                      PowerGraphColors colors = {})
{
    /**
     * Clip the requested size to the physical screen before drawing any pixels.
     * A small leftover rectangle is ignored rather than place axis labels outside
     * its border. Layouts remain pixel-based; clipping is not automatic scaling.
     */
    if (x < 0 || y < 0 || x >= display.width() || y >= display.height()) return;
    if (width > display.width() - x) width = display.width() - x;
    if (height > display.height() - y) height = display.height() - y;
    if (width < 64 || height < 40) return;
    const uint16_t background = colors.background;
    constexpr uint16_t border = 0x3186;
    constexpr uint16_t grid = 0x18E3, labels = 0x52AA;
    const uint16_t positive = colors.positive, negative = colors.negative;
    display.fillRect(x, y, width, height, background);
    display.drawRect(x, y, width, height, border);
    /**
     * Prune during drawing as well as polling: old peaks must age out even when
     * Wi-Fi is disconnected or HA has stopped returning valid measurements.
     */
    if (history) history->prune(now);
    float maximum = history ? history->maximum() : 100;
    for (int quarter = 1; quarter <= 3; ++quarter) {
        int gy = y + height * quarter / 4;
        display.drawLine(x + 1, gy, x + width - 2, gy, grid);
    }
    display.setTextFont(1);
    display.setTextColor(labels, background);
    display.setCursor(x + 3, y + 2);
    char label[16];
    snprintf(label, sizeof(label), "%ld", long(maximum));
    display.print(label);
    display.setCursor(x + 3, y + height - 11);
    display.print("0W");
    if (history && history->count >= 2) {
        PowerSegments(*history, now, x + 1, y + 1, width - 3, height - 3, maximum,
            [&](int x0, int y0, int x1, int y1, bool isNegative) {
                display.drawLine(x0, y0, x1, y1, isNegative ? negative : positive);
            });
    } else {
        display.setCursor(x + (width - display.textWidth("aguarda...")) / 2,
                          y + (height - 8) / 2);
        display.print("aguarda...");
    }
}
