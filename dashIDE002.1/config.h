/**
 * @file config.h
 *
 * Public configuration loader; no deployment credentials belong in this file.
 * secrets.h supplies local defaults and is deliberately absent from the public
 * package. Copy secrets_template.h to create it. An optional SD JSON file can
 * override individual values after startup, before log naming and Wi-Fi setup.
 * The backing String objects must outlive the const char* pointers assigned to
 * configuration values; pointing into the temporary JSON document would dangle.
 */

#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "board_storage.h"
#include "log.h"
#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "Copy secrets_template.h to secrets.h and configure your credentials."
#endif

// Heap storage for the overrides coming from /secrets.json
// (only allocated if present).
static String ovWIFI_SSID, ovWIFI_PASSWORD, ovHA_URL, ovHA_TOKEN;
static String ovTELEGRAM_TOKEN, ovTELEGRAM_CHAT_ID, ovJSON_URL, ovJSON_FILE, ovHOSTNAME;

// ============================================================================
// SD CONFIG
// ============================================================================
#ifndef SD_CS
#define SD_CS 4
#endif
#define SECRETS_FILE "/secrets.json"

// ============================================================================
// LOADER: Override the defaults with the secrets.json from the SD card
// (if present). Mounts the SD via sdEnsureMount() from log.h.
// ============================================================================
/**
 * Overlay local defaults from an optional SD document.
 * Return true only when a file was opened and parsed successfully. Missing
 * fields retain defaults. File/read/parse failures leave the application able
 * to continue. Strings are copied before the temporary document is destroyed.
 */
bool LoadSecretsFromSD() {
    if (!sdEnsureMount()) {
        writeLogln("[secrets] SD init failed. Using defaults.");
        return false;
    }
    if (!BoardSD.exists(SECRETS_FILE)) {
        writeLogln("[secrets] secrets.json not found. Using defaults.");
        return false;
    }

    File f = BoardSD.open(SECRETS_FILE, FILE_READ);
    if (!f) {
        writeLogln("[secrets] Failed to open secrets.json. Using defaults.");
        return false;
    }

    StaticJsonDocument<2048> doc;
    if (deserializeJson(doc, f)) {
        f.close();
        writeLogln("[secrets] Invalid JSON. Using defaults.");
        return false;
    }
    f.close();

    /**
     * Each assignment first copies text into persistent backing storage, then
     * publishes that storage's c_str() pointer. Do not assign a pointer borrowed
     * from doc: doc and its strings are destroyed when this function returns.
     */
    // Override each default only for the fields present in the file.
    const char* tmp;
    if ((tmp = doc["WIFI_SSID"]))         { ovWIFI_SSID = tmp;         WIFI_SSID = ovWIFI_SSID.c_str(); }
    if ((tmp = doc["WIFI_PASSWORD"]))     { ovWIFI_PASSWORD = tmp;     WIFI_PASSWORD = ovWIFI_PASSWORD.c_str(); }
    if ((tmp = doc["HA_URL"]))            { ovHA_URL = tmp;            HA_URL = ovHA_URL.c_str(); }
    if ((tmp = doc["HA_TOKEN"]))          { ovHA_TOKEN = tmp;          HA_TOKEN = ovHA_TOKEN.c_str(); }
    if ((tmp = doc["TELEGRAM_TOKEN"]))    { ovTELEGRAM_TOKEN = tmp;    TELEGRAM_TOKEN = ovTELEGRAM_TOKEN.c_str(); }
    if ((tmp = doc["TELEGRAM_CHAT_ID"]))  { ovTELEGRAM_CHAT_ID = tmp;  TELEGRAM_CHAT_ID = ovTELEGRAM_CHAT_ID.c_str(); }
    if ((tmp = doc["JSON_URL"]))          { ovJSON_URL = tmp;          JSON_URL = ovJSON_URL.c_str(); }
    if ((tmp = doc["JSON_FILE"]))         { ovJSON_FILE = tmp;         JSON_FILE = ovJSON_FILE.c_str(); }
    if ((tmp = doc["HOSTNAME"]))          { ovHOSTNAME = tmp;          HOSTNAME = ovHOSTNAME.c_str(); }

    writeLogln("[secrets] Loaded from BoardSD.");
    return true;
}

