// ================================================================
// Tasks.h - FreeRTOS  
// ESP32-S3 v3.9.2 Phase 3-1
// ================================================================
#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ================================================================
//   
// ================================================================
extern TaskHandle_t vacuumTaskHandle;
extern TaskHandle_t sensorTaskHandle;
extern TaskHandle_t uiTaskHandle;
extern TaskHandle_t wifiTaskHandle;
extern TaskHandle_t mqttTaskHandle;
extern TaskHandle_t loggerTaskHandle;
extern TaskHandle_t healthTaskHandle;
extern TaskHandle_t predictorTaskHandle;

// ================================================================
//   
// ================================================================
void vacuumControlTask(void* param);
void sensorReadTask(void* param);
void uiUpdateTask(void* param);
void wifiManagerTask(void* param);
void mqttHandlerTask(void* param);
void dataLoggerTask(void* param);
void healthMonitorTask(void* param);
void predictorTask(void* param);

// ================================================================
//    
// ================================================================
void createAllTasks();
