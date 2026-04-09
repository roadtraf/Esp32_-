#pragma once
#include <ArduinoJson.h>
// ================================================================
// Network.h    WiFi, MQTT,  , , 
// v4.0 -  MQTT    
// ================================================================

//  WiFi 
void initWiFi();
void connectWiFi();

//  MQTT 
void initMQTT();
void connectMQTT();
void publishMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void mqttLoop();   // loop()    

//  MQTT   () 
void subscribeToTopics();              //    
void handleMQTTCommand(const char* cmd, JsonDocument& doc);  //  
void publishSystemStatus();            //    
void publishSensorData();              //   
void publishAlarmState();              //   
void publishConfigUpdate();            //   

//  NTP 
void initNTP();   // wifiConnected   syncTime() 

//    
void saveConfig();
void loadConfig();

//  Watchdog 
void initWatchdog();
void feedWatchdog();

//    
void handleSerialCommand();

//   
void enterSleepMode();
void exitSleepMode();

//   
void printMemoryInfo();
void printStatistics();
