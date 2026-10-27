/**
 * @file power_history.h
 *
 * Hardware-independent signed-power history and line-segment geometry.
 * The ring stores monotonic millisecond timestamps and float watts, including
 * negative values. Plotting uses magnitudes, while sign is retained for color
 * and text. The fixed 151-element capacity bounds RAM to roughly 1.2 KiB of
 * samples at the minimum supported two-second polling interval.
 * No locking occurs here: firmware callers hold the engine mutex; native tests
 * can exercise the model without Arduino, credentials or a display.
 */

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>

// Small, allocation-free model shared by the firmware and native tests.
struct PowerSample { uint32_t time; float watts; };
struct PowerHistory {
    static constexpr uint32_t windowMs = 300000;
    static constexpr size_t capacity = 151; // 5 minutes at >= 2-second intervals
    PowerSample samples[capacity] = {};
    /**
     * head indexes the oldest valid slot; logical index count-1 is the newest.
     * Unused backing entries need not be cleared when old samples expire.
     */
    size_t head = 0, count = 0;
    /**
     * valid means at least one successful reading in this source lifetime.
     * It intentionally stays true after the visible window becomes empty, so the
     * value label can retain its last valid reading while the graph says waiting.
     */
    bool valid = false;
    float latest = 0;

    /**
     * Borrow the i-th oldest sample; caller must ensure i < count.
     * Modulo indexing hides wraparound from pruning, scaling and rendering.
     */
    const PowerSample& at(size_t i) const { return samples[(head + i) % capacity]; }
    /**
     * Discard points older than the rolling five-minute window.
     * Unsigned subtraction handles a normal millis() wrap across the retained
     * window. Call regularly; timestamps are not absolute wall-clock time.
     */
    void prune(uint32_t now) {
        while (count && uint32_t(now - at(0).time) > windowMs) {
            head = (head + 1) % capacity;
            --count;
        }
    }
    /**
     * Parse a complete numeric state and append only finite, bounded watts.
     * Trailing whitespace is allowed, trailing units/garbage are not. Invalid
     * readings leave latest/count unchanged; the source/renderer prunes separately.
     */
    bool record(uint32_t now, const char* state, bool kilowatts = false) {
        if (!state) return false;
        char* end = nullptr;
        float value = strtof(state, &end);
        if (end == state) return false;
        while (isspace(static_cast<unsigned char>(*end))) ++end;
        if (*end) return false;
        if (kilowatts) value *= 1000;
        if (!isfinite(value) || fabsf(value) > 1e9f) return false;
        prune(now);
        if (count == capacity) { head = (head + 1) % capacity; --count; }
        samples[(head + count++) % capacity] = {now, value};
        latest = value;
        valid = true;
        return true;
    }
    /**
     * Compute an upper Y bound from retained magnitudes, never a signed minimum.
     * No points gives 100 W; all-zero points give 5 W. The margin is at least
     * 5 W or 5 percent, ensuring a nonzero divisor and headroom over the peak.
     */
    float maximum() const {
        if (!count) return 100;
        float peak = 0;
        for (size_t i = 0; i < count; ++i) peak = fmaxf(peak, fabsf(at(i).watts));
        return peak + fmaxf(5, peak * .05f);
    }
};

// Calls line(x0,y0,x1,y1,negative). Plot uses abs(W), with sign changes
// interpolated through zero, matching draw_watts_graph in the Raspberry Pi.
// Coordinates are relative to the inside of the plot's border.
/**
 * Emit one or two colored segments for every pair of retained samples.
 * The plot rectangle is already inside the graph border. maximum must be
 * positive, as provided by PowerHistory::maximum(). Plotting does not mutate
 * the history; its caller prunes before selecting a scale.
 */
template<class Line>
void PowerSegments(const PowerHistory& history, uint32_t now,
                   int x, int y, int w, int h, float maximum, Line line)
{
    /**
     * Use real sample ages instead of evenly spaced array slots. During startup
     * new points appear near the right edge; missing/slow polls preserve the gap
     * in time. Double arithmetic keeps interpolation stable at a sign crossing.
     */
    auto px = [&](double age) { return x + int((PowerHistory::windowMs - age) / PowerHistory::windowMs * w); };
    auto py = [&](float value) { return y + h - int(fminf(maximum, fabsf(value)) / maximum * h); };
    for (size_t i = 1; i < history.count; ++i) {
        const auto& a = history.at(i - 1);
        const auto& b = history.at(i);
        uint32_t ageA = now - a.time, ageB = now - b.time;
        if (ageA > PowerHistory::windowMs || ageB > PowerHistory::windowMs) continue;
        /**
         * Split opposite signs at a linearly interpolated zero crossing.
         * Testing signs separately avoids a potentially large watts multiplication.
         * Both output segments meet at the base, but each uses its own side's color.
         */
        if ((a.watts < 0 && b.watts > 0) || (a.watts > 0 && b.watts < 0)) {
            double fraction = fabsf(a.watts) / (double(fabsf(a.watts)) + fabsf(b.watts));
            int zeroX = px(double(ageA) + (double(ageB) - ageA) * fraction);
            line(px(ageA), py(a.watts), zeroX, py(0), a.watts < 0);
            line(zeroX, py(0), px(ageB), py(b.watts), b.watts < 0);
        } else {
            line(px(ageA), py(a.watts), px(ageB), py(b.watts), a.watts < 0 || b.watts < 0);
        }
    }
}
