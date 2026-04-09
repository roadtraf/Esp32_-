// ================================================================
// RemoteManager.h - MQTT   
// Phase 2: MQTT    
// ================================================================
#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include "../include/SystemController.h"

// ================================================================
// MQTT  
// ================================================================
#define MQTT_TOPIC_REMOTE_CMD       "vacuum/remote/command"
#define MQTT_TOPIC_REMOTE_RESPONSE  "vacuum/remote/response"
#define MQTT_TOPIC_REMOTE_STATUS    "vacuum/remote/status"
#define MQTT_TOPIC_REMOTE_LOGIN     "vacuum/remote/login"

// ================================================================
//   
// ================================================================
struct RemoteCommand {
    char action[32];        // login, logout, calibrate, status 
    char password[32];      //  
    char parameter[64];     //  
    uint32_t timestamp;     //  
};

// ================================================================
//   
// ================================================================
class RemoteManager {
private:
    PubSubClient* mqttClient;
    
    //   
    bool remoteSessionActive;
    uint32_t remoteSessionStart;
    uint32_t remoteSessionTimeout;
    char remoteClientId[32];
    
    // 
    uint8_t remoteLoginAttempts;
    uint32_t remoteLockoutEnd;
    
    //  
    bool verifyRemotePassword(const char* password);
    void sendResponse(const char* message, bool success = true);
    void processRemoteCommand(const char* action, const char* parameter);
    
    //  
    void handleRemoteLogin(const char* password, const char* clientId);
    void handleRemoteLogout();
    void handleRemoteStatus();
    void handleRemoteCalibrate(const char* sensorType);
    void handleRemoteSensorData();
    void handleRemoteSettings(const char* setting, const char* value);
    
public:
    RemoteManager();
    
    // 
    void begin(PubSubClient* client);
    
    // MQTT  
    void handleMQTTMessage(const char* topic, const char* payload);
    
    //  
    void update();
    
    //   
    bool isRemoteSessionActive() const { return remoteSessionActive; }
    void terminateRemoteSession();
    
    //  
    void publishStatus();
    void publishSensorData();
    
    // 
    bool isRemoteLocked() const;
    void resetRemoteAttempts();
};

extern RemoteManager remoteManager;