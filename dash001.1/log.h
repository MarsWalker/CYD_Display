#ifndef LOG_H
#define LOG_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SD.h>
#include <stdarg.h>

/*
 * log.h
 * -------------------------------------------------------
 * Logging (Serial + TFT + SD) for dash001.
 * WiFi / HTTP live in comms.h (see that file).
 *
 * - Serial: always, with a [ms] timestamp and level (D/I/W/E).
 * - TFT: level-colored log, horizontal+vertical wrap
 *   (enable with logSetTFT(true) after tft.init()).
 * - SD: /logs/<HOSTNAME>.log with a line buffer (only writes on
 *   newline), periodic flush and rotation (enable with LogInit()).
 *
 * Levels: LOG_LVL_DEBUG/INFO/WARN/ERROR. Filter with LOG_MIN_LEVEL
 * (e.g. #define LOG_MIN_LEVEL LOG_LVL_INFO before the include).
 *
 * Usage:
 *   #include "log.h"
 *   TFT_eSPI tft = TFT_eSPI();   // global in the sketch
 *   tft.init();
 *   logSetTFT(true);
 *   LogInit(HOSTNAME);
 *   logI("hello");
 *   logEf("failed: %d", code);
 *   LogFlush();                  // write the buffer to the SD
 * -------------------------------------------------------
 */

#ifndef SD_CS
#define SD_CS 4
#endif

#define LOG_DIR            "/logs"
#define LOG_DEFAULT_HOST   "board1"
#define LOG_MAX_BYTES      51200UL  // rotation: .log -> .log.old
#define LOG_FLUSH_LINES    16       // flush every N lines
#define LOG_FLUSH_MS       2000UL   // ...or every 2 s
#define LOG_LINE_BUF       320      // line buffer (SD)

// ---------------------------------------------------------------
// Screen coordinates / configuration of the on-screen log
// ---------------------------------------------------------------
#define LOG_MARGIN_X      4
#define LOG_START_Y       4
#define LOG_LINE_HEIGHT   16
#define LOG_TEXT_SIZE     1
#define LOG_TEXT_COLOR    TFT_WHITE
#define LOG_BG_COLOR      TFT_BLACK

// ---------------------------------------------------------------
// Log levels
// ---------------------------------------------------------------
#define LOG_LVL_DEBUG 0
#define LOG_LVL_INFO  1
#define LOG_LVL_WARN  2
#define LOG_LVL_ERROR 3
#define LOG_LVL_OFF   4

#ifndef LOG_MIN_LEVEL
#define LOG_MIN_LEVEL LOG_LVL_DEBUG
#endif

// TFT object expected in the sketch (adjust the name if you use another one)
extern TFT_eSPI tft;

// ---------------------------------------------------------------
// Log state. Accessed via logS(): static inside an inline function,
// so there is a single instance even when included from several files.
// ---------------------------------------------------------------
struct LogState {
    int16_t  cursorX     = LOG_MARGIN_X;
    int16_t  cursorY     = LOG_START_Y;
    bool     tftOn       = false;
    bool     sdMounted   = false;
    bool     sdOK        = false;
    bool     lineOpen    = false;   // [ms] N prefix already emitted on this line
    File     file;
    uint32_t lastFlushMs = 0;
    uint16_t pendingLines = 0;
    char     lineBuf[LOG_LINE_BUF];
    size_t   lineLen     = 0;
};

inline LogState& logS() { static LogState s; return s; }

inline bool _lvlEnabled(uint8_t lvl)
{
    return lvl >= (uint8_t)LOG_MIN_LEVEL && lvl < (uint8_t)LOG_LVL_OFF;
}

inline char _lvlChar(uint8_t lvl)
{
    static const char chars[] = { 'D', 'I', 'W', 'E' };
    return (lvl <= LOG_LVL_ERROR) ? chars[lvl] : '?';
}

inline uint16_t _lvlColor(uint8_t lvl)
{
    if (lvl == LOG_LVL_WARN)  return TFT_YELLOW;
    if (lvl == LOG_LVL_ERROR) return TFT_RED;
    return LOG_TEXT_COLOR;
}

// ---------------------------------------------------------------
// SD: idempotent mount (shared with secrets.h)
// ---------------------------------------------------------------
inline bool sdEnsureMount()
{
    LogState& s = logS();
    if (s.sdMounted) return true;
    if (SD.begin(SD_CS)) {
        s.sdMounted = true;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------
// Periodic / forced flush
// ---------------------------------------------------------------
inline void LogFlush()
{
    LogState& s = logS();
    if (!s.sdOK) return;
    s.file.flush();
    s.pendingLines = 0;
    s.lastFlushMs  = millis();
}

inline void _sdMaybeFlush()
{
    LogState& s = logS();
    if (!s.sdOK) return;
    uint32_t now = millis();
    if (s.pendingLines >= LOG_FLUSH_LINES || (now - s.lastFlushMs) >= LOG_FLUSH_MS) {
        s.file.flush();
        s.pendingLines = 0;
        s.lastFlushMs  = now;
    }
}

// ---------------------------------------------------------------
// Closes the file (before deep sleep / reboot)
// ---------------------------------------------------------------
inline void LogClose()
{
    LogState& s = logS();
    if (!s.sdOK) return;
    LogFlush();
    s.file.close();
    s.sdOK = false;
}

// ---------------------------------------------------------------
// Rotation: if the .log grows past LOG_MAX_BYTES, rename to .old
// ---------------------------------------------------------------
inline void _logRotateIfNeeded(const String& path)
{
    File f = SD.open(path, FILE_READ);
    if (!f) return;
    size_t sz = f.size();
    f.close();
    if (sz < LOG_MAX_BYTES) return;

    String oldPath = path + ".old";
    if (SD.exists(oldPath.c_str())) SD.remove(oldPath.c_str());
    SD.rename(path.c_str(), oldPath.c_str());
}

// ---------------------------------------------------------------
// Log line: one prefix per line (Serial + SD),
// message accumulated, newline closes and writes to the SD.
// ---------------------------------------------------------------
inline void _lineEnsurePrefix(uint8_t lvl)
{
    LogState& s = logS();
    if (s.lineOpen) return;

    char p[24];
    snprintf(p, sizeof(p), "[%lu] %c ", (unsigned long)millis(), _lvlChar(lvl));
    Serial.print(p);

    if (s.sdOK) {
        size_t n = strlen(p);
        if (n >= sizeof(s.lineBuf)) n = sizeof(s.lineBuf) - 1;
        memcpy(s.lineBuf, p, n);
        s.lineBuf[n] = '\0';
        s.lineLen = n;
    }
    s.lineOpen = true;
}

inline void _lineAppend(const String& msg)
{
    LogState& s = logS();
    Serial.print(msg);
    if (!s.sdOK || !s.lineOpen) return;

    const char* p = msg.c_str();
    size_t n = msg.length();
    while (n > 0) {
        size_t space = sizeof(s.lineBuf) - s.lineLen - 1;
        size_t take  = (n < space) ? n : space;
        memcpy(s.lineBuf + s.lineLen, p, take);
        s.lineLen += take;
        s.lineBuf[s.lineLen] = '\0';
        p   += take;
        n   -= take;
        if (n > 0) {
            // buffer full: dump it and start a continuation line
            s.file.println(s.lineBuf);
            s.pendingLines++;
            s.lineLen = 0;
            s.lineBuf[0] = '\0';
            int w = snprintf(s.lineBuf, sizeof(s.lineBuf), "[%lu] + ", (unsigned long)millis());
            s.lineLen = (w > 0) ? (size_t)w : 0;
            _sdMaybeFlush();
        }
    }
}

inline void _lineEnd()
{
    LogState& s = logS();
    Serial.println();
    if (s.sdOK && s.lineOpen) {
        s.file.println(s.lineBuf);
        s.pendingLines++;
        _sdMaybeFlush();
    }
    s.lineOpen = false;
    s.lineLen  = 0;
    s.lineBuf[0] = '\0';
}

// ---------------------------------------------------------------
// LogInit: opens /logs/<hostname>.log (default: board1).
// Call AFTER LoadSecretsFromSD() so the right HOSTNAME is used.
// ---------------------------------------------------------------
inline void LogInit(const char* hostname = nullptr)
{
    LogState& s = logS();

    if (s.sdOK) {
        LogFlush();
        s.file.close();
        s.sdOK = false;
    }
    if (s.lineOpen) {
        Serial.println();
        s.lineOpen  = false;
        s.lineLen   = 0;
        s.lineBuf[0] = '\0';
    }

    if (!sdEnsureMount()) return;

    const char* host = (hostname && hostname[0]) ? hostname : LOG_DEFAULT_HOST;
    String path = String(LOG_DIR) + "/" + host + ".log";

    if (!SD.exists(LOG_DIR)) SD.mkdir(LOG_DIR);
    _logRotateIfNeeded(path);

    File f = SD.open(path, FILE_APPEND);
    if (!f) return;

    s.file = f;
    s.sdOK = true;

    char hdr[96];
    snprintf(hdr, sizeof(hdr), "[%lu] === log started (%s) ===",
             (unsigned long)millis(), host);
    s.file.println(hdr);
    s.pendingLines = 1;
    s.lastFlushMs  = millis();
    s.file.flush();
    s.pendingLines = 0;
}

// ---------------------------------------------------------------
// TFT: enable/disable the on-screen log (after tft.init())
// ---------------------------------------------------------------
inline void logSetTFT(bool enable)
{
    logS().tftOn = enable;
}

// ---------------------------------------------------------------
// Resets the log cursor (clears the screen and starts over)
// ---------------------------------------------------------------
inline void resetLog()
{
    LogState& s = logS();
    if (s.tftOn) tft.fillScreen(LOG_BG_COLOR);
    s.cursorX = LOG_MARGIN_X;
    s.cursorY = LOG_START_Y;
}

// ---------------------------------------------------------------
// TFT: vertical wrap (at the bottom, wraps to the top and clears)
// ---------------------------------------------------------------
inline void _tftVWrap()
{
    LogState& s = logS();
    if (!s.tftOn) return;
    if (s.cursorY > tft.height() - LOG_LINE_HEIGHT) {
        s.cursorX = LOG_MARGIN_X;
        s.cursorY = LOG_START_Y;
        tft.fillScreen(LOG_BG_COLOR);
    }
}

// ---------------------------------------------------------------
// TFT: prints msg from the cursor, breaking lines when it does not
// fit the screen width (horizontal wrap by chunks)
// ---------------------------------------------------------------
inline void _tftPrint(const String& msg)
{
    LogState& s = logS();
    if (!s.tftOn || msg.length() == 0) return;

    _tftVWrap();
    const int limitX = tft.width() - LOG_MARGIN_X;
    const size_t n   = msg.length();
    size_t i = 0;

    while (i < n) {
        if (s.cursorX >= limitX) {
            s.cursorX = LOG_MARGIN_X;
            s.cursorY += LOG_LINE_HEIGHT;
            _tftVWrap();
        }

        size_t take = 1;
        while (i + take < n &&
               tft.textWidth(msg.substring(i, i + take + 1)) <= (limitX - s.cursorX)) {
            take++;
        }

        String part = msg.substring(i, i + take);
        int w = tft.textWidth(part);
        if (w > (limitX - s.cursorX) && s.cursorX > LOG_MARGIN_X) {
            s.cursorX = LOG_MARGIN_X;
            s.cursorY += LOG_LINE_HEIGHT;
            _tftVWrap();
        }

        tft.setCursor(s.cursorX, s.cursorY);
        tft.print(part);
        s.cursorX += w;
        i += take;
    }
}

// ---------------------------------------------------------------
// writeLogLvl: prefix + msg without newline (accumulates on the line)
// writeLoglnLvl: closes the line (Serial + SD + new line on TFT)
// ---------------------------------------------------------------
inline void writeLogLvl(uint8_t lvl, const String& msg)
{
    if (!_lvlEnabled(lvl)) return;

    _lineEnsurePrefix(lvl);
    _lineAppend(msg);

    LogState& s = logS();
    if (s.tftOn) {
        tft.setTextSize(LOG_TEXT_SIZE);
        tft.setTextColor(_lvlColor(lvl), LOG_BG_COLOR);
        _tftPrint(msg);
    }
}

inline void writeLoglnLvl(uint8_t lvl, const String& msg)
{
    if (!_lvlEnabled(lvl)) return;

    _lineEnsurePrefix(lvl);
    _lineAppend(msg);
    _lineEnd();

    LogState& s = logS();
    if (s.tftOn) {
        tft.setTextSize(LOG_TEXT_SIZE);
        tft.setTextColor(_lvlColor(lvl), LOG_BG_COLOR);
        _tftPrint(msg);
        s.cursorX = LOG_MARGIN_X;
        s.cursorY += LOG_LINE_HEIGHT;
        _tftVWrap();
    }
}

// ---------------------------------------------------------------
// Classic API (compatible with old code) = INFO level
// ---------------------------------------------------------------
inline void writeLog(const String& msg)      { writeLogLvl(LOG_LVL_INFO, msg); }
inline void writeLogln(const String& msg)     { writeLoglnLvl(LOG_LVL_INFO, msg); }

// ---------------------------------------------------------------
// printf-style. Removes trailing newlines and closes the line.
// ---------------------------------------------------------------
inline void writeLogfLvl(uint8_t lvl, const char* fmt, ...)
{
    if (!_lvlEnabled(lvl)) return;

    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) buf[--len] = '\0';

    writeLoglnLvl(lvl, String(buf));
}

inline void writeLogf(const char* fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) buf[--len] = '\0';

    writeLoglnLvl(LOG_LVL_INFO, String(buf));
}

// ---------------------------------------------------------------
// Macros per level
// ---------------------------------------------------------------
#define logD(msg)      writeLoglnLvl(LOG_LVL_DEBUG, msg)
#define logI(msg)      writeLoglnLvl(LOG_LVL_INFO, msg)
#define logW(msg)      writeLoglnLvl(LOG_LVL_WARN, msg)
#define logE(msg)      writeLoglnLvl(LOG_LVL_ERROR, msg)
#define logDf(fmt, ...) writeLogfLvl(LOG_LVL_DEBUG, fmt, ##__VA_ARGS__)
#define logIf(fmt, ...) writeLogfLvl(LOG_LVL_INFO, fmt, ##__VA_ARGS__)
#define logWf(fmt, ...) writeLogfLvl(LOG_LVL_WARN, fmt, ##__VA_ARGS__)
#define logEf(fmt, ...) writeLogfLvl(LOG_LVL_ERROR, fmt, ##__VA_ARGS__)

#endif // LOG_H
