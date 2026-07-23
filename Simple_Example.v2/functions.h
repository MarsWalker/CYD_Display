// UPDATED
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <WiFi.h>
#include <NTPClient.h>
#include <TimeLib.h>

//----------------------------------------------------------
// HTTP(s) functions
//----------------------------------------------------------
// ---------------------------------------------------------------
// httpRequest: accepts a URL and automatically selects
// WiFiClientSecure (HTTPS) or WiFiClient (HTTP).
//
// Returns the response body as a String.
// Returns an empty String on error.
//
// Optionally returns the HTTP response code via httpCode.
// ---------------------------------------------------------------
inline String httpRequest(const String &url, int *httpCode = nullptr) {
    HTTPClient http;
    String payload = "";
    int code = -1;

    if (WiFi.status() != WL_CONNECTED) {
        writeLogln("WiFi is not connected!");
        if (httpCode) *httpCode = code;
        return payload;
    }

    bool isHttps = url.startsWith("https://");

    if (isHttps) {
        WiFiClientSecure client;
        client.setInsecure();   // Certificate validation disabled.
                                // Replace with setCACert() if certificate verification is required.

        if (http.begin(client, url)) {
            code = http.GET();

            if (code > 0) {
                payload = http.getString();
            } else {
                writeLogln("HTTPS error: " + http.errorToString(code));
            }

            http.end();
        } else {
            writeLogln("Failed to initialize HTTPS connection");
        }

    } else {
        WiFiClient client;

        if (http.begin(client, url)) {
            code = http.GET();

            if (code > 0) {
                payload = http.getString();
            } else {
                writeLogln("HTTP error: " + http.errorToString(code));
            }

            http.end();
        } else {
            writeLogln("Failed to initialize HTTP connection");
        }
    }

    if (httpCode) *httpCode = code;
    return payload;
}

//----------------------------------------------------------
// Misc functions
//----------------------------------------------------------
bool UpdateWeatherDataSource(WeatherDataSource &ws)
{
    int code;
    String json = httpRequest(ws.url, &code);

    if (code != HTTP_CODE_OK)
    {
        writeLogln("Weather HTTP error: " + String(code));
        return false;
    }

    DynamicJsonDocument weatherDoc(2048); // ajusta conforme resposta da API
    DeserializationError err = deserializeJson(weatherDoc, json);
    if (err)
    {
        writeLogln("Weather JSON parse error: " + String(err.c_str()));
        return false;
    }

    JsonVariant root = weatherDoc.as<JsonVariant>();

    ws.temperature = ResolveJsonPath(root, ws.map.temperature).as<float>();
    ws.humidity    = ResolveJsonPath(root, ws.map.humidity).as<float>();
    ws.pressure    = ResolveJsonPath(root, ws.map.pressure).as<float>();
    ws.wind        = ResolveJsonPath(root, ws.map.wind).as<float>();
    ws.weatherCode = ResolveJsonPath(root, ws.map.code).as<int>();

    // 1. Obtém o código numérico vindo da API
    ws.weatherCode = ResolveJsonPath(root, ws.map.code).as<int>();

    // 2. Procura o código no array de configurações para preencher o codeText
    WeatherCode* wc = FindWeatherCode(ws, ws.weatherCode);
    if (wc != nullptr)
    {
        ws.codeText = wc->label; // Preenche com o texto (ex: "Clear sky", "Partly cloudy")
    }
    else
    {
        ws.codeText = "Unknown Code"; // Caso venha um código da API que não está no JSON de config
    }

    return true;
}

// Chama isto periodicamente na DataTask, respeitando ws.refresh (ms)
void UpdateAllWeatherSources()
{
    static unsigned long lastUpdate[MAX_DATASOURCES] = {0};

    for (uint8_t i = 0; i < Config.weatherCount; i++)
    {
        WeatherDataSource &ws = Config.weather[i];
        unsigned long now = millis();

        if (now - lastUpdate[i] >= ws.refresh || lastUpdate[i] == 0)
        {
            if (UpdateWeatherDataSource(ws))
                lastUpdate[i] = now;
        }
    }
}

// ===========================================================
// External Dependencies and Global Instances
// ===========================================================
extern NTPClient timeClient;
// Note: Adjusted type references below. Change if your main configuration uses different types.
extern PubSubClient mqttClient; 
extern bool g_HAConnected;

// ===========================================================
// Core Utility Functions
// ===========================================================

/**
 * @brief Fetches the current time from the NTP server and formats it.
 * @return String formatted as "HH:MM:SS" (24-hour clock).
 */
String Clock()
{
    timeClient.update();

    int hora    = timeClient.getHours();
    int minuto  = timeClient.getMinutes();
    int segundo = timeClient.getSeconds();

    char buffer[9];
    sprintf(buffer, "%02d:%02d:%02d", hora, minuto, segundo);

    return String(buffer);
}

/**
 * @brief Calculates the current calendar date based on NTP epoch timestamp.
 * @return String formatted as "YYYY/MM/DD".
 */
String Date()
{
    timeClient.update();

    unsigned long epoch = timeClient.getEpochTime();

    int ano = year(epoch);
    int mes = month(epoch);
    int dia = day(epoch);

    char buffer[11];
    sprintf(buffer, "%04d/%02d/%02d", ano, mes, dia);

    return String(buffer);
}

/**
 * @brief Computes total system operational uptime since the microcontroller booted.
 * @return String formatted as "Uptime: Xd Xh Xm Xs".
 */
String Uptime()
{
    unsigned long seg = millis() / 1000;

    int dias  = seg / 86400;  seg %= 86400;
    int horas = seg / 3600;   seg %= 3600;
    int mins  = seg / 60;     seg %= 60;

    char buffer[32];
    sprintf(buffer, "Uptime: %dd %dh %dm %ds", dias, horas, mins, (int)seg);

    return String(buffer);
}

/**
 * @brief Evaluates current connection state of the local Wi-Fi radio interface.
 * @return true if connected to an access point, false otherwise.
 */
bool IsWIFIConnected()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief Reads the Received Signal Strength Indicator (RSSI) of the Wi-Fi network.
 * @return String representation of signal power level (e.g., "-65 dBm").
 */
String WifiRSSI()
{
    int rssi = WiFi.RSSI();

    char buffer[16];
    sprintf(buffer, "%d dBm", rssi);

    return String(buffer);
}

/**
 * @brief Monitors the dynamic operational overhead memory footprint available to the ESP system.
 * @return String showing remaining accessible system Heap RAM in Kilobytes (e.g., "234 KB").
 */
String FreeRAM()
{
    uint32_t freeHeap = ESP.getFreeHeap();

    char buffer[16];
    sprintf(buffer, "%d KB", freeHeap / 1024);

    return String(buffer);
}

/**
 * @brief Obtains the local IPv4 address assigned to the microcontroller network interface card.
 * @return String containing standard dotted-quad format IP (e.g., "192.168.1.100").
 */
String MyIP()
{
    return WiFi.localIP().toString();
}

/**
 * @brief Evaluates current connection status to the central MQTT broker instance.
 * @return true if communication link is active, false otherwise.
 */
bool IsMQTTConnected()
{
    if (mqttClient.connected())
        return true;

    return false;
}

/**
 * @brief Evaluates communication handshake state with the automated Home Assistant layer.
 * @return true if Home Assistant link integration flag is validated, false otherwise.
 */
bool IsHAConnected()
{
    return g_HAConnected;
}

// ===========================================================
// Command Parsing and Reflection Resolvers
// ===========================================================

/**
 * @brief Resolves dynamic "datasource:" image references into their final embedded image key.
 * @param texto Raw input Image field from JSON ("datasource:<id>:code_image", or a literal name).
 * @return Resolved image key string ready to be passed into DrawSmartImage.
 */
String ResolveImageValue(String texto)
{
    texto.trim();

    if (!texto.startsWith("datasource:") && !texto.startsWith("DrawImage:") && !texto.startsWith("DrawText:"))
    {
        return texto; // não é placeholder, é um nome de imagem literal (ex: "sun")
    }

    if (texto.startsWith("DrawImage:"))
    {
        int firstColon  = texto.indexOf(':');
        String image    = texto.substring(firstColon + 1);
        return image;
    }

    // formato esperado: datasource:<id>:<field>
    int firstColon  = texto.indexOf(':');
    int secondColon = texto.indexOf(':', firstColon + 1);


    if (secondColon == -1) return "";

    String id    = texto.substring(firstColon + 1, secondColon);
    String field = texto.substring(secondColon + 1);
    field.trim();

    WeatherDataSource* ws = FindWeatherSource(id);
    if (!ws) return "";

    if (field.equalsIgnoreCase("codetext"))
    {
        return ws ? ws->codeText : "Not found";
    }

    if (field.equalsIgnoreCase("code_image"))
    {
        WeatherCode* wc = FindWeatherCode(*ws, ws->weatherCode);
        return wc ? wc->image : "";
    }

    return "";
}

/**
 * @brief Evaluates and routes incoming command string requests to fetch dynamic text indicators.
 * Looks explicitly for a "function:" or "datasource:" prefix token before parsing the command identifier keyword.
 * * @param texto Raw input command parser tracking string.
 * @return String evaluation output response or an error token array flag if unmapped ("[!name?]").
 */
String ResolveTextValue(String texto)
{
    texto.trim();

    // ---------- Routing Option 1: Built-in system functions ----------
    if (texto.startsWith("function:"))
    {
        String funcName = texto.substring(9);
        funcName.trim();

        if (funcName.equalsIgnoreCase("Clock"))    return Clock();
        if (funcName.equalsIgnoreCase("Date"))     return Date();
        if (funcName.equalsIgnoreCase("Uptime"))   return Uptime();
        if (funcName.equalsIgnoreCase("WifiRSSI")) return WifiRSSI();
        if (funcName.equalsIgnoreCase("FreeRAM"))  return FreeRAM();
        if (funcName.equalsIgnoreCase("MyIP"))     return MyIP();

        return "[" + funcName + "?]";
    }

    // ---------- Routing Option 2: Live DataSource values (weather, etc.) ----------
    if (texto.startsWith("datasource:"))
    {
        // formato esperado: datasource:<id>:<field>
        int firstColon  = texto.indexOf(':');
        int secondColon = texto.indexOf(':', firstColon + 1);

        if (secondColon == -1) return "[!datasource?]";

        String id    = texto.substring(firstColon + 1, secondColon);
        String field = texto.substring(secondColon + 1);
        field.trim();

        WeatherDataSource* ws = FindWeatherSource(id);
        if (!ws) return "[!ID " + id + "?]";

        if (field.equalsIgnoreCase("temperature")) return String(ws->temperature, 1); // + "\u00B0C";
        if (field.equalsIgnoreCase("humidity"))    return String(ws->humidity, 0); // + "%";
        if (field.equalsIgnoreCase("pressure"))    return String(ws->pressure, 0); // + " hPa";
        if (field.equalsIgnoreCase("wind"))        return String(ws->wind, 1); // + " km/h";

        if (field.equalsIgnoreCase("codetext"))
        {
            WeatherCode* wc = FindWeatherCode(*ws, ws->weatherCode);
            return wc ? wc->label : "?";
        }

        return "[!Not found " + field + "?]";
    }

    // ---------- Nenhum prefixo reconhecido: devolve texto literal ----------
    return texto;
}
/**
 * @brief Evaluates and routes incoming command string requests to fetch runtime boolean statuses.
 * Looks explicitly for a "function:" prefix token before evaluating structural logical operations.
 * * @param texto Raw input command parser tracking string.
 * @return Boolean condition response logic mapping status, defaults to false if invalid.
 */
bool ResolveBoolValue(String texto)
{
    texto.trim();

    if (!texto.startsWith("function:"))
    {
        return false;
    }

    String funcName = texto.substring(9);
    funcName.trim();

    if (funcName.equalsIgnoreCase("IsWIFIConnected")) return IsWIFIConnected();
    if (funcName.equalsIgnoreCase("IsMQTTConnected")) return IsMQTTConnected();
    if (funcName.equalsIgnoreCase("IsHAConnected"))   return IsHAConnected();

    return false;
}

int GetOrCreateHACache(const String& entity)
{
    for (int i = 0; i < globalCacheCount; i++)
    {
        if (globalCache[i].entity_id == entity)
            return i;
    }

    if (globalCacheCount >= MAX_GLOBAL_ENTITIES)
        return -1;

    int index = globalCacheCount++;
    globalCache[index].entity_id = entity;
    globalCache[index].state = "unknown";
    globalCache[index].lastDrawnState = "";
    return index;
}

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

void UpdateHASensorCache()
{
    static uint32_t lastUpdate = 0;
    const uint32_t refreshHA = 10000; // consulta HA a cada 10 segundos

    uint32_t now = millis();
    if ((uint32_t)(now - lastUpdate) < refreshHA)
        return;

    lastUpdate = now;

    for (int i = 0; i < globalCacheCount; i++)
    {
        String state = GetHAState(globalCache[i].entity_id);
        state.trim();

        globalCache[i].state = state;
    }
}
#endif