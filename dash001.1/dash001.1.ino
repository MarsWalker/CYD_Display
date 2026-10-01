/*
 * dash001.ino - MAIN SKETCH of the CYD 2.8" wall dashboard.
 *
 * Flow (setup):
 *   1. Serial + TFT init (logSetTFT for on-screen boot messages).
 *   2. LoadSecretsFromSD() -> secrets.h defaults, possibly overridden
 *      by /secrets.json on the SD card (WIFI_SSID, JSON_URL, ...).
 *   3. LogInit(HOSTNAME) -> /logs/<HOSTNAME>.log on the SD card.
 *   4. loadBoardJson() -> board JSON (SD first, then JSON_URL).
 *   5. Parse board JSON: DataSources, log flags, touch, screens,
 *      timezone. TouchInit() + NTP start.
 *   6. LoadScreen(1) -> first screen drawn on the TFT.
 *   7. EngineInit() -> 1 core sequential / 2 cores (data on core 0).
 *   8. WebInit() -> tiny web-UI on :80 (screen change / reload).
 *
 * Loop:
 *   - WebLoop()   : serve the web-UI.
 *   - CheckTouch(): physical left/right tap navigation (wraparound).
 *   - EngineLoop(): datasource refresh + screen redraw.
 *
 * Each .h module holds ONE responsibility (see brain.md):
 *   log.h, comms.h (wifi+http), colors.h, images.h, datasources.h,
 *   datacache.h, screens.h, objects.h, functions.h, draw.h, engine.h,
 *   web.h, touch.h, secrets.h.
 * ---------------------------------------------------------------
 */

#include <TFT_eSPI.h>
#include <time.h>
#include "log.h"
#include "secrets.h"
#include "comms.h"
#include "colors.h"
#include "images.h"
#include "datasources.h"
#include "datacache.h"
#include "screens.h"
#include "objects.h"
#include "functions.h"
#include "draw.h"
#include "engine.h"
#include "build.h"
#include "web.h"
#include "touch.h"

// Global TFT display instance (used by log.h, images.h and the UI).
TFT_eSPI tft = TFT_eSPI();

// Timezone for the on-screen Clock/Date. Read from the board JSON root
// ("TIMEZONE", optional POSIX TZ string) so it can be changed per
// location without recompiling; default is Europe/Lisbon (WET/WEST).
static String g_timeZone = "WET0WEST,M3.5.0/1,M10.5.0";

// Parsed board JSON, kept alive for the whole session so the web-UI can
// load/reload screens (LoadScreen reads the embedded Objects).
JsonDocument g_boardDoc;

// ---------------------------------------------------------------
// Engine hooks: data refresh (datasources) and draw (screen).
// Implemented here, called by engine.h (see that file for details).
// ---------------------------------------------------------------

// Called cyclically (core 0 task on dual-core, or loop() on single-core).
// Refreshes every datasource whose Refresh interval has elapsed.
void EngineDataUpdate()
{
    if (!wifiConnected()) return;   // no network -> nothing to fetch
    UpdateDataSources();
}

// Called cyclically on the draw side (core 1 on dual-core, or loop()).
// Redraws the current screen objects only when the screen "Refresh"
// interval has elapsed (EngineDataUpdate keeps the datasources fresh).
void EngineDraw()
{
    static uint32_t lastDrawMs = 0;
    uint32_t now = millis();
    if (now - lastDrawMs < g_screenRefresh) return;
    lastDrawMs = now;

    DrawObjects(false);   // incremental update (fill only on screen load)
}

// ---------------------------------------------------------------
// Board JSON loader.
//
// Order of sources (first valid wins):
//   1. SD card first  -> /fileName (works without network).
//   2. Network        -> JSON_URL + fileName (base URL from secrets).
//
// Used both for the board JSON itself (JSON_FILE) and for the screen
// files it references (e.g. "main.json"), which are fetched from
// the same JSON_URL. Accepts both JSON formats: "multi" and
// "standalone". Returns the JSON body as a String, or empty on
// failure.
// ---------------------------------------------------------------
String fetchBoardJson(const String& fileName)
{
    logIf("Board JSON: %s", fileName.c_str());

    // ------------------------------------------------------------
    // 1) SD first — if /fileName exists on the card, no network
    //    is needed.
    // ------------------------------------------------------------
    if (sdEnsureMount()) {
        String path = String("/") + fileName;
        if (SD.exists(path)) {
            File f = SD.open(path, FILE_READ);
            if (f) {
                String body;
                body.reserve(f.size());
                while (f.available()) body += (char)f.read();
                f.close();
                body.trim();

                if (body.length() > 0 &&
                    (body.startsWith("{") || body.startsWith("["))) {
                    logI(String("OK [SD] ") + path);
                    return body;
                }
                logWf("Ignored [SD] %s (invalid content).", path.c_str());
            }
        }
    }

    // ------------------------------------------------------------
    // 2) Network — only if there is no valid file on the SD.
    //    Single origin: JSON_URL (always read from secrets).
    //    Reconnect only if the WiFi is not already up (avoids a
    //    needless re-association + DHCP churn on every fetch).
    // ------------------------------------------------------------
    if (!wifiConnected() && !wifiConnect(WIFI_SSID, WIFI_PASSWORD)) {
        logE("No WiFi to fetch the JSON.");
        return String();
    }

    String url = String(JSON_URL) + fileName;
    int code = -1;
    String body = httpGet(url, &code);
    if (code == 200 && body.length() > 0 &&
        (body.startsWith("{") || body.startsWith("["))) {
        logI(String("OK [") + code + "] " + url);
        return body;
    }
    logWf("Failed [%d] %s", code, url.c_str());

    logE("No source returned a valid JSON.");
    return String();
}

// Loads the board JSON named by JSON_FILE (default "simple.json").
String loadBoardJson()
{
    return fetchBoardJson(String(JSON_FILE));
}

// ---------------------------------------------------------------
// Loads and draws the screen with the given number (1-based).
//
//  - Applies the screen settings to the display: rotation + invert,
//    and stores the "Refresh" interval in g_screenRefresh.
//  - RESETS the object array (ClearObjects) and fills it again from:
//      * standalone: the Objects embedded in the screen entry of the
//        board JSON (boardDoc, still parsed in setup);
//      * multi: the screen file JSON_URL + "name" (Objects at root).
//  - Draws the objects on the TFT (full redraw).
//
// Returns true on success.
// ---------------------------------------------------------------
bool LoadScreen(int number, JsonDocument* boardDoc)
{
    int idx = FindScreen(number);
    if (idx < 0) {
        logWf("Screen %d not found in Screens array.", number);
        return false;
    }

    ScreenInfo& sc = g_screens[idx];

    // Keep the current rotation global in sync (touch navigation
    // reads it to align its axes with the displayed screen).
    g_currentRotation = sc.rotation;

    // Screen configuration -> display.
    tft.setRotation(sc.rotation);
    tft.invertDisplay(sc.invert);

    // How often this screen should refresh.
    g_screenRefresh = (sc.refresh > 0) ? sc.refresh : 1000UL;

    // A new screen always resets the object array.
    ClearObjects();

    if (sc.embedded && boardDoc) {
        // Standalone: Objects live in the screen entry of the board JSON.
        JsonObject scn = FindScreenJson(*boardDoc, number);
        if (scn.isNull()) {
            logE("Embedded screen not found in the board JSON.");
            return false;
        }
        logIf("Screen %d (standalone, embedded objects).", number);
        ParseObjects(scn);
    } else if (sc.name[0]) {
        // Multi: download the screen file (Objects at its root).
        String screenJson = fetchBoardJson(String(sc.name));
        if (screenJson.length() == 0) {
            logE("Screen file download failed.");
            return false;
        }
        JsonDocument sdoc;
        if (deserializeJson(sdoc, screenJson)) {
            logE("Invalid screen JSON.");
            return false;
        }
        logIf("Screen %d (multi, file %s).", number, sc.name);
        ParseObjects(sdoc.as<JsonObject>());
    } else {
        logE("Screen has no name and is not embedded.");
        return false;
    }

    // Full redraw of the newly loaded screen.
    DrawObjects(true);
    logIf("[screen] #%d loaded: rotation=%d invert=%d refresh=%lu objects=%d",
          number, sc.rotation, sc.invert, g_screenRefresh, g_objectCount);
    return true;
}

// ---------------------------------------------------------------
// Web hooks (declared in web.h, implemented here).
//
// WebChangeScreen: switches the current screen without touch, e.g.
//   http://<ip>/?screen=2
// The new screen number is taken from g_screens[] (validated by
// LoadScreen -> FindScreen) and drawn with the board JSON g_boardDoc.
//
// WebReloadBoard: re-downloads the board JSON (same origin logic as
// setup: SD first, then JSON_URL network) and re-parses datasources,
// log flags, timezone and screens — then reloads the current screen.
//   http://<ip>/?reload=true
// ---------------------------------------------------------------
bool WebChangeScreen(int number)
{
    int idx = FindScreen(number);
    if (idx < 0) {
        logWf("[web] Screen %d not found.", number);
        return false;
    }
    if (number == g_currentScreen) {
        logIf("[web] Already on screen %d.", number);
        return true;
    }

    EngineLock();
    logIf("[web] screen->%d free heap before = %u B", number, (unsigned)ESP.getFreeHeap());
    bool ok = LoadScreen(number, &g_boardDoc);
    EngineUnlock();
    if (ok) g_currentScreen = number;
    logIf("[web] screen -> %d (ok=%d), free heap after = %u B",
          number, ok, (unsigned)ESP.getFreeHeap());
    return ok;
}

bool WebReloadBoard()
{
    logIf("[web] reload: free heap before = %u B", (unsigned)ESP.getFreeHeap());

    String json = loadBoardJson();
    if (json.length() == 0) {
        logE("[web] reload: failed to load JSON.");
        return false;
    }

    EngineLock();

    g_boardDoc.clear();
    if (deserializeJson(g_boardDoc, json)) {
        EngineUnlock();
        logE("[web] reload: invalid JSON.");
        return false;
    }
    json = String();   // release the downloaded body before parsing

    // Re-parse everything derived from the board JSON.
    ParseDataSources(g_boardDoc);
    ParseLogFlags(g_boardDoc);
    ParseScreens(g_boardDoc);

    // Drop the old datasource/entity cache — staleness from the
    // previous board must not survive (values re-fetch on the next tick).
    ResetDataCaches();

    // Timezone (optional) — re-apply only when it actually changed
    // (configTzTime re-runs sntp_init; re-calling it on every reload
    // would spawn extra SNTP tasks/allocations needlessly).
    const char* tzJson = g_boardDoc["TIMEZONE"];
    if (tzJson) {
        char posixTz[36];
        timeZoneToPosix(tzJson, posixTz, sizeof(posixTz));
        if (g_timeZone != posixTz) {
            g_timeZone = posixTz;
            setenv("TZ", g_timeZone.c_str(), 1);
            tzset();
            configTzTime(g_timeZone.c_str(), "pool.ntp.org", "time.google.com");
            logIf("[web] Timezone: %s -> %s", tzJson, g_timeZone.c_str());
        }
    }

    // Reload the screen that was on the display (fall back to #1).
    int target = (FindScreen(g_currentScreen) >= 0) ? g_currentScreen : 1;
    bool ok = LoadScreen(target, &g_boardDoc);
    if (ok) g_currentScreen = target;

    EngineUnlock();

    logIf("[web] reload done (screen %d, ok=%d), free heap after = %u B",
          target, ok, (unsigned)ESP.getFreeHeap());
    return ok;
}

// ---------------------------------------------------------------
// Setup: hardware init, secrets, log, board JSON and engine.
// ---------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    // Screen init (no UI yet, but needed for TFT log output).
    tft.init();
    tft.invertDisplay(true);
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    logSetTFT(true);

    LoadSecretsFromSD();   // must come first: reads HOSTNAME from the SD
    LogInit(HOSTNAME);     // opens /logs/<HOSTNAME>.log

    delay(2000);           // settle before the first network request

    logI("=== dash001 ===");
    logIf("Firmware build #%d", BUILD_NR);
    logIf("SSID: %s", WIFI_SSID);
    logIf("Log: " LOG_DIR "/%s.log", HOSTNAME);
    logIf("Images: %d", IMAGE_TABLE_SIZE);

    String json = loadBoardJson();

    // Even when the JSON comes from the SD, the datasources (weather/HA)
    // need the network — make sure the WiFi is up.
    if (!wifiConnected()) {
        wifiConnect(WIFI_SSID, WIFI_PASSWORD);
    }

    // Network hostname: the router/DHCP sees <HOSTNAME> instead of an
    // ESP32 default id (keeps the DHCP name stable across resets).
    WiFi.setHostname(HOSTNAME);

    // NTP for the "function:Clock"/"function:Date" objects. Uses the
    // timezone from JSON ("TIMEZONE") or the default (Lisbon) so DST is
    // applied automatically (WEST +1h in summer).
    setenv("TZ", g_timeZone.c_str(), 1);
    tzset();
    configTzTime(g_timeZone.c_str(), "pool.ntp.org", "time.google.com");

    if (json.length() > 0) {
        logIf("=== JSON %s (%u bytes) ===", JSON_FILE, (unsigned)json.length());
        Serial.print(json);
        Serial.println();

        g_boardDoc.clear();
        DeserializationError err = deserializeJson(g_boardDoc, json);
        if (err) {
            logEf("Invalid JSON: %s", err.c_str());
            return;
        }

        // Parse the DataSources (always on the board JSON root) and
        // the log flags, then store the array of screens.
        ParseDataSources(g_boardDoc);
        ParseLogFlags(g_boardDoc);

        // Physical touch navigation — ALWAYS active, like the upstream
        // Example: LEFT tap -> previous screen, RIGHT tap -> next one.
        // The board JSON flag (touchEnable/TouchEnable/Touch/touch) is
        // best-effort; navigation no longer depends on it being present.
        g_touchOn = g_boardDoc["touchEnable"] | g_boardDoc["TouchEnable"]
                   | g_boardDoc["Touch"]     | g_boardDoc["touch"];
        TouchInit();   // always on — tap log goes out now, navigation
                      // (left/right) always acts (like the Example).
        logIf("[touch] ready.");
        ParseScreens(g_boardDoc);

        // Timezone can come from the JSON root ("TIMEZONE": "Europe/Lisbon"
        // or any <region>/<city>) so the location can be changed without
        // recompiling. Converted to the POSIX TZ string and re-applied to
        // TZ+SNTP (falls back to the default when absent/unknown).
        const char* tzJson = g_boardDoc["TIMEZONE"];
        if (tzJson) {
            char posixTz[36];
            timeZoneToPosix(tzJson, posixTz, sizeof(posixTz));
            g_timeZone = posixTz;
            setenv("TZ", g_timeZone.c_str(), 1);
            tzset();
            configTzTime(g_timeZone.c_str(), "pool.ntp.org", "time.google.com");
            logIf("Timezone: %s -> %s", tzJson, g_timeZone.c_str());
        }

        // Report the number of screens found in this board JSON
        // (e.g. simple.json has a single screen).
        logIf("Screens found: %d", g_screenCount);

        // The TFT now belongs to the UI — stop logging on it
        // (Serial + SD keep working, the first FillScreen clears it).
        logSetTFT(false);

        // Load (and draw) the first screen. Standalone screens use the
        // board `doc`; multi screens fetch their own file.
        if (g_screenCount > 0) {
            logIf("Loading screen #1 of %d...", g_screenCount);
            if (!LoadScreen(1, &g_boardDoc)) {
                logE("Failed to load screen 1.");
            }
            g_currentScreen = 1;
        }
    }

    // Detect the number of cores and start the matching mode.
    EngineInit();

    // Tiny web-UI (screen change / reload / restart). Needs the WiFi up.
    WebInit();
}

// Main loop: delegates to the engine (sequential or dual-core mode).
void loop()
{
    WebLoop();        // serve the web-UI (handleClient — non blocking)

    // Touch navigation — active only when the board JSON root has
    // "Touch": true. LEFT tap -> previous screen, RIGHT tap -> the
    // next one, through WebChangeScreen() (the same safe navigation
    // port the web-UI uses, so the engine lock + LoadScreen always
    // apply and no mid-draw corruption can happen).
    if (g_touchOn && CheckTouch(g_currentRotation)) {
        // Navigate the real screen list (g_screens[]) with wraparound:
        // tapping LEFT goes to the previous screen, RIGHT to the next one,
        // wrapping around the ends (last -> first, first <- last).
        int idx = FindScreen(g_currentScreen);
        if (idx < 0) idx = 0;   // safety: unknown current -> treat as #1
        if (g_touchX < TOUCH_W / 2) {
            idx = (idx <= 0) ? g_screenCount - 1 : idx - 1;
        } else {
            idx = (idx >= g_screenCount - 1) ? 0 : idx + 1;
        }
        WebChangeScreen(g_screens[idx].number);
    }

    EngineLoop();
}