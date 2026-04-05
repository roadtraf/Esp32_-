// TaskConfig.h
#ifndef TASK_CONFIG_H
#define TASK_CONFIG_H

#include <Arduino.h>

/* VACUUM_CONTROL_STACK = 4096
SENSOR_READ_STACK = 3072
UI_UPDATE_STACK = 8192
WIFI_MANAGER_STACK = 4096
MQTT_HANDLER_STACK = 4096
DATA_LOGGER_STACK = 4096
HEALTH_MONITOR_STACK = 2048
PREDICTOR_STACK = 4096 으로 조정
*/

// Task Stack Sizes (optimized)
namespace TaskConfig {
    // Critical Tasks
    constexpr uint16_t VACUUM_CONTROL_STACK = 4096;  // 4096 -> 3072
    constexpr uint16_t SENSOR_READ_STACK = 3072;     // 3072 -> 2048
    constexpr uint16_t UI_UPDATE_STACK = 8192;       // 3072 -> 2560
    
    // Normal Priority Tasks
    constexpr uint16_t WIFI_MANAGER_STACK = 4096;    // 유지 (WiFi 스택 요구사항)
    constexpr uint16_t MQTT_HANDLER_STACK = 4096;    // 4096 -> 3584
    constexpr uint16_t DATA_LOGGER_STACK = 4096;     // 2560 -> 2048
    
    // Low Priority Tasks
    constexpr uint16_t HEALTH_MONITOR_STACK = 2048;  // 2560 -> 2048
    constexpr uint16_t PREDICTOR_STACK = 4096;       // 3072 -> 2560
    constexpr uint16_t CLOUD_SYNC_STACK = 3584;      // 3584 -> 3072
    
    // Task Priorities (0 = lowest, 25 = highest)
    constexpr UBaseType_t VACUUM_CONTROL_PRIORITY = 5;
    constexpr UBaseType_t SENSOR_READ_PRIORITY = 4;
    constexpr UBaseType_t UI_UPDATE_PRIORITY = 3;
    constexpr UBaseType_t WIFI_MANAGER_PRIORITY = 2;
    constexpr UBaseType_t MQTT_HANDLER_PRIORITY = 3;
    constexpr UBaseType_t DATA_LOGGER_PRIORITY = 2;
    constexpr UBaseType_t HEALTH_MONITOR_PRIORITY = 1;
    constexpr UBaseType_t PREDICTOR_PRIORITY = 1;
    constexpr UBaseType_t CLOUD_SYNC_PRIORITY = 1;
    
    // Task Update Intervals (ms)
    constexpr uint32_t VACUUM_CONTROL_INTERVAL = 100;
    constexpr uint32_t SENSOR_READ_INTERVAL = 200;
    constexpr uint32_t UI_UPDATE_INTERVAL = 500;
    constexpr uint32_t HEALTH_CHECK_INTERVAL = 5000;
    constexpr uint32_t PREDICTION_INTERVAL = 30000;
    constexpr uint32_t CLOUD_SYNC_INTERVAL = 60000;
}

#endif // TASK_CONFIG_H