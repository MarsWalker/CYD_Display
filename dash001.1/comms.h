#ifndef COMMS_H
#define COMMS_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "log.h"

/*
 * comms.h
 * -------------------------------------------------------
 * WiFi connection + HTTP/HTTPS requests for dash001.
 *
 * The client is chosen automatically from the URL:
 *   http://  -> WiFiClient
 *   https:// -> WiFiClientSecure (setInsecure: no CA validation)
 *
 * Usage:
 *   wifiConnect(WIFI_SSID, WIFI_PASSWORD);
 *   int code;
 *   String body = httpGet(String(JSON_URL) + HOSTNAME + ".json", &code);
 *   httpPost("https://api.telegram.org/bot.../sendMessage",
 *            payload, "application/json");
 *
 * All HTTP functions accept an optional `quiet` flag: when true,
 * the request/status debug lines are suppressed (used to hide the
 * traffic of datasources whose log is disabled).
 * -------------------------------------------------------
 */

#ifndef HTTP_TIMEOUT_MS
#define HTTP_TIMEOUT_MS 15000UL   // request timeout (ms)
#endif

// ---------------------------------------------------------------
// WiFi
// ---------------------------------------------------------------

// Connects to the WiFi (blocking until connected or timeout).
// Returns true if connected.
inline bool wifiConnect(const char* ssid, const char* pass, uint32_t timeoutMs = 20000)
{
    if (!ssid || ssid[0] == '\0') {
        logE("[wifi] Empty SSID");
        return false;
    }

    logI(String("[wifi] Connecting to \"") + ssid + "\"");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);

    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - t0) < timeoutMs) {
        delay(500);
        Serial.print('.');          // dots only on Serial (don't fill SD/TFT)
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        logI(String("[wifi] Connected, IP: ") + WiFi.localIP().toString());
        return true;
    }
    logEf("[wifi] Failed to connect (%lu ms)", (unsigned long)timeoutMs);
    return false;
}

inline bool wifiConnect(const String& ssid, const String& pass, uint32_t timeoutMs = 20000)
{
    return wifiConnect(ssid.c_str(), pass.c_str(), timeoutMs);
}

inline bool wifiConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

inline void wifiDisconnect()
{
    WiFi.disconnect();
    logI("[wifi] Disconnected");
}

inline String wifiIP()
{
    return wifiConnected() ? WiFi.localIP().toString() : String();
}

inline int32_t wifiRSSI()
{
    return wifiConnected() ? WiFi.RSSI() : 0;
}

// ---------------------------------------------------------------
// HTTP: shared core. The `client` (created by the caller according
// to http/https) must live until http.end() — hence the template.
// ---------------------------------------------------------------
enum HttpVerb { HTTPV_GET, HTTPV_POST };

template <typename ClientT>
inline String _httpRun(ClientT& client, HttpVerb verb, const String& url,
                       const String& body, const char* contentType,
                       int* httpCode, uint32_t timeoutMs, bool quiet = false)
{
    if (httpCode) *httpCode = -1;

    if (!wifiConnected()) {
        if (!quiet) logE("[http] WiFi not connected");
        return String();
    }
    if (timeoutMs > 60000UL) timeoutMs = 60000UL;

    HTTPClient http;
    http.setConnectTimeout((int32_t)timeoutMs);
    http.setTimeout((uint16_t)timeoutMs);
    http.setReuse(false);

    if (!quiet) logDf("[http] %s %s", (verb == HTTPV_GET) ? "GET" : "POST", url.c_str());

    if (!http.begin(client, url)) {
        if (!quiet) logE("[http] Failed to start connection");
        return String();
    }

    int code = -1;
    if (verb == HTTPV_GET) {
        code = http.GET();
    } else {
        if (contentType && contentType[0]) http.addHeader("Content-Type", contentType);
        code = http.POST(body);
    }

    String payload;
    if (code > 0) {
        payload = http.getString();
        if (!quiet) {
            logDf("[http] status %d, %u bytes", code, (unsigned)payload.length());
            if (code < 200 || code >= 300) {
                logWf("[http] unexpected code: %d", code);
            }
        }
    } else {
        if (!quiet) logEf("[http] error: %s", http.errorToString(code).c_str());
    }

    http.end();
    if (httpCode) *httpCode = code;
    if (code <= 0) return String();   // a real error (kept distinct from an empty 200)
    return payload;
}

inline bool _httpIsHttps(const String& url)
{
    return url.startsWith("https://");
}

// ---------------------------------------------------------------
// HTTP GET. `httpCode` (optional) returns the response code.
// Returns the body, or String() on a network error.
// ---------------------------------------------------------------
inline String httpGet(const String& url, int* httpCode = nullptr,
                      uint32_t timeoutMs = HTTP_TIMEOUT_MS, bool quiet = false)
{
    if (_httpIsHttps(url)) {
        WiFiClientSecure client;
        client.setInsecure();               // no CA validation (simple setup)
        return _httpRun(client, HTTPV_GET, url, String(), nullptr, httpCode, timeoutMs, quiet);
    }
    WiFiClient client;
    return _httpRun(client, HTTPV_GET, url, String(), nullptr, httpCode, timeoutMs, quiet);
}

// ---------------------------------------------------------------
// HTTP GET with an extra header (e.g. "Authorization: Bearer <token>"
// for Home Assistant). Takes the header value and name.
// ---------------------------------------------------------------
template <typename ClientT>
inline String _httpRunWithHeader(ClientT& client, const String& url,
                                 const String& headerName, const String& headerValue,
                                 int* httpCode, uint32_t timeoutMs, bool quiet = false)
{
    if (!wifiConnected()) {
        if (!quiet) logE("[http] WiFi not connected");
        return String();
    }
    if (timeoutMs > 60000UL) timeoutMs = 60000UL;

    HTTPClient http;
    http.setConnectTimeout((int32_t)timeoutMs);
    http.setTimeout((uint16_t)timeoutMs);
    http.setReuse(false);

    if (!quiet) logDf("[http] GET %s (+%s)", url.c_str(), headerName.c_str());

    if (!http.begin(client, url)) {
        if (!quiet) logE("[http] Failed to start connection");
        return String();
    }
    if (headerName.length() > 0) http.addHeader(headerName, headerValue);

    int code = http.GET();

    String payload;
    if (code > 0) {
        payload = http.getString();
        if (!quiet) {
            logDf("[http] status %d, %u bytes", code, (unsigned)payload.length());
            if (code < 200 || code >= 300) {
                logWf("[http] unexpected code: %d", code);
            }
        }
    } else {
        if (!quiet) logEf("[http] error: %s", http.errorToString(code).c_str());
    }

    http.end();
    if (httpCode) *httpCode = code;
    if (code <= 0) return String();
    return payload;
}

// GET with a Bearer token (Home Assistant): addHeader("Authorization", "Bearer "+token)
inline String httpGetBearer(const String& url, const String& token,
                            int* httpCode = nullptr,
                            uint32_t timeoutMs = HTTP_TIMEOUT_MS, bool quiet = false)
{
    if (_httpIsHttps(url)) {
        WiFiClientSecure client;
        client.setInsecure();
        return _httpRunWithHeader(client, url, "Authorization",
                                  "Bearer " + token, httpCode, timeoutMs, quiet);
    }
    WiFiClient client;
    return _httpRunWithHeader(client, url, "Authorization",
                              "Bearer " + token, httpCode, timeoutMs, quiet);
}

// ---------------------------------------------------------------
// HTTP POST (body + Content-Type). Default: application/json.
// ---------------------------------------------------------------
inline String httpPost(const String& url, const String& body,
                       const char* contentType = "application/json",
                       int* httpCode = nullptr,
                       uint32_t timeoutMs = HTTP_TIMEOUT_MS)
{
    if (_httpIsHttps(url)) {
        WiFiClientSecure client;
        client.setInsecure();
        return _httpRun(client, HTTPV_POST, url, body, contentType, httpCode, timeoutMs);
    }
    WiFiClient client;
    return _httpRun(client, HTTPV_POST, url, body, contentType, httpCode, timeoutMs);
}

#endif // COMMS_H
