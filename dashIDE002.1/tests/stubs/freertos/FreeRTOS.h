/**
 * @file freertos/FreeRTOS.h
 *
 * Single-threaded scheduler substitute. The sentinel exception terminates a
 * worker round; task creation marks a handle without spawning a thread.
 */

#pragma once
#include <cstdint>
#define portMAX_DELAY UINT32_MAX
#define pdMS_TO_TICKS(x) (x)
#define pdPASS 1
using TaskHandle_t=void*;
inline void vTaskDelay(unsigned){throw 42;}
inline int xTaskCreate(void(*)(void*),const char*,unsigned,void*,int,TaskHandle_t* task){*task=(void*)1;return pdPASS;}
