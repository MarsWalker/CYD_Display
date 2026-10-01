void DataTask()
{
    HandleWebServer();
    UpdateAllWeatherSources();
    UpdateHASensorCache();

    // MQTT
    // HTTP
    // Sensors
    // Weather
    // Home Assistant
    // etc...
}

void DisplayTask()
{
    uint32_t now = millis();
    if ((uint32_t)(now - ScreenLastRefresh) < ScreenRefresh)
        return;  // Ainda não passou o intervalo

    ScreenLastRefresh = now;

    DrawScreen();
  //    TouchRead();
  //    DrawCurrentScreen();
  //    DrawStatusBar();
  //    UpdateAnimations();

}

void DataTaskWrapper(void *pvParameters)
{
    while (true)
    {
        DataTask();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void DisplayTaskWrapper(void *pvParameters)
{
    while (true)
    {
        DisplayTask();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
