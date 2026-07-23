#include <TFT_eSPI.h>          // High-performance graphics driver library
#include <WiFi.h>              // Wi-Fi radio core library for ESP32
#include <WiFiUdp.h>           // UDP communication layer for NTP requests
#include <NTPClient.h>         // Network Time Protocol client library
#include <TimeLib.h>           // Time manipulation and translation utilities
#include <PubSubClient.h>      // MQTT client container for machine messaging

WiFiClient espClient;                 // Base networking stack instance handler
PubSubClient mqttClient(espClient);   // Attaches MQTT messaging engine onto the network stack

// ===========================================================
// Dynamic Project Module Includes (Compilation order is strict!)
// ===========================================================
#include "secrets.h"
#include "flog.h"
// void resetLog()
// void logCheckWrap()
// void writeLog(const String &msg)
// void writeLogln(const String &msg)
#include "fwifi.h"
// void ConnectWifi()
#include "images_colors.h"
// const ImageEntry IMAGE_TABLE[]
// uint16_t GetColorFromName(String nome)
// int FindImageIndex(String imgName)
#include "gvars.h"
#include "structures.h"    
#include "fconfig.h"
// JsonVariant ResolveJsonPath(JsonVariant root, const String& path)
// bool LoadConfigurationFromJson(JsonDocument& doc)
// WeatherDataSource* FindWeatherSource(const String& id)
// WeatherCode* FindWeatherCode(WeatherDataSource& ws, int code)
// ScreenInfo* FindScreenByNumber(int number)
#include "functions.h"          
// inline String httpRequest(const String &url, int *httpCode = nullptr)
// bool UpdateWeatherDataSource(WeatherDataSource &ws)
// void UpdateAllWeatherSources()
// String Clock()
// String Date()
// String Uptime()
// bool IsWIFIConnected()
// String WifiRSSI()
// String FreeRAM()
// String MyIP()
// bool IsMQTTConnected()
// bool IsHAConnected()
// String ResolveImageValue(String texto)
// String ResolveTextValue(String texto)
// bool ResolveBoolValue(String texto)

#include "fdraw.h"     
// void DrawText( int x, int, String text, uint16_t color, int font)
// void FillScreen(uint16_t color)
// void DrawImage( int x,  int y, const unsigned char* bitmap, int w, int h, uint16_t color, uint8_t zoom)
// void DrawSmartImage(int x, int y, String source, uint16_t color, uint8_t zoom, bool isColorDefined)
#include "fscreen.h"   
// inline bool LoadScreenFromJson(const String& json)
// bool LoadScreen(const String& url)
// inline void DrawObjectFromJson(JsonObject obj)
// inline void DrawScreen()
#include "fweb.h"   
// void HandleRootRequest()
// void HandleNotFoundRequest()
// void InitWebServer()
// void HandleWebServer()
#include "ftasks.h"
// void DataTask()
// void DisplayTask()

// ===========================================================
// Global System Instances & Variables
// ===========================================================
TFT_eSPI tft = TFT_eSPI();     // Hardware graphics engine controller instance

const long utcOffsetInSeconds = 3600; // Timezone shift allocation tracker (e.g., UTC +1 hour)
WiFiUDP ntpUDP;                       // Binds a raw network channel for timeline server pings
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds); // NTP time tracker setup

TaskHandle_t DataTaskHandle = nullptr;
TaskHandle_t DisplayTaskHandle = nullptr;

void setup()
{
    Serial.begin(115200);

    tft.init();
    tft.invertDisplay(true);
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    resetLog();
    writeLogln("=== Starting ===");

    ConnectWifi();
    InitWebServer(); 

    timeClient.begin();
    timeClient.update();

    String fullConfigURL = String(JSON_URL) + String(HOSTNAME) + ".json";

    int code;
    String configJson = httpRequest(fullConfigURL, &code);

    if (code == HTTP_CODE_OK)
    {
        JsonDocument configDoc; // ou DynamicJsonDocument configDoc(4096); se estiveres em ArduinoJson v6
        DeserializationError err = deserializeJson(configDoc, configJson);

        if (!err)
        {
            LoadConfigurationFromJson(configDoc);
            writeLogln("Screens carregados: " + String(Config.screenCount));
            for (int i = 0; i < Config.screenCount; i++)
            {
                writeLogln("  [" + String(i) + "] Number=" + String(Config.screens[i].Number) +
                            " Name=" + Config.screens[i].Name);
            }

            writeLogln("Configuration loaded. Weather sources: " + String(Config.weatherCount));

            // Primeira atualização imediata dos dados, para não esperar pelo primeiro refresh
            UpdateAllWeatherSources();
        }
        else
        {
            writeLogln("Configuration JSON parse error: " + String(err.c_str()));
        }
    }
    else
    {
        writeLogln("Configuration HTTP error: " + String(code));
    }

#if CONFIG_FREERTOS_UNICORE
    writeLogln("Single-core CPU detected");

    DrawScreen();

#else
    writeLogln("Dual-core CPU detected");

    DrawScreen();

    xTaskCreatePinnedToCore(DataTaskWrapper, "Data", 8192, NULL, 1, &DataTaskHandle, 0);          // Core 0
    xTaskCreatePinnedToCore(DisplayTaskWrapper, "Display", 8192, NULL, 1, &DisplayTaskHandle, 1);          // Core 1
#endif
}

void loop()
{
#if CONFIG_FREERTOS_UNICORE
    DataTask();
    DisplayTask();
#else
    UpdateAllWeatherSources();
    vTaskDelay(pdMS_TO_TICKS(1000));
#endif
}