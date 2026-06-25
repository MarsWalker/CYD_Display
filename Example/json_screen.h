#ifndef JSON_SCREEN_H
#define JSON_SCREEN_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "datasource.h" // Include our isolated data acquisition layer

// ============================================================================
// EXTERNAL GLOBAL VARIABLES & DRIVERS
// ============================================================================
extern TFT_eSPI tft;           // External instance of the TFT display driver (TFT_eSPI)
extern const char* JSON_URL;   // External base URL pointing to the screen configuration server
extern bool g_HAConnected;     // Global flag tracking the Home Assistant connection status

// ============================================================================
// HELPER COMPONENT FUNCTIONS (Color & Image Mapping)
// ============================================================================
uint16_t GetColorFromName(String nome); // Converts a string color name (e.g., "RED") to RGB565 uint16_t
int FindImageIndex(String nome);        // Looks up the index of an internal bitmap by its name

// ============================================================================
// LOW-LEVEL GRAPHICS DRAWING FUNCTIONS
// ============================================================================
void DrawPixel(int x, int y, uint16_t cor);
void DrawRectangle(int x1, int y1, int x2, int y2, uint16_t corBordo, uint16_t corFundo);
void DrawButton(int x, int y, int w, int h, String texto, uint16_t corFundo, uint16_t corTexto);
void DrawCircle(int x, int y, int raio, uint16_t corBordo, uint16_t corFundo);
void DrawText(int x, int y, String texto, uint16_t cor, int fonte);
void DrawLine(int x1, int y1, int x2, int y2, uint16_t cor);

// ============================================================================
// ADVANCED IMAGE & TEXT RENDERING ENGINES
// ============================================================================
void DrawImage(int x, int y, const unsigned char* bitmap, int w, int h, uint16_t cor, uint8_t zoom);
void DrawJPG(int x, int y, String url);
void DrawJPGFromSD(int x, int y, String path);
void DrawSmartImage(int x, int y, String source, uint16_t cor, uint8_t zoom, bool corDefinida);
String ResolveTextValue(String texto);
bool ResolveBoolValue(String texto);


// ============================================================================
// GLOBAL DATA STORAGE: HOME ASSISTANT ENTITY CACHE
// ============================================================================
struct EntityCache {
    String entity_id;  // The Home Assistant entity code (e.g., "sensor.living_room_temp")
    String lastState;  // Stores the last known state string to detect real changes
};

const int MAX_GLOBAL_ENTITIES = 20;               // Maximum number of HA sensors we can track simultaneously
EntityCache globalCache[MAX_GLOBAL_ENTITIES];    // Allocation of the static entity array cache
int globalCacheCount = 0;                        // Counter for currently active cached entities


// ============================================================================
// LOCAL STORAGE: INTERACTIVE TOUCH SCREEN ZONES
// ============================================================================
struct TouchZone {
    int x1, y1;     // Top-Left bounding box coordinates
    int x2, y2;     // Bottom-Right bounding box coordinates
    String action;  // Dynamic payload execution instruction triggered upon release/tap
};

const int MAX_TOUCH_ZONES = 15;                 // Maximum interactive elements permitted per screen layout
TouchZone currentTouchZones[MAX_TOUCH_ZONES];   // Holds touch fields exclusively for the active page layout
int currentTouchZonesCount = 0;                 // Counter for coordinates actively mapped on screen


// ============================================================================
// API CORE: HOME ASSISTANT STATE GETTER
// ============================================================================
String GetHAState(String entity_id) {
    if (entity_id == "") return "unknown";

    HTTPClient http;
    String url = String(HA_URL) + "/api/states/" + entity_id;

    http.begin(url);
    http.addHeader("Authorization", String("Bearer ") + HA_TOKEN);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(1500);

    int httpCode = http.GET();
    String stateValue = "unknown";

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);

        g_HAConnected = true;
        if (!error && doc.containsKey("state")) {
            stateValue = doc["state"].as<String>();
        }
    } else {
        g_HAConnected = false;
    }
    http.end();
    return stateValue;
}


// ============================================================================
// SYSTEM UTILITY: COUNT SCREENS DECLARED IN MASTER FILE
// ============================================================================
int CountScreens(const char* url) {
    int total = 0;

    HTTPClient http;
    http.begin(url);
    http.setTimeout(2000);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(4096);

        if (deserializeJson(doc, payload) == DeserializationError::Ok) {
            if (doc.containsKey("Screens")) {
                total = doc["Screens"].as<JsonArray>().size();
            }
        }
    } else {
        Serial.printf("Error: Reading number of screens (HTTP: %d)\n", httpCode);
    }
    http.end();

    if (total == 0) total = 1;
    return total;
}


// ============================================================================
// TOUCH DETECTION COMPONENT ENGINE
// ============================================================================
String GetTouchActionForScreen(int tx, int ty) {
    for (int i = 0; i < currentTouchZonesCount; i++) {
        if (tx >= currentTouchZones[i].x1 && tx <= currentTouchZones[i].x2 &&
            ty >= currentTouchZones[i].y1 && ty <= currentTouchZones[i].y2) {
            Serial.printf("[Touch] Zone %d matches signature action payload: %s\n", i, currentTouchZones[i].action.c_str());
            return currentTouchZones[i].action;
        }
    }
    return "";
}


// ============================================================================
// COMPATIBILITY HOOK HELPER
// ============================================================================
static String _stripLegacyPrefix(String source) {
    if (source.startsWith("DrawImage:")) {
        source = source.substring(10);
        source.trim();
    }
    return source;
}


// ============================================================================
// CORE ROUTINE: GRAPHIC OBJECT RENDERING DISPATCHER
// ============================================================================
void DrawObjectFromJson(JsonObject obj, bool firstDraw) {

    if (obj.containsKey("TouchX1") && obj.containsKey("TouchY1") &&
        obj.containsKey("TouchX2") && obj.containsKey("TouchY2") &&
        obj.containsKey("TouchAction")) {

        if (currentTouchZonesCount < MAX_TOUCH_ZONES) {
            currentTouchZones[currentTouchZonesCount].x1     = obj["TouchX1"];
            currentTouchZones[currentTouchZonesCount].y1     = obj["TouchY1"];
            currentTouchZones[currentTouchZonesCount].x2     = obj["TouchX2"];
            currentTouchZones[currentTouchZonesCount].y2     = obj["TouchY2"];
            currentTouchZones[currentTouchZonesCount].action = obj["TouchAction"].as<String>();
            Serial.println(currentTouchZones[currentTouchZonesCount].action);
            currentTouchZonesCount++;
        }
    }

    String type = obj["Type"].as<String>();

    if (type == "FillScreen") {
        if (firstDraw) {
            tft.fillScreen(GetColorFromName(obj["Color"].as<String>()));
        }
    }
    else if (type == "DrawSmartImage") {
        int x        = obj["X"];
        int y        = obj["Y"];
        uint8_t zoom = obj["Zoom"] | 1;

        String source = ResolveCustomValue(obj["Source"].as<String>()); // Defined in datasource.h
        source = _stripLegacyPrefix(source);

        bool corDefinida = obj.containsKey("Color");
        uint16_t cor = corDefinida ? GetColorFromName(obj["Color"].as<String>()) : 0;

        DrawSmartImage(x, y, source, cor, zoom, corDefinida);
    }
    else if (type == "DrawImageBool") {
        int x        = obj["X"];
        int y        = obj["Y"];
        uint8_t zoom = obj["Zoom"] | 1;

        bool result   = ResolveBoolValue(obj["Value"].as<String>());
        String source = _stripLegacyPrefix(result ? obj["true"].as<String>() : obj["false"].as<String>());

        DrawSmartImage(x, y, source, 0, zoom, false);
    }
    else if (type == "DrawImage") {
        int x        = obj["X"];
        int y        = obj["Y"];
        uint8_t zoom = obj["Zoom"] | 1;

        String source = ResolveCustomValue(obj["Image"].as<String>());
        source = _stripLegacyPrefix(source);

        bool corDefinida = obj.containsKey("Color");
        uint16_t cor = corDefinida ? GetColorFromName(obj["Color"].as<String>()) : 0;

        DrawSmartImage(x, y, source, cor, zoom, corDefinida);
    }
    else if (type == "DrawJPG") {
        int x = obj["X"];
        int y = obj["Y"];

        String url = "";
        if      (obj.containsKey("URL"))   url = obj["URL"].as<String>();
        else if (obj.containsKey("Color")) url = obj["Color"].as<String>();

        url = ResolveCustomValue(url);

        if (url.length() > 0)
            DrawSmartImage(x, y, url, 0, 1, false);
    }
    else if (type == "DrawText") {
        int x      = obj["X"];
        int y      = obj["Y"];
        int font   = obj["Font"];
        uint16_t cor = GetColorFromName(obj["Color"].as<String>());

        String text   = ResolveCustomValue(obj["Text"].as<String>());
        String prefix = obj["Preffix"] | "";
        String suffix = obj["Suffix"]  | "";
        text = prefix + text + suffix;

        DrawText(x, y, text, cor, font);
    }
    else if (type == "DrawButton") {
        int x = obj["X"];
        int y = obj["Y"];
        int w = obj["W"];
        int h = obj["H"];

        String text       = ResolveCustomValue(obj["Text"].as<String>());
        uint16_t fillColor = GetColorFromName(obj["FillColor"].as<String>());
        uint16_t textColor = GetColorFromName(obj["TextColor"].as<String>());

        DrawButton(x, y, w, h, text, fillColor, textColor);
    }
    else if (type == "DrawRectangle") {
        int x1 = obj["X1"]; int y1 = obj["Y1"];
        int x2 = obj["X2"]; int y2 = obj["Y2"];
        DrawRectangle(x1, y1, x2, y2,
            GetColorFromName(obj["BorderColor"].as<String>()),
            GetColorFromName(obj["FillColor"].as<String>()));
    }
    else if (type == "DrawLine") {
        int x1 = obj["X1"]; int y1 = obj["Y1"];
        int x2 = obj["X2"]; int y2 = obj["Y2"];
        DrawLine(x1, y1, x2, y2, GetColorFromName(obj["Color"].as<String>()));
    }
    else if (type == "DrawCircle") {
        int x = obj["X"]; int y = obj["Y"]; int radius = obj["Radius"];
        DrawCircle(x, y, radius,
            GetColorFromName(obj["BorderColor"].as<String>()),
            GetColorFromName(obj["FillColor"].as<String>()));
    }
    else if (type == "DrawHASensor") {
        String entity = obj["entity"].as<String>();
        int iconX = obj["IconX"]; int iconY = obj["IconY"];
        int textX = obj["X"];     int textY = obj["Y"];
        int font  = obj["Font"];
        uint16_t corText = GetColorFromName(obj["Color"].as<String>());
        String labelText = obj["Text"].as<String>();

        int cacheIndex = -1;
        for (int i = 0; i < globalCacheCount; i++) {
            if (globalCache[i].entity_id == entity) { cacheIndex = i; break; }
        }
        if (cacheIndex == -1 && globalCacheCount < MAX_GLOBAL_ENTITIES) {
            cacheIndex = globalCacheCount;
            globalCache[cacheIndex].entity_id  = entity;
            globalCache[cacheIndex].lastState  = "";
            globalCacheCount++;
        }

        String currentState  = GetHAState(entity);
        currentState.trim();
        String previousState = (cacheIndex != -1) ? globalCache[cacheIndex].lastState : "";

        if (currentState != previousState || firstDraw) {
            tft.fillRect(iconX, iconY, 24, 24, TFT_BLACK);
            if (cacheIndex != -1) globalCache[cacheIndex].lastState = currentState;

            String stateKey = "state:" + currentState;
            String source   = obj.containsKey(stateKey)   ? obj[stateKey].as<String>()   :
                              obj.containsKey("state:*")   ? obj["state:*"].as<String>()  : "warning";
            source = _stripLegacyPrefix(source);

            int zoom = obj["Zoom"] | 1;
            DrawSmartImage(iconX, iconY, source, 0, zoom, false);
            DrawText(textX, textY, labelText, corText, font);
        }
    }
    else if (type == "DrawHASensorMultiSensor") {
        String entityList = obj["entity"].as<String>();
        int iconX = obj["IconX"]; int iconY = obj["IconY"];
        int textX = obj["X"];     int textY = obj["Y"];
        int font  = obj["Font"];
        uint16_t corText = GetColorFromName(obj["Color"].as<String>());
        String labelText = obj["Text"].as<String>();

        String combinedState = "";
        String activeSource  = "";
        bool foundActive     = false;

        int start = 0; bool done = false;
        while (!done) {
            int commaPos    = entityList.indexOf(',', start);
            String singleEntity = (commaPos == -1) ? entityList.substring(start)
                                                   : entityList.substring(start, commaPos);
            if (commaPos == -1) done = true; else start = commaPos + 1;
            singleEntity.trim();

            if (singleEntity.length() > 0) {
                String st = GetHAState(singleEntity);
                st.trim();
                combinedState += st + "|";

                if (!foundActive) {
                    String stateKey = "state:" + st;
                    if (obj.containsKey(stateKey)) {
                        activeSource = obj[stateKey].as<String>();
                        foundActive  = true;
                    }
                }
            }
        }

        if (!foundActive) {
            activeSource = obj.containsKey("else")     ? obj["else"].as<String>()    :
                           obj.containsKey("state:*")  ? obj["state:*"].as<String>() : "warning";
        }
        activeSource = _stripLegacyPrefix(activeSource);

        int cacheIndex = -1;
        for (int i = 0; i < globalCacheCount; i++) {
            if (globalCache[i].entity_id == entityList) { cacheIndex = i; break; }
        }
        if (cacheIndex == -1 && globalCacheCount < MAX_GLOBAL_ENTITIES) {
            cacheIndex = globalCacheCount;
            globalCache[cacheIndex].entity_id = entityList;
            globalCache[cacheIndex].lastState = "";
            globalCacheCount++;
        }

        String previousState = (cacheIndex != -1) ? globalCache[cacheIndex].lastState : "";

        if (combinedState != previousState || firstDraw) {
            tft.fillRect(iconX, iconY, 24, 24, TFT_BLACK);
            if (cacheIndex != -1) globalCache[cacheIndex].lastState = combinedState;

            int zoom = obj["Zoom"] | 1;
            DrawSmartImage(iconX, iconY, activeSource, 0, zoom, false);
            DrawText(textX, textY, labelText, corText, font);
        }
    }
}


// ============================================================================
// MAIN PIPELINE ENTRYPOINT: LOAD AND DRAW DYNAMIC WINDOW LAYER SCREEN
// ============================================================================
unsigned long LoadAndDrawScreen(const char* url, unsigned long numeroEcra, bool firstDraw) {
    unsigned long refreshRate = 1000;
    int alvoEcra = (int)numeroEcra;
    String subScreenName = "";

    // PHASE 1: Fetch and Decode Master JSON Framework Configurations File
    {
        HTTPClient http;
        http.begin(url);
        http.setTimeout(2000);
        int httpCode = http.GET();

        if (httpCode != HTTP_CODE_OK) {
            Serial.println("[System Error] Master JSON target address resolution failed.");
            http.end();
            return refreshRate;
        }

        String payload = http.getString();
        http.end();

        DynamicJsonDocument doc(4096);
        if (deserializeJson(doc, payload) != DeserializationError::Ok) {
            Serial.println("[System Error] Master configuration parsing execution sequence failed.");
            return refreshRate;
        }

        ParseDataSourcesFromDoc(doc); // Defined in datasource.h

        JsonArray screens = doc["Screens"].as<JsonArray>();
        JsonObject currentScreenObj;
        bool found = false;

        for (JsonObject scr : screens) {
            if (scr["Number"].as<int>() == alvoEcra) {
                currentScreenObj = scr;
                found = true;
                tft.invertDisplay(scr["Invert"] | true);
                tft.setRotation(scr["Rotation"] | 1);
                break;
            }
        }

        if (!found && screens.size() > 0) {
            currentScreenObj = screens[0];
            alvoEcra = currentScreenObj["Number"].as<int>();
        }

        if (currentScreenObj.containsKey("name")) {
            subScreenName = currentScreenObj["name"].as<String>();
        }
    }

    if (subScreenName == "") {
        Serial.println("[System Error] Child screen route target identifier asset label configuration missing.");
        return refreshRate;
    }

    // PHASE 2: Fetch and Render Target Sub-Screen Dynamic Component Elements
    String subScreenUrl = String(JSON_URL) + subScreenName;

    HTTPClient http;
    http.begin(subScreenUrl);
    http.setTimeout(2000);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[System Error] Remote rendering components file request rejected (HTTP: %d)\n", httpCode);
        http.end();
        return refreshRate;
    }

    String subPayload = http.getString();
    http.end();

    DynamicJsonDocument subDoc(16384);
    if (deserializeJson(subDoc, subPayload) != DeserializationError::Ok) {
        Serial.println("[System Error] Failed to reconstruct dynamic object layout document.");
        return refreshRate;
    }

    if (subDoc.containsKey("Refresh")) {
        refreshRate = subDoc["Refresh"].as<unsigned long>();
    }

    currentTouchZonesCount = 0;

    if (firstDraw) {
        tft.fillScreen(TFT_BLACK);
    }

    for (JsonObject obj : subDoc["Objects"].as<JsonArray>()) {
        DrawObjectFromJson(obj, firstDraw);
    }

    return refreshRate;
}
#endif