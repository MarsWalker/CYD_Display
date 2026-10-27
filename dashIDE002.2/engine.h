/**
 * @file engine.h
 *
 * Scheduling boundary between configuration/data ownership and screen drawing.
 * On dual-core chips one FreeRTOS task polls data while loop() handles UI/web.
 * A single shared mutex protects ordinary sources, objects and power history.
 * Ping owns a separate mutex and performs network requests without holding this
 * engine mutex; it must not call engine-locking operations from its callbacks.
 * On single-core chips the same hooks run sequentially in loop().
 */

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
/**
 * Enter the shared configuration/cache/render critical section.
 * Before EngineInit creates the mutex this is a no-op, which permits the same
 * screen/configuration functions to be used safely during single-threaded boot.
 * The mutex is not recursive: hooks called under it must not reacquire it.
 */
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
/**
 * Dedicated data producer on core 0 for dual-core ESP32 profiles.
 * The one-second task cadence is additional to each source's refresh gate.
 * Blocking ordinary HTTP requests keep the engine lock; independent Ping work
 * does not use this lock and can continue on its own task.
 */
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
/**
 * Create shared state protection and start the optional data task once.
 * This is a boot-time initialization operation, not a per-screen reset.
 */
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
/**
 * Delete the ordinary data task if running.
 * This helper does not stop the independent Ping worker or destroy its state;
 * it is not a complete runtime reconfiguration/shutdown API.
 */
inline void EngineStop()
{
    if (g_dataTask) {
        vTaskDelete(g_dataTask);
        g_dataTask = nullptr;
    }
}

#endif // ENGINE_H