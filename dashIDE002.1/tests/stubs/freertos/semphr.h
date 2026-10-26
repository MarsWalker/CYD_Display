/**
 * @file freertos/semphr.h
 *
 * Non-blocking semaphore placeholders for single-threaded worker tests.
 * These stubs do not model real race conditions or FreeRTOS scheduling.
 */

#pragma once
#include "FreeRTOS.h"
using SemaphoreHandle_t=void*;
inline SemaphoreHandle_t xSemaphoreCreateBinary(){return (void*)1;}
inline SemaphoreHandle_t xSemaphoreCreateMutex(){return (void*)1;}
inline void xSemaphoreGive(SemaphoreHandle_t){}
inline void xSemaphoreTake(SemaphoreHandle_t,unsigned){}
inline void vSemaphoreDelete(SemaphoreHandle_t){}
