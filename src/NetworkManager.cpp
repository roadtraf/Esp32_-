// ================================================================
// NetworkManager.cpp -   
// ESP32-S3 v3.9.2 Phase 3-1 - Step 5
// ================================================================
#include "Config.h"
#include "../include/NetworkManager.h"

// FreeRTOS (delay )
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

//  
AppNetworkManager networkManager;

//  
extern SystemConfig config;
extern WiFiClient wifiClient;
extern PubSubClient mqttClient;

// ================================================================
// 
// ================================================================
void AppNetworkManager::begin() {
    Serial.println("[NetworkMgr]  ...");
    
    wifiConnected = false;
    mqttConnected = false;
    autoReconnect = true;
    
    lastWiFiCheck = 0;
    lastMQTTCheck = 0;
    lastPublish = 0;
    lastCloudUpload = 0;
    
    // MQTT  
    mqttClient.setCallback(mqttCallback);
    
    Serial.println("[NetworkMgr]   ");
}

void AppNetworkManager::update() {
    if (autoReconnect) {
        checkConnections();
    }
    
    if (mqttConnected) {
        mqttLoop();
    }
}

// ================================================================
// WiFi 
// ================================================================
bool AppNetworkManager::connectWiFi() {
    if (strlen(config.wifiSSID) == 0) {
        Serial.println("[NetworkMgr] WiFi SSID ");
        return false;
    }
    
    Serial.printf("[NetworkMgr] WiFi  : %s\n", config.wifiSSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.wifiSSID, config.wifiPassword);
    
    uint32_t startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        char ipBuf[16]; WiFi.localIP().toString().toCharArray(ipBuf, sizeof(ipBuf));
        Serial.printf("[NetworkMgr]  WiFi : %s\n", ipBuf);
        Serial.printf("[NetworkMgr] RSSI: %d dBm\n", WiFi.RSSI());
        return true;
    } else {
        wifiConnected = false;
        Serial.println("[NetworkMgr]  WiFi  ");
        return false;
    }
}

void AppNetworkManager::disconnectWiFi() {
    WiFi.disconnect();
    wifiConnected = false;
    Serial.println("[NetworkMgr] WiFi  ");
}

bool AppNetworkManager::isWiFiConnected() {
    wifiConnected = (WiFi.status() == WL_CONNECTED);
    return wifiConnected;
}

int AppNetworkManager::getWiFiRSSI() {
    return WiFi.RSSI();
}

// ================================================================
// MQTT 
// ================================================================
bool AppNetworkManager::connectMQTT() {
    if (!isWiFiConnected()) {
        Serial.println("[NetworkMgr] WiFi , MQTT  ");
        return false;
    }
    
    if (strlen(config.mqttBroker) == 0) {
        Serial.println("[NetworkMgr] MQTT   ");
        return false;
    }
    
    mqttClient.setServer(config.mqttBroker, config.mqttPort);
    
    Serial.printf("[NetworkMgr] MQTT  : %s:%d\n", 
                  config.mqttBroker, config.mqttPort);
    
    char clientId[32];
    snprintf(clientId, sizeof(clientId), "ESP32-%08x", (uint32_t)ESP.getEfuseMac());
    
    if (mqttClient.connect(clientId, config.mqttUser, config.mqttPassword)) {
        mqttConnected = true;
        Serial.println("[NetworkMgr]  MQTT ");
        
        //  
        mqttClient.subscribe("vacuum/control/#");
        
        return true;
    } else {
        mqttConnected = false;
        Serial.printf("[NetworkMgr]  MQTT   (: %d)\n", mqttClient.state());
        return false;
    }
}

void AppNetworkManager::disconnectMQTT() {
    mqttClient.disconnect();
    mqttConnected = false;
    Serial.println("[NetworkMgr] MQTT  ");
}

bool AppNetworkManager::isMQTTConnected() {
    mqttConnected = mqttClient.connected();
    return mqttConnected;
}

void AppNetworkManager::mqttLoop() {
    if (mqttConnected) {
        mqttClient.loop();
    }
}

// ================================================================
//  
// ================================================================
void AppNetworkManager::publishSensorData() {
    if (!mqttConnected) return;
    
    extern SensorData sensorData;
    
    char payload[200];
    snprintf(payload, sizeof(payload),
             "{\"pressure\":%.1f,\"temperature\":%.1f,\"current\":%.2f}",
             sensorData.pressure, sensorData.temperature, sensorData.current);
    
    mqttClient.publish("vacuum/sensors", payload);
}

void AppNetworkManager::publishSystemStatus() {
    if (!mqttConnected) return;
    
    extern SystemState currentState;
    extern bool errorActive;
    
    char payload[100];
    snprintf(payload, sizeof(payload),
             "{\"state\":%d,\"error\":%s}",
             (int)currentState, errorActive ? "true" : "false");
    
    mqttClient.publish("vacuum/status", payload);
}

void AppNetworkManager::publishCustom(const char* topic, const char* payload) {
    if (!mqttConnected) return;
    mqttClient.publish(topic, payload);
}

// ================================================================
//  
// ================================================================
void AppNetworkManager::uploadToCloud() {
    if (!isWiFiConnected()) return;
    
    uint32_t now = millis();
    if (now - lastCloudUpload < 60000) return;  // 1 1
    
    // CloudManager 
    #ifdef ENABLE_CLOUD
    extern CloudManager cloudManager;
    extern SensorData sensorData;
    extern HealthMonitor healthMonitor;
    extern SystemState currentState;
    extern bool errorActive;
    extern Statistics stats;
    extern ErrorInfo currentError;
    
    bool success = cloudManager.uploadData(
        sensorData.pressure,
        sensorData.temperature,
        sensorData.current,
        healthMonitor.getHealthScore(),
        (int)currentState,
        errorActive ? 1 : 0,
        stats.uptime / 3600.0f,
        currentError.code
    );
    
    if (success) {
        Serial.println("[NetworkMgr]    ");
    }
    #endif
    
    lastCloudUpload = now;
}

// ================================================================
//  
// ================================================================
void AppNetworkManager::enableAutoReconnect(bool enable) {
    autoReconnect = enable;
    Serial.printf("[NetworkMgr]  : %s\n", enable ? "" : "");
}

void AppNetworkManager::checkConnections() {
    uint32_t now = millis();
    
    // WiFi  (5)
    if (now - lastWiFiCheck >= 5000) {
        lastWiFiCheck = now;
        
        if (!isWiFiConnected()) {
            Serial.println("[NetworkMgr] WiFi  ,  ...");
            attemptWiFiReconnect();
        }
    }
    
    // MQTT  (10)
    if (now - lastMQTTCheck >= 10000) {
        lastMQTTCheck = now;
        
        if (isWiFiConnected() && !isMQTTConnected()) {
            Serial.println("[NetworkMgr] MQTT  ,  ...");
            attemptMQTTReconnect();
        }
    }
}

bool AppNetworkManager::attemptWiFiReconnect() {
    return connectWiFi();
}

bool AppNetworkManager::attemptMQTTReconnect() {
    return connectMQTT();
}

// ================================================================
// MQTT 
// ================================================================
void AppNetworkManager::mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.printf("[NetworkMgr] MQTT : %s\n", topic);
    
    //    
    // (    )
}

// ================================================================
//  
// ================================================================
void AppNetworkManager::printStatus() {
    Serial.println("\n");
    Serial.println("                           ");
    Serial.println("");
    Serial.printf(" WiFi: %s                              \n",
                  wifiConnected ? " " : " ");
    
    if (wifiConnected) {
        Serial.printf(" SSID: %-31s \n", WiFi.SSID().c_str());
        char ipBuf2[16]; WiFi.localIP().toString().toCharArray(ipBuf2, sizeof(ipBuf2));
        Serial.printf(" IP: %-33s \n", ipBuf2);
        Serial.printf(" RSSI: %d dBm                          \n", WiFi.RSSI());
    }
    
    Serial.println("");
    Serial.printf(" MQTT: %s                              \n",
                  mqttConnected ? " " : " ");
    
    if (mqttConnected) {
        Serial.printf(" : %-31s \n", config.mqttBroker);
    }
    
    Serial.println("");
    Serial.printf("  : %s                       \n",
                  autoReconnect ? "" : "");
    Serial.println("\n");
}
