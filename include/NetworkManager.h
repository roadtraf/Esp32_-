// ================================================================
// NetworkManager.h -   
// ESP32-S3 v3.9.2 Phase 3-1 - Step 5
// ================================================================
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ================================================================
//   
// ================================================================
class AppNetworkManager {
public:
    // 
    void begin();
    void update();
    
    // WiFi 
    bool connectWiFi();
    void disconnectWiFi();
    bool isWiFiConnected();
    int getWiFiRSSI();
    
    // MQTT 
    bool connectMQTT();
    void disconnectMQTT();
    bool isMQTTConnected();
    void mqttLoop();
    
    //  
    void publishSensorData();
    void publishSystemStatus();
    void publishCustom(const char* topic, const char* payload);
    
    //  
    void uploadToCloud();
    
    //  
    void enableAutoReconnect(bool enable);
    void checkConnections();
    
    //  
    void printStatus();
    
private:
    bool wifiConnected;
    bool mqttConnected;
    bool autoReconnect;
    
    uint32_t lastWiFiCheck;
    uint32_t lastMQTTCheck;
    uint32_t lastPublish;
    uint32_t lastCloudUpload;
    
    // WiFi 
    bool attemptWiFiReconnect();
    
    // MQTT 
    bool attemptMQTTReconnect();
    
    // MQTT 
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
};

extern AppNetworkManager networkManager;
