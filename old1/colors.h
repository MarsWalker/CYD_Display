// UPDATED
#ifndef COLORS_H
#define COLORS_H

#include <TFT_eSPI.h>

/**
 * @brief Converts a color name string or hex literal into its corresponding 16-bit RGB565 value.
 * Normalizes input text by trimming spaces and converting to lowercase to ensure match safety.
 * If no match is found, defaults to white.
 * * @param nome The string representation of the color name (e.g., "red", "skyblue") or hex ("0xF800").
 * @return uint16_t The formatted 16-bit RGB565 color value for the TFT display.
 */
uint16_t GetColorFromName(String nome)
{
    nome.trim();
    nome.toLowerCase();

    // Basic Colors
    if (nome == "black")       return TFT_BLACK;
    if (nome == "white")       return TFT_WHITE;
    if (nome == "red")         return TFT_RED;
    if (nome == "green")       return TFT_GREEN;
    if (nome == "blue")        return TFT_BLUE;
    if (nome == "yellow")      return TFT_YELLOW;
    if (nome == "cyan")        return TFT_CYAN;
    if (nome == "magenta")     return TFT_MAGENTA;
    if (nome == "orange")      return TFT_ORANGE;
    if (nome == "pink")        return TFT_PINK;
    if (nome == "purple")      return TFT_PURPLE;
    if (nome == "grey")        return TFT_LIGHTGREY;
    if (nome == "gray")        return TFT_LIGHTGREY;

    // Dark Shades
    if (nome == "darkgrey")    return TFT_DARKGREY;
    if (nome == "darkgray")    return TFT_DARKGREY;
    if (nome == "darkgreen")   return TFT_DARKGREEN;
    if (nome == "darkcyan")    return TFT_DARKCYAN;
    if (nome == "navy")        return TFT_NAVY;
    if (nome == "maroon")      return TFT_MAROON;
    if (nome == "olive")       return TFT_OLIVE;

    // Light Shades / Variants
    if (nome == "lightgrey")   return TFT_LIGHTGREY;
    if (nome == "lightgray")   return TFT_LIGHTGREY;
    if (nome == "greenyellow") return TFT_GREENYELLOW;
    if (nome == "skyblue")     return TFT_SKYBLUE;
    if (nome == "gold")        return TFT_GOLD;
    if (nome == "silver")      return TFT_SILVER;
    if (nome == "violet")      return TFT_VIOLET;
    if (nome == "brown")       return TFT_BROWN;

    // Custom Hex Parser (Expected Format: "0xFFFF")
    if (nome.startsWith("0x") && nome.length() >= 6)
    {
        return (uint16_t)strtol(nome.c_str(), NULL, 16);
    }

    // Default Fallback
    return TFT_WHITE;
}

#endif