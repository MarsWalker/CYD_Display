#ifndef COLORS_H
#define COLORS_H

#include <TFT_eSPI.h>

uint16_t GetColorFromName(String colorName)
{
    colorName.trim();
    colorName.toLowerCase();

    if (colorName == "black")      return TFT_BLACK;
    if (colorName == "white")      return TFT_WHITE;
    if (colorName == "red")        return TFT_RED;
    if (colorName == "green")      return TFT_GREEN;
    if (colorName == "blue")       return TFT_BLUE;
    if (colorName == "yellow")     return TFT_YELLOW;
    if (colorName == "cyan")       return TFT_CYAN;
    if (colorName == "magenta")    return TFT_MAGENTA;
    if (colorName == "orange")     return TFT_ORANGE;
    if (colorName == "pink")       return TFT_PINK;
    if (colorName == "purple")     return TFT_PURPLE;
    if (colorName == "grey")       return TFT_LIGHTGREY;
    if (colorName == "gray")       return TFT_LIGHTGREY;

    if (colorName == "darkgrey")   return TFT_DARKGREY;
    if (colorName == "darkgray")   return TFT_DARKGREY;
    if (colorName == "darkgreen")  return TFT_DARKGREEN;
    if (colorName == "darkcyan")   return TFT_DARKCYAN;
    if (colorName == "navy")       return TFT_NAVY;
    if (colorName == "maroon")     return TFT_MAROON;
    if (colorName == "olive")      return TFT_OLIVE;

    if (colorName == "lightgrey")  return TFT_LIGHTGREY;
    if (colorName == "lightgray")  return TFT_LIGHTGREY;
    if (colorName == "greenyellow") return TFT_GREENYELLOW;
    if (colorName == "skyblue")    return TFT_SKYBLUE;
    if (colorName == "gold")       return TFT_GOLD;
    if (colorName == "silver")     return TFT_SILVER;
    if (colorName == "violet")     return TFT_VIOLET;
    if (colorName == "brown")      return TFT_BROWN;

    if (colorName.startsWith("0x") && colorName.length() >= 6)
    {
        return (uint16_t)strtol(colorName.c_str(), NULL, 16);
    }

    return TFT_WHITE;
}

#endif
