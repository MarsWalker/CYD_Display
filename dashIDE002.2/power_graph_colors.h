/**
 * @file power_graph_colors.h
 *
 * Compact RGB565 palette copied into each parsed graph object.
 * In-class defaults preserve the original graph appearance when optional JSON
 * fields are absent. RGB565 values describe logical colors; each board backend
 * handles its physical panel's channel order and inversion separately.
 */

#pragma once
#include <stdint.h>

struct PowerGraphColors {
    uint16_t background = 0x0861;
    uint16_t positive = 0xFFE0;
    uint16_t negative = 0x07E0;
};

