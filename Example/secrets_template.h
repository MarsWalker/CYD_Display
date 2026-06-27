// Rename this file to secrets.h and update the information

ifndef SECRETS_H
#define SECRETS_H

// ============================================================================
// WIFI NETWORK CREDENTIALS
// ============================================================================
const char* WIFI_SSID     = "SSID Name";            // The name (SSID) of your local Wi-Fi network
const char* WIFI_PASSWORD = "WIFI Secret Password"; // The security password for your Wi-Fi network

// ============================================================================
// HOME ASSISTANT REST API CONFIGURATION
// ============================================================================
const char* HA_URL        = "http://homeassistant.lan:8123"; // Base URL of your HA instance (IP address or local hostname)

/* * ----------------------------------------------------------------------------
 * HOME ASSISTANT LONG-LIVED ACCESS TOKEN (HA_TOKEN)
 * ----------------------------------------------------------------------------
 * WHAT IS IT?
 * The HA_TOKEN is a Long-Lived Access Token (LLAT) used for secure API 
 * authentication. Because the ESP32 interacts directly with Home Assistant's 
 * REST API to read states or execute actions, it needs to prove it is authorized.
 * Unlike browser cookies, an LLAT remains valid for 10 years, making it perfect 
 * for standalone embedded IoT hardware.
 * * STEP-BY-STEP GUIDE: HOW TO OBTAIN IT
 * 1. Log into your Home Assistant dashboard web interface.
 * 2. Click on your User Profile/Avatar icon in the bottom-left corner sidebar.
 * 3. Select the "Security" tab at the top of your profile settings page.
 * 4. Scroll down to the very bottom to the "Long-Lived Access Tokens" section.
 * 5. Click the "Create Token" button.
 * 6. Enter a memorable name for identification (e.g., "CYD Display Board").
 * 7. Click "OK" and a popup will display a very long cryptographic string.
 * 8. Copy the entire string immediately and paste it into the variable below.
 * * NOTE: Home Assistant only shows this token ONCE. If you close the window, 
 * you cannot recover it and will have to generate a new one. Treat it as a password.
 * ----------------------------------------------------------------------------
 */
const char* HA_TOKEN      = "HA_TOKEN";                      

// ============================================================================
// TELEGRAM BOT CONFIGURATION (For Alerts and Notifications)
// ============================================================================
const char* TELEGRAM_TOKEN   = "ZZZZZZZZZZ:YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY"; // Unique API token provided by BotFather
const char* TELEGRAM_CHAT_ID = "XXXXXXXXXX";                                   // Your personal or group unique chat ID identifier

// ============================================================================
// SYSTEM & DEVICE PROPERTIES
// ============================================================================
const char* JSON_URL = "https://github.com/MarsWalker/CYD_Display/tree/main/dashboards"; // URL pointing to your screen configuration repository
const char* HOSTNAME = "board_name";  // The mDNS/network name registered by this specific ESP32 board

#endif