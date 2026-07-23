#include <ArduinoJson.h>


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

inline bool LoadScreenFromJson(const String& json)
{
    DeserializationError error = deserializeJson(ScreenDoc, json);
    ScreenLastRefresh = millis();
    ScreenRefresh = ScreenDoc["Refresh"] | 1000UL;

    if (error)
    {
        writeLogln("JSON parse error: " + String(error.c_str()));
        return false;
    }

    return true;
}

bool LoadScreen(const String& url)
{
    int code;

    writeLogln(url);

    String json = httpRequest(url, &code);

    if (code != HTTP_CODE_OK)
    {
        writeLogln("HTTP error: " + String(code));
        return false;
    }
    else
    {
      writeLogln("HTTP JSON loaded");
      FirstDraw = true;
    }
    return LoadScreenFromJson(json);
}

inline void DrawObjectFromJson(JsonObject obj)
{
    String type    = obj["Type"] | "";
    String text    = ResolveTextValue(obj["Text"]);
    int x          = obj["X"] | 0;
    int y          = obj["Y"] | 0;
    int zoom       = obj["Zoom"] | 1;
    int font       = obj["Font"] | 1;
    uint16_t color = GetColorFromName(obj["Color"]);
    bool HasColor  = obj.containsKey("Color");
    String Preffix = obj["Preffix"] | "";
    String Suffix  = obj["Suffix"] | "";

    if (type == "FillScreen")
    {
      if (FirstDraw)
      {
        FirstDraw = false;
        FillScreen(screen_backgound_color);
      }
    }
    else if (type == "DrawText")
    {
        text = Preffix + text + Suffix;
        DrawText( x, y, text, color, font );
    }
    else if (type == "DrawHASensor")
    {
        String entity = obj["entity"] | "";
        int iconX = obj["IconX"] | 0;
        int iconY = obj["IconY"] | 0;

        int cacheIndex = GetOrCreateHACache(entity);
        if (cacheIndex == -1)
            return;

        String currentState = globalCache[cacheIndex].state;

        if (currentState != globalCache[cacheIndex].lastDrawnState || FirstDraw)
        {
            tft.fillRect(iconX, iconY, 24, 24, TFT_BLACK);

            String stateKey = "state:" + currentState;
            String source = obj.containsKey(stateKey) ? obj[stateKey].as<String>() : obj.containsKey("state:*") ? obj["state:*"].as<String>() : "warning";
            source = _stripLegacyPrefix(source);

            int iconZoom = obj["Zoom"] | 1;
            DrawSmartImage(iconX, iconY, source, 0, iconZoom, false);
            DrawText(x, y, text, color, font);

            globalCache[cacheIndex].lastDrawnState = currentState;
        }
    }
    else if (type == "DrawImage")
    {
        String imageSource = ResolveImageValue(obj["Image"] | "");
        DrawSmartImage( x, y, imageSource, color, zoom , HasColor );
    }
    else if (type == "DrawImageBool")
    {
      bool value = ResolveBoolValue(obj["Value"] | "");
      String imageSource = ResolveImageValue(value ? (obj["true"] | "") : (obj["false"] | ""));
      DrawSmartImage(x, y, imageSource, color, zoom, HasColor);
    }
    else 
    {
        Serial.println(type);
    }
}

inline void DrawScreen()
{
    if (CurrentScreen != RequestedScreen)
    {
        ScreenInfo* screenInfo = FindScreenByNumber(RequestedScreen);

        if (screenInfo != nullptr)
        {
            CurrentScreen = RequestedScreen;
            writeLogln("A carregar ecra: " + screenInfo->Name);

            if (LoadScreen(String(JSON_URL) + screenInfo->Name))
            {
                tft.invertDisplay(screenInfo->Invert);
                tft.setRotation(screenInfo->Rotation);
                DrawScreen();
            }
            return; // evita desenhar se o LoadScreen falhar
        }
        else
        {
            writeLogln("Screen number nao encontrado: " + String(RequestedScreen));
            return;
        }
    }

    if (!ScreenDoc.containsKey("Objects"))
    {
        writeLogln("No objects");
        return;
    }

    JsonArray objects = ScreenDoc["Objects"].as<JsonArray>();

    for (JsonObject obj : objects)
    {
        DrawObjectFromJson(obj);
    }
}
