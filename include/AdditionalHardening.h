// ================================================================
// AdditionalHardening.h -       
// v3.9.4 Hardened Edition - Additional Issues
// ================================================================
//   :
//  [A]   (sensorData/currentState/stats)  
//  [B] ledcWrite   - PWM  
//  [C] Preferences(NVS)  - Flash write corruption
//  [D] Serial.print   -  
//  [E]   - SmartAlert char[1024] 
//  [F] MQTT callback  changeState() - ISR-like 
//  [G] OTA  /  -  
//  [H] ADC + WiFi   (ESP32 ADC2 )
//  [I] DFPlayer UART    play -   
//  [J] volatile  -    
//  [K] NTP   SD  1970 
//  [L]     -  
// ================================================================
#pragma once

#include <Arduino.h>

//  [A]    
// SensorData, Statistics SensorTask  UITask/ControlTask 
// volatile     atomic  
// : FreeRTOS Mutex / 

//  [B] PWM   
// Control.cpp (VacuumCtrl Task) + ControlManager.cpp + UIManager.cpp
//    0  write    PWM 
#define PWM_MUTEX_TIMEOUT_MS        50

//  [C] NVS(Preferences)  
// ESP32 NVS  Flash 
//  write  NVS wear-leveling    
#define NVS_MUTEX_TIMEOUT_MS        200

//  [D] Serial   
//   Serial.printf     
//  :   
#ifdef RELEASE_BUILD
  #define SAFE_LOG(fmt, ...)   do {} while(0)
  #define SAFE_LOGLN(msg)      do {} while(0)
#else
  #define SAFE_LOG(fmt, ...)   SafeSerial::printf(fmt, ##__VA_ARGS__)
  #define SAFE_LOGLN(msg)      SafeSerial::println(msg)
#endif

//  [E]    
// SmartAlert::sendEmail(): char emailBody[1024]  MQTTHandler Task  4096
//       
// : Task   
#define STACK_VACUUM_CTRL   4096
#define STACK_SENSOR_READ   3072
#define STACK_UI_UPDATE     10240   // [] LovyanGFX  + SPI Guard
#define STACK_WIFI_MGR      4096
#define STACK_MQTT_HANDLER  6144   // [] char buffer[512]  
#define STACK_DATA_LOGGER   4096
#define STACK_HEALTH_MON    3072
#define STACK_PREDICTOR     4096
#define STACK_DS18B20       2048
#define STACK_VOICE_ALERT   3072   // [] VoiceAlert 

//     (uxTaskGetStackHighWaterMark)
#define STACK_WARN_WORDS    256    // 256 words = 1KB  

//  [F] MQTT callback  
// mqttCallback() mqttLoop()   
//  Core0 MQTTHandler Task  
//  changeState() Core1 VacuumCtrl Task   
// :     FreeRTOS Queue  
#define MQTT_CMD_QUEUE_SIZE  8     // MQTT   
#define MQTT_CMD_TIMEOUT_MS  100   //   

//  [G] OTA   
// OTA  :  /       
// : OTA onStart /  OFF
#define OTA_SAFE_SHUTDOWN_DELAY_MS  500  //   OTA  

//  [H] ADC   
// ESP32 (non-S3): ADC2 WiFi    
// ESP32-S3: ADC2   () -  ADC calibration 
// GPIO4(), GPIO5()  ESP32-S3 ADC1 ()
// : GPIO4 = ADC1_CH3, GPIO5 = ADC1_CH4  
// , analogRead()    Mutex 
#define ADC_MUTEX_TIMEOUT_MS  20
#define ADC_OVERSAMPLE_COUNT   4  //   
#define ADC_REJECT_THRESHOLD   0.15f  // 15%    

//  [I] DFPlayer   
//  /  play()  
// DFPlayer UART  
#define VOICE_QUEUE_SIZE     8    //    
#define VOICE_MUTEX_TIMEOUT_MS 50

//  [J] volatile  
//    
// currentState, errorActive, sleepMode   

//  [K] NTP   
// time(nullptr) < 100000  1970-01-01 00:01:xx
//  SD : /reports/daily_19700101.txt ( )
#define NTP_VALID_THRESHOLD  1700000000UL  // 2023  
#define NTP_FALLBACK_PREFIX  "BOOT"        //    prefix

//  [L]   
//   : 5~50ms  ON/OFF 
// : digitalRead()      
#define ESTOP_DEBOUNCE_MS    20   //   
#define ESTOP_CONFIRM_COUNT   3   //    (20ms  3 = 60ms)
