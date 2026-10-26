/**
 * @file board_geometry_checks.cpp
 *
 * Compile-time regression tests for display/touch coordinate transformations.
 * Non-square panels expose width/height swaps and off-by-one errors that a
 * square test surface would hide. static_assert failures stop compilation;
 * these checks allocate no runtime buffers and perform no hardware access.
 */

#include "board_geometry.h"
#include <initializer_list>

// Compile-time regression checks for the non-square panel and mirrored modes.
// The same function indexes the framebuffer and transforms touch coordinates.
constexpr bool mapsTo(int px, int py, unsigned r, int expectedX, int expectedY) {
    int x = 0, y = 0;
    BlackNativeToLogical(px, py, r, x, y);
    return x == expectedX && y == expectedY;
}
static_assert(mapsTo(0, 0, 0, 0, 0));
static_assert(mapsTo(319, 479, 0, 319, 479));
static_assert(mapsTo(0, 0, 1, 0, 319));
static_assert(mapsTo(319, 479, 1, 479, 0));
static_assert(mapsTo(0, 0, 2, 319, 479));
static_assert(mapsTo(319, 479, 2, 0, 0));
static_assert(mapsTo(0, 0, 3, 479, 0));
static_assert(mapsTo(319, 479, 3, 0, 319));
static_assert(mapsTo(0, 0, 4, 319, 0));
static_assert(mapsTo(0, 0, 5, 479, 319));
static_assert(mapsTo(0, 0, 6, 0, 479));
static_assert(mapsTo(0, 0, 7, 0, 0));
static_assert(mapsTo(80, 461, 1, 461, 239));

constexpr bool staysInside() {
    for (unsigned r = 0; r < 8; ++r) {
        for (int px : {0, 80, 160, 319}) {
            for (int py : {0, 120, 240, 461, 479}) {
                int x = 0, y = 0;
                BlackNativeToLogical(px, py, r, x, y);
                if (x < 0 || y < 0 || x >= ((r & 1) ? 480 : 320) ||
                    y >= ((r & 1) ? 320 : 480)) return false;
            }
        }
    }
    return true;
}
static_assert(staysInside());

constexpr bool pinkGeometryChecks() {
    for (unsigned r = 0; r < 8; ++r) {
        for (int px : {0, 120, 239}) {
            for (int py : {0, 160, 319}) {
                int x = 0, y = 0;
                PinkNativeToLogical(px, py, r, x, y);
                if (x < 0 || y < 0 || x >= ((r & 1) ? 320 : 240) ||
                    y >= ((r & 1) ? 240 : 320)) return false;
            }
        }
    }
    int x = 0, y = 0;
    PinkNativeToLogical(0, 0, 1, x, y);
    if (x != 0 || y != 239) return false;
    PinkNativeToLogical(239, 319, 1, x, y);
    if (x != 319 || y != 0) return false;
    PinkNativeToLogical(0, 0, 4, x, y);
    return x == 0 && y == 319;
}
static_assert(pinkGeometryChecks());

constexpr bool cst820GeometryChecks() {
    const int originX[] = {0, 0, 239, 319, 0, 0, 239, 319};
    const int originY[] = {0, 239, 319, 0, 319, 0, 0, 239};
    for (unsigned r = 0; r < 8; ++r) {
        int x = 0, y = 0;
        Cst820NativeToLogical(0, 0, r, x, y);
        if (x != originX[r] || y != originY[r]) return false;
        for (int px : {0, 239}) {
            for (int py : {0, 319}) {
                Cst820NativeToLogical(px, py, r, x, y);
                if (x < 0 || y < 0 || x >= ((r & 1) ? 320 : 240) ||
                    y >= ((r & 1) ? 240 : 320)) return false;
            }
        }
    }
    return true;
}
static_assert(cst820GeometryChecks());
