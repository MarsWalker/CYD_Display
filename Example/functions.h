// UPDATED
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <WiFi.h>
#include <NTPClient.h>
#include <TimeLib.h>

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
 * @brief Evaluates and routes incoming command string requests to fetch dynamic text indicators.
 * Looks explicitly for a "function:" prefix token before parsing the command identifier keyword.
 * * @param texto Raw input command parser tracking string.
 * @return String evaluation output response or an error token array flag if unmapped ("[!name?]").
 */
String ResolveTextValue(String texto)
{
    texto.trim();

    if (!texto.startsWith("function:"))
    {
        return texto;
    }

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

#endif