#ifndef ENGINE_H
#define ENGINE_H

#include <Arduino.h>
#include <esp_chip_info.h>

#include "log.h"
#include "datasources.h"

/*
 * engine.h
 * -------------------------------------------------------
 * Detects the number of CPU cores and selects the run mode:
 *
 *  - 1 core  -> everything runs sequentially in loop():
 *                 EngineLoop() = EngineDataUpdate() + EngineDraw()
 *
 *  - 2 cores -> a data task runs on core 0 (EngineDataTask,
 *                 the "data producer") and drawing happens on
 *                 core 1 (loop()).
 *
 * The shared state between the two tasks is protected by a
 * mutex (EngineLock()/EngineUnlock()).
 *
 * EngineDataUpdate() and EngineDraw() are defined in dash001.ino.
 * -------------------------------------------------------
 */

static bool         g_dualCore = false;
static TaskHandle_t g_dataTask = nullptr;
static SemaphoreHandle_t g_stateLock = nullptr;

// ---------------------------------------------------------------
// Shared-state lock (mutex)
// ---------------------------------------------------------------
inline void EngineLock()
{
    if (g_stateLock) xSemaphoreTake(g_stateLock, portMAX_DELAY);
}

inline void EngineUnlock()
{
    if (g_stateLock) xSemaphoreGive(g_stateLock);
}

// ---------------------------------------------------------------
// Hooks implemented in the main sketch
// ---------------------------------------------------------------
void EngineDataUpdate();   // refresh and publish the shared state
void EngineDraw();         // draw the screen from the shared state

// ---------------------------------------------------------------
// Data task (dual-core only): runs on core 0.
// ---------------------------------------------------------------
static void EngineDataTask(void* arg)
{
    (void)arg;
    while (true) {
        EngineLock();
        EngineDataUpdate();
        EngineUnlock();
        vTaskDelay(pdMS_TO_TICKS(1000));   // steady cadence; the real
                                           // refresh is filtered per
                                           // datasource
    }
}

// ---------------------------------------------------------------
// Initialisation: detect cores and start the data task if dual-core.
// ---------------------------------------------------------------
inline void EngineInit()
{
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    g_dualCore = (chip.cores >= 2);

    if (g_stateLock == nullptr) {
        g_stateLock = xSemaphoreCreateMutex();
    }

    if (g_dualCore) {
        logIf("[engine] %u cores: data on core 0 + draw on core 1.",
              (unsigned)chip.cores);
        xTaskCreatePinnedToCore(EngineDataTask, "engineData",
                                8192, nullptr, 1, &g_dataTask, 0);
    } else {
        logI("[engine] 1 core: everything sequential in loop().");
    }
}

// ---------------------------------------------------------------
// Called from loop(). On single-core it alternates data+draw; on
// dual-core it only draws (data comes from the EngineDataTask).
// ---------------------------------------------------------------
inline void EngineLoop()
{
    if (!g_dualCore) {
        EngineDataUpdate();
    }
    EngineLock();
    EngineDraw();
    EngineUnlock();
    delay(100);   // draw cadence (~10 FPS)
}

// ---------------------------------------------------------------
// (future) For when the sketch wants to stop the data task.
// ---------------------------------------------------------------
inline void EngineStop()
{
    if (g_dataTask) {
        vTaskDelete(g_dataTask);
        g_dataTask = nullptr;
    }
}

#endif // ENGINE_H