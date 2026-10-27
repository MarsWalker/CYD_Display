/**
 * @file power_graph_json.h
 *
 * Read optional per-object graph colors independently of hardware rendering.
 * Resolver is injected so production uses the shared named-color function and
 * native tests can substitute a small deterministic resolver. Values must be
 * non-empty strings to override defaults. Background is canonical; lowercase
 * background and the historical backgroud spelling are fallback aliases.
 */

#pragma once
#include "power_graph_colors.h"
#include <ArduinoJson.h>

// Keep color-name resolution consistent with the rest of the dashboard.
// Each missing, empty or non-string property independently keeps its default.
template<class Resolver>
PowerGraphColors ParsePowerGraphColors(JsonObject object, Resolver resolve)
{
    PowerGraphColors colors;
    JsonVariant bg = object["Background"];
    if (!bg.is<const char*>()) bg = object["background"];
    if (!bg.is<const char*>()) bg = object["backgroud"];
    const char* background = bg | "";
    const char* positive = object["PositiveColor"] | "";
    const char* negative = object["NegativeColor"] | "";
    if (*background) colors.background = resolve(background);
    if (*positive) colors.positive = resolve(positive);
    if (*negative) colors.negative = resolve(negative);
    return colors;
}
