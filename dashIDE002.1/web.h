/**
 * @file web.h
 *
 * Minimal HTTP control page served by the ESP32 on port 80.
 * Routes are handled synchronously from loop(), and screen/reload operations
 * call sketch hooks that acquire the engine mutex. Restart is deferred until
 * after the response has been sent. The interface has no authentication and
 * its GET query parameters can change device state; it is intended for a
 * trusted local network, not direct exposure as a public administration API.
 */

#ifndef WEB_H
#define WEB_H

#include <Arduino.h>
#include <WebServer.h>

#include "log.h"
#include "config.h"
#include "comms.h"
#include "screens.h"
#include "build.h"

/*
 * web.h
 * -------------------------------------------------------
 * A tiny web-UI served on the device so the touchscreen is not
 * required to control it:
 *
 *   http://<ip>/                 mini-help page (when no query data)
 *   http://<ip>/?screen=N        switch to screen N (any order)
 *   http://<ip>/?restart=true    reboot the board
 *   http://<ip>/?reload=true     re-fetch + re-parse the board JSON
 *
 * WebChangeScreen() and WebReloadBoard() are hooks implemented in
 * dashIDE002.1.ino (same pattern as EngineDataUpdate/EngineDraw). Call
 * WebInit() after the WiFi is up, and WebLoop() from loop().
 * -------------------------------------------------------
 */

static WebServer g_webServer(80);
static bool      g_webRestartPending = false;

// Number of the screen currently loaded (kept here, used by dashIDE002.1.ino).
static int g_currentScreen = 1;

// Rotation of the CURRENTLY loaded screen (from the board JSON
// "Rotation" of that screen). Kept here so touch.h can align its
// axes with the screen actually on display.
static int g_currentRotation = 1;

// Hooks implemented in the main sketch.
bool WebChangeScreen(int number);
bool WebReloadBoard();

// Builds the mini-help page (also used for action results).
/**
 * Build the small device control page from current configuration.
 * JSON_URL, JSON_FILE and screen names are rendered as informational strings;
 * the page is not a dashboard editor or a general-purpose template engine.
 */
inline String WebHelpPage(const char* msg)
{
    String html;
    html.reserve(1024);

    html += F("<!DOCTYPE html><html><head><meta charset='utf-8'>"
              "<meta name='viewport' content='width=device-width,initial-scale=1'>"
              "<title>dashIDE002.1</title>"
              "<style>"
              "body{font-family:system-ui,sans-serif;background:#111;color:#eee;"
              "max-width:480px;margin:0 auto;padding:16px}"
              "a{color:#4fc3f7;text-decoration:none;display:block;padding:6px 0;"
              "border-bottom:1px solid #333}"
              "h2{color:#ffca28}.msg{background:#1b3a2b;padding:8px;border-radius:6px}"
              "small{color:#888}</style></head><body><h2>dashIDE002.1</h2>");

    if (msg && msg[0]) {
        html += F("<p class='msg'>");
        html += msg;
        html += F("</p>");
    }

    html += F("<h3>Screen source</h3>");
    html += F("<p style='font-size:12px;word-wrap:break-word;overflow-wrap:break-word'>"
              "<b>URL:</b> ");
    html += String(JSON_URL);
    html += F("<br><b>File:</b> ");
    html += String(JSON_FILE);
    html += F("</p>");

    html += F("<h3>Screens</h3>");
    for (int i = 0; i < g_screenCount; i++) {
        html += F("<a href='/?screen=");
        html += String(g_screens[i].number);
        html += F("'>Screen ");
        html += String(g_screens[i].number);
        if (g_screens[i].name[0]) {
            html += F(" &middot; ");
            html += g_screens[i].name;
            html += F("<br><small>");
            html += String(JSON_URL);
            html += g_screens[i].name;
            html += F("</small>");
        } else {
            html += F("<br><small>");
            html += String(JSON_URL);
            html += String(JSON_FILE);
            html += F("</small>");
        }
        html += F("</a>");
    }

    html += F("<h3>Options</h3>"
              "<a href='/?reload=true'>Reload JSON</a>"
              "<a href='/?restart=true'>Restart device</a>"
              "<hr><small>dashIDE002.1 build ");
    html += String(BUILD_NR);
    html += F("</small></body></html>");
    return html;
}

// GET / — the single entry point of the web-UI.
/**
 * Dispatch a single GET action in priority order: screen, restart, reload.
 * The first recognized action wins if several are supplied. Restart marks a
 * pending flag so the response can leave before the MCU is reset.
 */
inline void WebHandleRoot()
{
    // ?screen=N  -> switch screen (validates against g_screens[]).
    if (g_webServer.hasArg("screen")) {
        int n = g_webServer.arg("screen").toInt();
        String msg;
        if (n > 0 && WebChangeScreen(n)) {
            msg = "Screen " + String(n) + " loaded.";
        } else {
            msg = "Screen " + String(n) + " not found.";
        }
        g_webServer.send(200, "text/html", WebHelpPage(msg.c_str()));
        return;
    }

    // ?restart=true -> reboot the board (respond before resetting).
    if (g_webServer.hasArg("restart")) {
        g_webServer.send(200, "text/html",
            "<html><body style='font-family:sans-serif;background:#111;color:#eee'>"
            "<h2>dashIDE002.1</h2><p>Restarting...</p></body></html>");
        g_webRestartPending = true;
        return;
    }

    // ?reload=true -> re-fetch + re-parse the board JSON, reload screen.
    if (g_webServer.hasArg("reload")) {
        bool ok = WebReloadBoard();
        g_webServer.send(200, "text/html",
            WebHelpPage(ok ? "JSON reloaded successfully."
                           : "Failed to reload JSON.").c_str());
        return;
    }

    // No query data -> mini help with the available options.
    g_webServer.send(200, "text/html", WebHelpPage(""));
}

inline void WebInit()
{
    g_webServer.on("/", HTTP_GET, WebHandleRoot);
    g_webServer.onNotFound([]() {
        g_webServer.sendHeader("Location", String("/"), true);
        g_webServer.send(302, "text/plain", "");
    });
    g_webServer.begin();
    logIf("[web] http://%s/", WiFi.localIP().toString().c_str());
}

/**
 * Service queued HTTP work on the UI loop and execute a deferred reboot.
 * Handlers may call blocking reload/source operations; this WebServer instance
 * is synchronous rather than a separate asynchronous server task.
 */
inline void WebLoop()
{
    g_webServer.handleClient();
    if (g_webRestartPending) {
        delay(200);
        ESP.restart();
    }
}

#endif // WEB_H