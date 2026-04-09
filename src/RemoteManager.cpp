// ================================================================
// RemoteManager.cpp - MQTT   
// ================================================================
#include "RemoteManager.h"
#include "Config.h"
#include "Sensor.h"
#include "SensorBuffer.h"
#include <ArduinoJson.h>

RemoteManager remoteManager;

// ================================================================
// 
// ================================================================
RemoteManager::RemoteManager()
    : mqttClient(nullptr),
      remoteSessionActive(false),
      remoteSessionStart(0),
      remoteSessionTimeout(600000),  // 10
      remoteLoginAttempts(0),
      remoteLockoutEnd(0)
{
    memset(remoteClientId, 0, sizeof(remoteClientId));
}

// ================================================================
// 
// ================================================================
void RemoteManager::begin(PubSubClient* client) {
    mqttClient = client;
    
    Serial.println("[RemoteManager]  ");
    Serial.println("[RemoteManager]   ");
}

// ================================================================
// MQTT  
// ================================================================
void RemoteManager::handleMQTTMessage(const char* topic, const char* payload) {
    if (strcmp(topic, MQTT_TOPIC_REMOTE_CMD) != 0) {
        return;  //    
    }
    
    Serial.printf("[RemoteManager]  : %s\n", payload);
    
    // JSON 
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.println("[RemoteManager] JSON  ");
        sendResponse("JSON  ", false);
        return;
    }
    
    //  
    const char* action = doc["action"] | "";
    const char* password = doc["password"] | "";
    const char* parameter = doc["parameter"] | "";
    const char* clientId = doc["client_id"] | "";
    
    if (strlen(action) == 0) {
        sendResponse(" ", false);
        return;
    }
    
    //   ( )
    if (strcmp(action, "login") == 0) {
        handleRemoteLogin(password, clientId);
        return;
    }
    
    //  
    if (strcmp(action, "logout") == 0) {
        handleRemoteLogout();
        return;
    }
    
    //   (/   )
    if (!remoteSessionActive) {
        sendResponse("  .  .", false);
        return;
    }
    
    //  
    processRemoteCommand(action, parameter);
}

// ================================================================
//   
// ================================================================
void RemoteManager::handleRemoteLogin(const char* password, const char* clientId) {
    //  
    if (isRemoteLocked()) {
        uint32_t remaining = (remoteLockoutEnd - millis()) / 1000;
        char msg[64];
        snprintf(msg, sizeof(msg), " : %lu ", remaining);
        sendResponse(msg, false);
        return;
    }
    
    //  
    if (!verifyRemotePassword(password)) {
        remoteLoginAttempts++;
        
        if (remoteLoginAttempts >= 3) {
            remoteLockoutEnd = millis() + 60000;  // 1 
            sendResponse("  3.   (60)", false);
        } else {
            char msg[64];
            snprintf(msg, sizeof(msg), "  (%d/3)", remoteLoginAttempts);
            sendResponse(msg, false);
        }
        return;
    }
    
    //  
    resetRemoteAttempts();
    remoteSessionActive = true;
    remoteSessionStart = millis();
    strncpy(remoteClientId, clientId, sizeof(remoteClientId) - 1);
    
    //     
    systemController.enterManagerMode(password);
    
    Serial.printf("[RemoteManager]   : %s\n", clientId);
    sendResponse("    ");
    
    //  
    publishStatus();
}

// ================================================================
//   
// ================================================================
void RemoteManager::handleRemoteLogout() {
    if (!remoteSessionActive) {
        sendResponse("  ", false);
        return;
    }
    
    terminateRemoteSession();
    sendResponse("  ");
}

// ================================================================
//   
// ================================================================
void RemoteManager::terminateRemoteSession() {
    remoteSessionActive = false;
    memset(remoteClientId, 0, sizeof(remoteClientId));
    
    //    
    systemController.enterOperatorMode();
    
    Serial.println("[RemoteManager]   ");
    publishStatus();
}

// ================================================================
//  
// ================================================================
void RemoteManager::processRemoteCommand(const char* action, const char* parameter) {
    if (strcmp(action, "status") == 0) {
        handleRemoteStatus();
    }
    else if (strcmp(action, "sensor_data") == 0) {
        handleRemoteSensorData();
    }
    else if (strcmp(action, "calibrate") == 0) {
        handleRemoteCalibrate(parameter);
    }
    else if (strcmp(action, "setting") == 0) {
        // parameter : "key=value"
        char key[32], value[32];
        if (sscanf(parameter, "%[^=]=%s", key, value) == 2) {
            handleRemoteSettings(key, value);
        } else {
            sendResponse("   (key=value )", false);
        }
    }
    else {
        char msg[64];
        snprintf(msg, sizeof(msg), "   : %s", action);
        sendResponse(msg, false);
    }
}

// ================================================================
//  
// ================================================================
void RemoteManager::handleRemoteStatus() {
    StaticJsonDocument<512> doc;
    
    doc["mode"] = systemController.getModeString();
    doc["uptime"] = millis() / 1000;
    doc["free_heap"] = ESP.getFreeHeap();
    doc["wifi_rssi"] = WiFi.RSSI();
    doc["mqtt_connected"] = mqttClient->connected();
    
    //  
    doc["temp_sensor"] = isTemperatureSensorConnected();
    doc["sensor_count"] = getTemperatureSensorCount();
    
    char buffer[512];
    serializeJson(doc, buffer);
    
    mqttClient->publish(MQTT_TOPIC_REMOTE_RESPONSE, buffer);
}

// ================================================================
//   
// ================================================================
void RemoteManager::handleRemoteSensorData() {
    StaticJsonDocument<512> doc;
    
    //  
    doc["temperature"] = readTemperature();
    doc["pressure"] = readPressure();
    doc["current"] = readCurrent();
    
    // 
    SensorStats stats;
    calculateSensorStats(stats);
    
    JsonObject statsObj = doc.createNestedObject("stats");
    statsObj["avg_temp"] = stats.avgTemperature;
    statsObj["avg_pressure"] = stats.avgPressure;
    statsObj["avg_current"] = stats.avgCurrent;
    statsObj["samples"] = stats.sampleCount;
    
    char buffer[512];
    serializeJson(doc, buffer);
    
    mqttClient->publish(MQTT_TOPIC_REMOTE_RESPONSE, buffer);
}

// ================================================================
// 
// ================================================================
void RemoteManager::handleRemoteCalibrate(const char* sensorType) {
    if (strcmp(sensorType, "pressure") == 0) {
        calibratePressure();
        sendResponse("   ");
    }
    else if (strcmp(sensorType, "current") == 0) {
        calibrateCurrent();
        sendResponse("   ");
    }
    else if (strcmp(sensorType, "temperature") == 0) {
        calibrateTemperature();
        sendResponse("   ");
    }
    else {
        sendResponse("    ", false);
    }
}

// ================================================================
//  
// ================================================================
void RemoteManager::handleRemoteSettings(const char* key, const char* value) {
    // : target_pressure, pid_kp 
    char msg[64];
    
    if (strcmp(key, "target_pressure") == 0) {
        float newValue = atof(value);
        // config.targetPressure = newValue;
        // saveConfig();
        snprintf(msg, sizeof(msg), "  : %.1f kPa", newValue);
        sendResponse(msg);
    }
    else {
        snprintf(msg, sizeof(msg), "   : %s", key);
        sendResponse(msg, false);
    }
}

// ================================================================
//  
// ================================================================
void RemoteManager::update() {
    if (!remoteSessionActive) {
        return;
    }
    
    //   
    uint32_t elapsed = millis() - remoteSessionStart;
    if (elapsed >= remoteSessionTimeout) {
        Serial.println("[RemoteManager]   ");
        terminateRemoteSession();
        sendResponse("  ( )");
    }
}

// ================================================================
//  
// ================================================================
void RemoteManager::publishStatus() {
    if (!mqttClient || !mqttClient->connected()) {
        return;
    }
    
    StaticJsonDocument<256> doc;
    
    doc["remote_session"] = remoteSessionActive;
    doc["mode"] = systemController.getModeString();
    doc["client_id"] = remoteClientId;
    
    if (remoteSessionActive) {
        uint32_t remaining = remoteSessionTimeout - (millis() - remoteSessionStart);
        doc["timeout_remaining"] = remaining / 1000;
    }
    
    char buffer[256];
    serializeJson(doc, buffer);
    
    mqttClient->publish(MQTT_TOPIC_REMOTE_STATUS, buffer);
}

// ================================================================
//    ()
// ================================================================
void RemoteManager::publishSensorData() {
    if (!mqttClient || !mqttClient->connected() || !remoteSessionActive) {
        return;
    }
    
    handleRemoteSensorData();  // 
}

// ================================================================
//  
// ================================================================
bool RemoteManager::verifyRemotePassword(const char* password) {
    return strcmp(password, MANAGER_PASSWORD) == 0;
}

// ================================================================
//  
// ================================================================
void RemoteManager::sendResponse(const char* message, bool success) {
    if (!mqttClient || !mqttClient->connected()) {
        return;
    }
    
    StaticJsonDocument<256> doc;
    doc["success"] = success;
    doc["message"] = message;
    doc["timestamp"] = millis();
    
    char buffer[256];
    serializeJson(doc, buffer);
    
    mqttClient->publish(MQTT_TOPIC_REMOTE_RESPONSE, buffer);
}

// ================================================================
// 
// ================================================================
bool RemoteManager::isRemoteLocked() const {
    if (remoteLoginAttempts < 3) {
        return false;
    }
    return millis() < remoteLockoutEnd;
}

void RemoteManager::resetRemoteAttempts() {
    remoteLoginAttempts = 0;
    remoteLockoutEnd = 0;
}