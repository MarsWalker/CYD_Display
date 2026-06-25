// ===========================================================
// Touch Driver Selection Configuration
// ===========================================================
// Choose the specific hardware touch controller driver below:
#define TOUCH_DRIVER_XPT2046 // Yellow display module (2.8 inch variant)
//#define TOUCH_DRIVER_XPT2046_2_4 // Yellow display module (2.4 inch variant)
//#define TOUCH_DRIVER_XPT2046_3_2 // Yellow display module (3.2 inch variant)

// ===========================================================
// Core Core & Network Framework Includes
// ===========================================================
#include <WiFi.h>              // Wi-Fi radio core library for ESP32
#include <WiFiUdp.h>           // UDP communication layer for NTP requests
#include <NTPClient.h>         // Network Time Protocol client library
#include <TFT_eSPI.h>          // High-performance graphics driver library
#include <TimeLib.h>           // Time manipulation and translation utilities
#include <HTTPClient.h>        // HTTP protocol client for web/server requests
#include <ArduinoJson.h>       // Fast JSON parsing and serialization stream
#include <PubSubClient.h>      // MQTT client container for machine messaging

// ===========================================================
// Global System Instances & Variables
// ===========================================================
TFT_eSPI tft = TFT_eSPI();     // Hardware graphics engine controller instance

int CurrentScreen          = 1; // Pointer state tracking active runtime display page
int totalScreens           = 1; // Evaluated total dynamic pages available in configuration
unsigned long LastUpdate   = 0; // Runtime timestamp tracking the last frame refresh cycle (ms)
unsigned long refreshRate  = 1000; // Dynamic window timer adjusting interface loop delay (ms)
bool needFullRedraw        = true; // Semaphore forcing a complete canvas reset on state switches

const long utcOffsetInSeconds = 3600; // Timezone shift allocation tracker (e.g., UTC +1 hour)
bool g_HAConnected = false;           // Logic handshake flag checking connection with Home Assistant

WiFiClient espClient;                 // Base networking stack instance handler
PubSubClient mqttClient(espClient);   // Attaches MQTT messaging engine onto the network stack

WiFiUDP ntpUDP;                       // Binds a raw network channel for timeline server pings
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds); // NTP time tracker setup

// ===========================================================
// Dynamic Project Module Includes (Compilation order is strict!)
// ===========================================================
#include "secrets.h"          // Passwords, Private API Keys, URLs, and Identity setups
#include "wifi_functions.h"   // Wi-Fi operational management and handshake loops
#include "colors.h"           // Color translation tables and RGB565 format conversion engines
#include "images.h"           // Flash-optimized binary bitmaps (PROGMEM array registries)
#include "draw_functions.h"   // Custom primitive drawing algorithms on the canvas
#include "functions.h"        // Native micro evaluation utilities and runtime string parsers
#include "touch.h"            // SPI touch capture abstraction layer
#include "json_screen.h"      // Dynamic interface compiler parsing structural layout streams

// ===========================================================
// Microcontroller Setup Entrypoint (Runs once on boot)
// ===========================================================
void setup()
{
    // Open hardware peripheral UART line for code debugging output
    Serial.begin(115200);
    Serial.println("\n=== Starting ===");

    // Establish link connection to the local network access point router
    ConnectWifi();

    // Fire up hardware SPI communication channels to initialize screen registers
    tft.init();
    tft.invertDisplay(true);   // Flips standard color profiles to fix variant color issues
    tft.setRotation(1);        // Configures standard horizontal wide landscape mode layout
    tft.fillScreen(TFT_BLACK); // Clears the graphic frame buffer artifact garbage to black

    // Register graphic target converter function to render JPEG stream structures
    TJpgDec.setCallback(tft_output);
        
    // Boot up the synchronization client module targeting internet time servers
    timeClient.begin();
    timeClient.update();

    // Use values imported from secrets.h to parse dynamic target server JSON layouts
    String fullConfigURL = String(JSON_URL) + String(HOSTNAME) + ".json";

    // Query configuration matrix files to resolve dynamic compilation boundaries
    totalScreens = CountScreens(fullConfigURL.c_str());
    if (totalScreens == 0) totalScreens = 1; // Security fallback assertion map

    Serial.printf("Number of screens: %d\n", totalScreens);

    // Trigger explicit display initialization requirements flags
    needFullRedraw = true;
    
    // Boot peripheral touch controller interface loops
    TouchInit();
}

// ===========================================================
// Main Operational Loop (Runs infinitely)
// ===========================================================
void loop()
{
    unsigned long Now = millis(); // Benchmark system operational reference timer (ms)

    // Assemble uniform server query path string definitions dynamically
    String fullMasterUrl = String(JSON_URL) + String(HOSTNAME) + ".json";

    // -------------------------------------------------------
    // 1. Process Hardware Touch Events
    // -------------------------------------------------------
    if (CheckTouch())
    {
        // Extract layout tracking coordinate mapped zones from screen profiles
        String acaoMapeada = GetTouchActionForScreen(touchX, touchY);
        acaoMapeada.trim();
    
        // Parse conditional target jump parameters if a macro switch matches
        if (acaoMapeada.startsWith("goto:")) {
            int targetScreen = acaoMapeada.substring(5).toInt();
            Serial.print("GOTO ->");
            Serial.println(targetScreen);
            
            // Boundary checking verification constraint
            if (targetScreen > 0 && targetScreen <= totalScreens) {
                CurrentScreen = targetScreen;
            }
        } else {
            // Default sequential layout iteration strategy fallback if unmapped
            CurrentScreen++;
            if (CurrentScreen > totalScreens) CurrentScreen = 1;
        }
        
        // Assert canvas refresh semaphore flags to process data stream mutations
        needFullRedraw = true;
        LastUpdate = 0; // Force immediate update cycle execution bypass constraints
    }

    // -------------------------------------------------------
    // 2. Scheduled Screen Rendering Cycle
    // -------------------------------------------------------
    if (Now - LastUpdate >= refreshRate)
    {
        LastUpdate = Now; // Update benchmark tracking timer reference metric

        // Compiles and displays active user components over structural matrix definitions.
        // Returns dynamically required operational delay frames for loop rate adjustments.
        refreshRate = LoadAndDrawScreen(
            fullMasterUrl.c_str(),
            CurrentScreen,
            needFullRedraw);

        // Lower layout initial construction flag metrics post render operation completions
        if (needFullRedraw) needFullRedraw = false;
    }
}