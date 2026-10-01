//UPDATED
#ifndef TOUCH_XPT2046_H
#define TOUCH_XPT2046_H

#include <SPI.h>
#include <XPT2046_Touchscreen.h>

// ===========================================================
// Hardware Pin Definitions for XPT2046 Touch Controller
// ===========================================================
#define TOUCH_CS   33
#define TOUCH_IRQ  36
#define TOUCH_CLK  25
#define TOUCH_MISO 39
#define TOUCH_MOSI 32

// Reference to external touchscreen instance if needed elsewhere
extern XPT2046_Touchscreen ts;

// ===========================================================
// Global Variables for Touch Tracking and Debouncing
// ===========================================================
int touchX = 0;                             // Last registered X coordinate mapped to screen resolution
int touchY = 0;                             // Last registered Y coordinate mapped to screen resolution
unsigned long lastTouchTime = 0;            // Stores the timestamp of the last valid touch event (ms)
const unsigned long TOUCH_DEBOUNCE = 400;   // Debounce timeout threshold for improved stability and precision

// ===========================================================
// Calibration and Screen Resolution Constants
// ===========================================================
#define TS_MINX  300                        // Minimum raw ADC value from touch controller on X axis
#define TS_MAXX  3900                       // Maximum raw ADC value from touch controller on X axis
#define TS_MINY  300                        // Minimum raw ADC value from touch controller on Y axis
#define TS_MAXY  3900                       // Maximum raw ADC value from touch controller on Y axis
#define SCREEN_W 320                        // Target screen width in pixels
#define SCREEN_H 240                        // Target screen height in pixels

// ===========================================================
// Hardware Instances
// ===========================================================
SPIClass touchSPI(VSPI);                    // Dedicated SPI bus instance for the touch controller
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ); // Touchscreen instance bound to CS and IRQ pins

// ===========================================================
// Functions
// ===========================================================

/**
 * @brief Initializes the SPI bus and the XPT2046 touch controller.
 * Configures the hardware pins, registers the SPI interface, and sets the screen orientation.
 */
void TouchInit() {
    touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    ts.begin(touchSPI);
    ts.setRotation(1);                      // Set landscape rotation to match display orientation
    Serial.println("Touch XPT2046 active.");
}

/**
 * @brief Checks for a valid, debounced touch event and updates global coordinates.
 * Reads raw ADC touch data, ignores noise, and maps raw positions into screen pixel spaces.
 * * @return true if a new and valid touch point was registered, false otherwise.
 */
bool CheckTouch() {
    // Check hardware registers and IRQ pin status; return early if no physical touch is active
    if (!ts.tirqTouched() || !ts.touched()) return false;
    
    // Software debounce check
    unsigned long agora = millis();
    if (agora - lastTouchTime < TOUCH_DEBOUNCE) return false;
    lastTouchTime = agora;

    // Retrieve raw coordinate point from hardware registers
    TS_Point p = ts.getPoint();

    // Discard default/empty data points
    if (p.x == 0 && p.y == 0) return false;

    // Debug output printing raw coordinate metrics
    Serial.println(p.x);
    Serial.println(p.y);
    Serial.println(p.z);

    // Map raw hardware ADC coordinates into target screen resolution pixels
    touchX = map(p.x, TS_MINX, TS_MAXX, 0, SCREEN_W);
    touchY = map(p.y, TS_MINY, TS_MAXY, 0, SCREEN_H);
    
    // Constrain outputs to stay strictly within display boundary limits
    touchX = constrain(touchX, 0, SCREEN_W - 1);
    touchY = constrain(touchY, 0, SCREEN_H - 1);

    return true;
}

#endif