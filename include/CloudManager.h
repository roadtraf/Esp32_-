/*
 * CloudManager.h - v3.9.1 Phase 1 
 * ThingSpeak   + String  char[] 
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "ThingSpeak.h"
#include "Config.h"

//  ThingSpeak   
#define UPDATE_INTERVAL  (60 * 1000)  // 60 (ThingSpeak  )

//    
struct CloudDataPoint {
    float pressure;
    float temperature;
    float current;
    float healthScore;
    uint32_t timestamp;
};

//  CloudManager  
class CloudManager {
public:
    CloudManager();
    
    // 
    bool begin();
    
    // v3.8:  
    bool uploadExtendedData();      //  +  
    bool uploadTrendData();         //    ( )
    bool uploadAlertData(           //   ( )
        MaintenanceLevel level,
        float healthScore,
        const char* message
    );
    
    //  ( )
    bool uploadData(
        float pressure,
        float temperature,
        float current,
        float healthScore,
        int mode,
        int errorStatus,
        float uptime,
        int errorCode
    );
    
    //  
    bool shouldUpdate();
    
    //  
    bool isCloudConnected();
    
    //  
    void bufferData(float pressure, float temperature, float current, float healthScore);
    CloudDataPoint getBufferedData();
    
    // 
    void printStatistics();
    
    // v3.9.1: String  char[] 
    void getSystemStatusString(char* buffer, size_t size);
    
private:
    WiFiClient client;
    CloudDataPoint dataBuffer;
    uint32_t lastUpdateTime;
    bool isConnected;
};
