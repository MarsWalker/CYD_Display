#pragma once
#include <ArduinoJson.h>

Configuration Config;   // instância global (declarada extern no Configuration.h)

// Resolve caminhos tipo "current.temperature_2m" dentro de um JsonVariant
JsonVariant ResolveJsonPath(JsonVariant root, const String& path)
{
    JsonVariant current = root;
    int start = 0;

    while (start < (int)path.length())
    {
        int dot = path.indexOf('.', start);
        String key = (dot == -1) ? path.substring(start) : path.substring(start, dot);

        if (!current.is<JsonObject>()) return JsonVariant();
        current = current[key];

        if (dot == -1) break;
        start = dot + 1;
    }
    return current;
}

bool LoadConfigurationFromJson(JsonDocument& doc)
{
    Config.weatherCount = 0;
    Config.screenCount = 0;

    // DataSources
    if (doc.containsKey("DataSources"))
    {
        JsonArray sources = doc["DataSources"].as<JsonArray>();

        for (JsonObject src : sources)
        {
            String type = src["Type"] | "";

            if (type == "Weather" && Config.weatherCount < MAX_DATASOURCES)
            {
                WeatherDataSource& ws = Config.weather[Config.weatherCount];

                ws.type    = DataSourceType::Weather;
                ws.id      = src["Id"].as<String>();
                ws.refresh = src["Refresh"] | 0;
                ws.url     = src["url"].as<String>();

                JsonObject map = src["Map"];
                ws.map.temperature = map["temperature"] | "";
                ws.map.humidity    = map["humidity"]    | "";
                ws.map.pressure    = map["pressure"]    | "";
                ws.map.wind        = map["wind"]        | "";
                ws.map.code        = map["code"]        | "";

                ws.codeCount = 0;
                JsonObject codes = src["Codes"];

                for (JsonPair kv : codes)
                {
                    if (ws.codeCount >= MAX_WEATHER_CODES) break;

                    WeatherCode& wc = ws.codes[ws.codeCount];
                    wc.code = String(kv.key().c_str()).toInt();

                    JsonObject c = kv.value().as<JsonObject>();
                    wc.label = c["label"] | "";
                    wc.image = c["image"] | "";
                    wc.url   = c["url"]   | "";

                    ws.codeCount++;
                }

                Config.weatherCount++;
            }
        }
    }

    // Screens — independente de DataSources
    if (doc.containsKey("Screens"))
    {
        JsonArray screens = doc["Screens"].as<JsonArray>();

        for (JsonObject screen : screens)
        {
            if (Config.screenCount >= MAX_SCREENS)
            {
                writeLogln("MAX_SCREENS atingido, a ignorar ecras extra");
                break;
            }

            ScreenInfo& s = Config.screens[Config.screenCount];
            s.Number   = screen["Number"] | 0;
            s.Name     = String((const char*)(screen["Name"] | ""));
            s.Invert   = screen["Invert"] | false;
            s.Rotation = screen["Rotation"] | 1;

            Config.screenCount++;
        }
    }

    return true;
}

WeatherDataSource* FindWeatherSource(const String& id)
{
    for (uint8_t i = 0; i < Config.weatherCount; i++)
        if (Config.weather[i].id == id)
            return &Config.weather[i];
    return nullptr;
}

WeatherCode* FindWeatherCode(WeatherDataSource& ws, int code)
{
    for (uint8_t i = 0; i < ws.codeCount; i++)
        if (ws.codes[i].code == code)
            return &ws.codes[i];
    return nullptr;
}

ScreenInfo* FindScreenByNumber(int number)
{
    for (int i = 0; i < Config.screenCount; i++)
    {
        if (Config.screens[i].Number == number)
        {
            return &Config.screens[i];
        }
    }
    return nullptr;
}