/**
 * @file secrets_template.h
 *
 * Public declaration template for local deployment settings.
 * Copy this file to secrets.h; edit the copy and keep it out of version control.
 * The pointers may be redirected to persistent String storage by config.h when
 * an SD override exists. Do not replace them with pointers into temporary JSON.
 * The public layout URL is intentionally separate from Wi-Fi and HA secrets.
 */

#pragma once
// Copy to secrets.h and edit locally. Never publish secrets.h or secrets.json.
// Values can be overridden by /secrets.json on the SD card.
const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";
const char* HA_URL = "http://homeassistant.local:8123";
const char* HA_TOKEN = "";
// Reserved configuration fields; Telegram alerts are not implemented.
const char* TELEGRAM_TOKEN = "";
const char* TELEGRAM_CHAT_ID = "";
const char* JSON_URL = "https://raw.githubusercontent.com/MarsWalker/CYD_Display/main/dashboards/";
const char* JSON_FILE = "simple.v6.json";
const char* HOSTNAME = "cyd-dashboard";
