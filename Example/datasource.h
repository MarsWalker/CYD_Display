#ifndef DATASOURCE_H
#define DATASOURCE_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ============================================================================
// EXTERNAL PROPERTIES (Provided by main configuration)
// ============================================================================
extern String ResolveTextValue(String texto); // Fallback resolution from the main layout module

// ============================================================================
// GLOBAL DATA STORAGE: WEATHER & EXTERNAL DATASOURCES CACHE
// ============================================================================
// Purpose: Manages multi-field data payloads parsed from external API JSON configurations.
struct DataSourceCache {
    String id;                    // Unique identifier for the datasource (e.g., "OpenMeteo")
    unsigned long refreshInterval;// How often this source should be fetched (in milliseconds)
    unsigned long lastFetchTime;  // Timestamp of the last successful HTTP GET request
    String temperature;           // Parsed current temperature value
    String humidity;              // Parsed current humidity value
    String pressure;              // Parsed atmospheric pressure value
    String wind;                  // Parsed wind speed value
    String code;                  // Raw numerical weather condition code
    String codeText;              // Human-readable condition string (e.g., "Light Rain")
    String codeImage;             // Internal system bitmap name reference (e.g., "rain_light")
    String codeUrl;               // Remote JPG URL reference for weather icons
};

const int MAX_DATASOURCES = 5;                  // Maximum number of active continuous external API streams
DataSourceCache dsCache[MAX_DATASOURCES];      // Allocation of the data streams cache array
int dsCacheCount = 0;                           // Counter for registered datasources

// ============================================================================
// ENGINE: PARSE DATA SOURCES FROM MASTER JSON
// ============================================================================
// Parses background JSON data tasks. Supports legacy text-only mappings and modern complex objects.
void ParseDataSourcesFromDoc(DynamicJsonDocument& doc) {
    if (!doc.containsKey("DataSources")) return;

    unsigned long agora = millis();
    JsonArray dataSources = doc["DataSources"].as<JsonArray>();

    for (JsonObject ds : dataSources) {
        String id      = ds["Id"].as<String>();
        unsigned long refresh = ds["Refresh"] | 60000; // Default refresh window to 60 seconds if empty
        String dsUrl   = ds["url"].as<String>();

        // --- STEP 1: Find or Register the DataSource in global cache allocation ---
        int idx = -1;
        for (int i = 0; i < dsCacheCount; i++) {
            if (dsCache[i].id == id) { idx = i; break; }
        }
        if (idx == -1 && dsCacheCount < MAX_DATASOURCES) {
            idx = dsCacheCount;
            dsCache[idx].id               = id;
            dsCache[idx].lastFetchTime    = 0;
            dsCache[idx].temperature      = "--";
            dsCache[idx].humidity         = "--";
            dsCache[idx].pressure         = "--";
            dsCache[idx].wind             = "--";
            dsCache[idx].code             = "0";
            dsCache[idx].codeText         = "";
            dsCache[idx].codeImage        = "";
            dsCache[idx].codeUrl          = "";
            dsCacheCount++;
        }

        if (idx == -1) continue; // Skip execution if structure is full

        dsCache[idx].refreshInterval = refresh;

        // --- STEP 2: Time-window throttle verification ---
        bool deveAtualizar = (dsCache[idx].lastFetchTime == 0) ||
                             (agora - dsCache[idx].lastFetchTime >= dsCache[idx].refreshInterval);

        if (!deveAtualizar) continue; // Skip downloading if the data is still fresh

        dsCache[idx].lastFetchTime = agora;
        Serial.printf("Updating DataSource: %s...\n", id.c_str());

        // --- STEP 3: Request Remote Payload API Data ---
        HTTPClient dsHttp;
        dsHttp.begin(dsUrl);
        dsHttp.setTimeout(2000);

        int dsCode = dsHttp.GET();
        if (dsCode != HTTP_CODE_OK) {
            Serial.printf("DataSource fetch failed (%d). Retaining old values.\n", dsCode);
            dsHttp.end();
            continue;
        }

        String dsPayload = dsHttp.getString();
        dsHttp.end();

        DynamicJsonDocument dsDoc(4096);
        if (deserializeJson(dsDoc, dsPayload) != DeserializationError::Ok) {
            Serial.println("Failed to decode DataSource JSON payload structural logic.");
            continue;
        }

        // --- STEP 4: Dynamic mapping extraction into the active cache slot ---
        JsonObject mapObj   = ds["Map"].as<JsonObject>();
        JsonObject codesObj = ds["Codes"].as<JsonObject>();

        if (mapObj.containsKey("temperature")) dsCache[idx].temperature = dsDoc["current"]["temperature_2m"].as<String>();
        if (mapObj.containsKey("humidity"))    dsCache[idx].humidity    = dsDoc["current"]["relative_humidity_2m"].as<String>();
        if (mapObj.containsKey("pressure"))    dsCache[idx].pressure    = dsDoc["current"]["pressure_msl"].as<String>();
        if (mapObj.containsKey("wind"))        dsCache[idx].wind        = dsDoc["current"]["wind_speed_10m"].as<String>();

        // --- STEP 5: Complex Weather Code Parsing Handling ---
        if (mapObj.containsKey("code")) {
            int code = dsDoc["current"]["weather_code"].as<int>();
            dsCache[idx].code = String(code);

            // Clean previous context state parameters before assignments
            dsCache[idx].codeText  = "Unknown";
            dsCache[idx].codeImage = "";
            dsCache[idx].codeUrl   = "";

            if (!codesObj.isNull()) {
                String key = String(code);
                if (codesObj.containsKey(key)) {
                    JsonVariant entry = codesObj[key];

                    if (entry.is<JsonObject>()) {
                        // Modern Format Object mapping parsing context
                        JsonObject entryObj = entry.as<JsonObject>();
                        if (entryObj.containsKey("label")) dsCache[idx].codeText  = entryObj["label"].as<String>();
                        if (entryObj.containsKey("image")) dsCache[idx].codeImage = entryObj["image"].as<String>();
                        if (entryObj.containsKey("url"))   dsCache[idx].codeUrl   = entryObj["url"].as<String>();
                    } else {
                        // Legacy formatting syntax logic fallback (Maps raw variant direct to code text string)
                        dsCache[idx].codeText = entry.as<String>();
                    }
                }
            }
        }
        Serial.println("DataSource structure updated successfully!");
    }
}

// ============================================================================
// SOLVER: CUSTOM & ROUTED PROPERTY VARIABLES
// ============================================================================
// Parses text lookups referencing internal data caches.
// Syntax structure example formatting: "datasource:OpenMeteo:temperature"
String ResolveCustomValue(String texto) {
    if (texto.startsWith("datasource:")) {
        int firstColon = texto.indexOf(':', 11);
        if (firstColon != -1) {
            String dsId  = texto.substring(11, firstColon);
            String field = texto.substring(firstColon + 1);

            for (int i = 0; i < dsCacheCount; i++) {
                if (dsCache[i].id == dsId) {
                    if (field == "temperature") return dsCache[i].temperature;
                    if (field == "humidity")    return dsCache[i].humidity;
                    if (field == "pressure")    return dsCache[i].pressure;
                    if (field == "wind")        return dsCache[i].wind;
                    if (field == "code")        return dsCache[i].code;
                    if (field == "codeText")    return dsCache[i].codeText;
                    if (field == "code_image")  return dsCache[i].codeImage;
                    if (field == "code_url")    return dsCache[i].codeUrl;
                }
            }
        }
        return "--"; // Return standard fallback UI empty spacer placeholder if string match processing structural path crashes
    }
    return ResolveTextValue(texto); // Forward structural logic mapping fallback processing 
}

#endif