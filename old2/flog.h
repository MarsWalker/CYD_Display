/*
 * flog.h
 * -------------------------------------------------------
 * Logging functions (Serial + TFT) and a helper function
 * for making HTTP/HTTPS requests based on the provided URL.
 *
 * Include immediately after secrets.h:
 *
 *   #include "secrets.h"
 *   #include "logutils.h"
 *
 * IMPORTANT:
 * - Assumes a global "tft" object already exists
 *   (typically TFT_eSPI in CYD projects).
 *   If your sketch uses a different name, adjust the line below.
 * - Assumes WiFi is already connected before calling httpRequest().
 * -------------------------------------------------------
 */

#ifndef LOGUTILS_H
#define LOGUTILS_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// If your TFT object has a different name, change it here:
extern TFT_eSPI tft;

// ---------------------------------------------------------------
// Screen log position / configuration
// ---------------------------------------------------------------
#define LOG_MARGIN_X      4      // Left margin (cursor reset position)
#define LOG_START_Y       4      // Initial Y position
#define LOG_LINE_HEIGHT   16     // Line height (depends on font size)
#define LOG_TEXT_SIZE     1      // Font size used for logging
#define LOG_TEXT_COLOR    TFT_WHITE
#define LOG_BG_COLOR      TFT_BLACK

// Current screen log cursor position
static int16_t logCursorX = LOG_MARGIN_X;
static int16_t logCursorY = LOG_START_Y;

// ---------------------------------------------------------------
// Resets the log cursor (useful after clearing the screen)
// ---------------------------------------------------------------
inline void resetLog() {
    tft.fillScreen(LOG_BG_COLOR);
    logCursorX = LOG_MARGIN_X;
    logCursorY = LOG_START_Y;
}

// ---------------------------------------------------------------
// If the cursor reaches the bottom of the screen,
// wrap back to the top (simple screen wrap)
// ---------------------------------------------------------------
inline void logCheckWrap() {
    if (logCursorY > tft.height() - LOG_LINE_HEIGHT) {
        logCursorX = LOG_MARGIN_X;
        logCursorY = LOG_START_Y;
        tft.fillScreen(LOG_BG_COLOR);
    }
}

// ---------------------------------------------------------------
// writeLog: writes a message to both Serial and TFT,
// keeping the cursor on the same line.
// ---------------------------------------------------------------
inline void writeLog(const String &msg) {
    Serial.print(msg);

    logCheckWrap();
    tft.setTextSize(LOG_TEXT_SIZE);
    tft.setTextColor(LOG_TEXT_COLOR, LOG_BG_COLOR);
    tft.setCursor(logCursorX, logCursorY);
    tft.print(msg);

    // Advance X according to the width of the printed text
    logCursorX += tft.textWidth(msg);
}

// ---------------------------------------------------------------
// writeLogln: writes a message to Serial (with newline)
// and TFT, then moves the cursor to the next line.
// ---------------------------------------------------------------
inline void writeLogln(const String &msg) {
    Serial.println(msg);

    logCheckWrap();
    tft.setTextSize(LOG_TEXT_SIZE);
    tft.setTextColor(LOG_TEXT_COLOR, LOG_BG_COLOR);
    tft.setCursor(logCursorX, logCursorY);
    tft.print(msg);

    // Move to the next line
    logCursorX = LOG_MARGIN_X;
    logCursorY += LOG_LINE_HEIGHT;
}
#endif // LOGUTILS_H