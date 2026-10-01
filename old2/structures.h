#pragma once

/*
 * structures.h
 * ------------------------------------------------------------
 * Global configuration structures loaded from configuration.json.
 *
 * Currently implemented:
 *   - Weather data sources
 *   - Screens
 *
 * Designed to be easily extended with:
 *   - Home Assistant
 *   - MQTT
 *   - REST
 *   - Modbus
 *   - Local sensors
 * ------------------------------------------------------------
 */

#include <Arduino.h>

//==============================================================
// Limits
//==============================================================

constexpr uint8_t MAX_DATASOURCES      = 10;
constexpr uint8_t MAX_WEATHER_CODES    = 32;
constexpr uint8_t MAX_SCREENS          = 10;

//==============================================================
// Data Source Types
//==============================================================

enum class DataSourceType : uint8_t
{
    Unknown = 0,
    Weather,
    HomeAssistant,
    MQTT,
    REST,
    Sensor
};

//==============================================================
// Generic Data Source
//==============================================================

struct DataSource
{
    DataSourceType type = DataSourceType::Unknown;

    String id;

    uint32_t refresh = 0;

    String url;
};

//==============================================================
// Weather Code Description
//==============================================================

struct WeatherCode
{
    int code = 0;
    String label;
    String image;
    String url;
};

//==============================================================
// Weather JSON Mapping
//==============================================================

struct WeatherMap
{
    String temperature;
    String humidity;
    String pressure;
    String wind;
    String code;
};

//==============================================================
// Weather Data Source
//==============================================================

struct WeatherDataSource : public DataSource
{
    WeatherMap map;
    WeatherCode codes[MAX_WEATHER_CODES];
    uint8_t codeCount = 0;
    // Runtime values (updated after each refresh)
    float temperature = NAN;
    float humidity = NAN;
    float pressure = NAN;
    float wind = NAN;
    String codeText = "C";
    int weatherCode = -1;
};

//==============================================================
// Screens
//==============================================================

struct ScreenInfo
{
    int     Number   = 0;
    String  Name;
    bool    Invert   = false;
    uint8_t Rotation = 1;
};

//==============================================================
// Global Configuration
//==============================================================

struct Configuration
{
    //----------------------------------------------------------
    // Weather
    //----------------------------------------------------------

    WeatherDataSource weather[MAX_DATASOURCES];

    uint8_t weatherCount = 0;

    //----------------------------------------------------------
    // Screens
    //----------------------------------------------------------

    ScreenInfo screens[MAX_SCREENS];

    uint8_t screenCount = 0;

    //----------------------------------------------------------
    // Future datasource types
    //----------------------------------------------------------

    // HomeAssistantSource homeAssistant[MAX_DATASOURCES];
    // MQTTSource mqtt[MAX_DATASOURCES];
    // RestSource rest[MAX_DATASOURCES];
};

//==============================================================
// Global instance
//==============================================================

extern Configuration Config;

struct HAEntityCache
{
    String entity_id;
    String state;           // último valor recebido do Home Assistant
    String lastDrawnState;  // último valor já desenhado
};

HAEntityCache globalCache[MAX_GLOBAL_ENTITIES];
