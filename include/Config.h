// Config.h    ESP32-S3    v3.9.5 
//(UI  +  )

#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Sensor.h"        //   
#include "SensorBuffer.h"
#include "Lang.h"  //   
#include "SystemController.h"   // SystemController 
#include "RemoteManager.h"    // Phase 2:  
#include "UITheme.h"
#include "UIComponents.h"
#include "EnhancedWatchdog.h"
#include "ConfigManager.h"
#include "SafeMode.h"       
#include "WiFiResilience.h"    

#ifndef ENABLE_MQTT   // MQTT  
#define ENABLE_MQTT
#endif

//    
#define FIRMWARE_VERSION "v3.9.5"
#define BUILD_DATE "2026-02-18"


//  v3.8.0      
// #define UNIT_TEST_MODE

//  v3.8.0     
#define ENABLE_PREDICTIVE_MAINTENANCE

//  v3.8.1:  ,    
#define ENABLE_DATA_LOGGING           //  
#define ENABLE_TREND_ANALYSIS         //  

//  v3.8.2:    
#define ENABLE_SMART_ALERTS

//  v3.8.3:    
#define ENABLE_ADVANCED_ANALYSIS

//  v3.8.3: ThingSpeak  
#define ENABLE_THINGSPEAK
#define THINGSPEAK_CHANNEL_ID     123456      // 
#define THINGSPEAK_WRITE_KEY      "YOUR_WRITE_KEY"

// v3.8   ()
#define THINGSPEAK_CHANNEL_TREND    234567
#define THINGSPEAK_WRITE_KEY_TREND  "YOUR_KEY"
#define THINGSPEAK_CHANNEL_ALERT    345678
#define THINGSPEAK_WRITE_KEY_ALERT  "YOUR_KEY"

// v3.9    ()
#define DEFAULT_LANGUAGE    LANGUAGE_KOREAN
#define ENABLE_VOICE_ALERTS

//    (0-30)
#define VOICE_VOLUME_DEFAULT    20    //  
#define VOICE_VOLUME_ERROR      25    //   
#define VOICE_VOLUME_EMERGENCY  30    //   

//   
//  (    !)
#define MANAGER_PASSWORD    "admin1234"
#define DEVELOPER_PASSWORD  "dev5678"

//    ()
#define AUTO_LOGOUT_TIME    300000      // 5 ( )

//    
//   (IRF540 4CH MOSFET)
#define PIN_PUMP_PWM        1
#define PIN_VALVE           2
#define PIN_12V_MAIN        42
#define PIN_12V_EMERGENCY   43

// DFPlayer Mini 
#define DFPLAYER_RX_PIN     17    // ESP32 RX  DFPlayer TX
#define DFPLAYER_TX_PIN     18    // ESP32 TX  DFPlayer RX
#define DFPLAYER_UART       2     // UART2 
#define DFPLAYER_BAUD       9600

//  
#define PIN_PRESSURE_SENSOR 4
#define PIN_CURRENT_SENSOR  5
#define PIN_TEMP_SENSOR     14    // DS18B20  
#define PIN_LIMIT_SWITCH    40
#define PIN_PHOTO_SENSOR    41
#define PIN_EMERGENCY_STOP  39
#define PIN_ESTOP           0     // GPIO0 (E-Stop , main_hardened )

//  
#define PIN_BUZZER          38
#define PIN_LED_GREEN       37
#define PIN_LED_RED         36

//    

// ================================================================
//    (v3.9.5  )
// ================================================================
//  : DC12V 42W,   -85kPa,  40L/min
//  :  90% = -76.5kPa (  )
// ================================================================

// PID  
#define TARGET_PRESSURE        -60.0f   // kPa   (5~10kg) 
#define PRESSURE_HYSTERESIS      2.0f   // kPa  2kPa  

//   
#define PRESSURE_ALARM_KPA     -80.0f   // kPa   ( )
#define PRESSURE_TRIP_KPA      -85.0f   // kPa   ( , )

// UI  
#define PRESSURE_MIN_KPA      -100.0f   // kPa  / 
#define PRESSURE_MAX_KPA         0.0f   // kPa  /  ()

// PID 
#define PID_KP                  2.0f
#define PID_KI                  0.5f
#define PID_KD                  1.0f
#define PID_OUTPUT_MIN          0.0f
#define PID_OUTPUT_MAX        100.0f
#define INTEGRAL_LIMIT         50.0f

// ================================================================
//   (v3.9.5  )
// ================================================================
//  : DC12V 42W    3.5A
//  :  1.4(5A) , 1.7(6A) 
// ================================================================

//   
#define CURRENT_THRESHOLD_WARNING   5.0f    // A   ( )
#define CURRENT_ALARM_A             5.0f    // A  UI   ()

#define CURRENT_THRESHOLD_CRITICAL  6.0f    // A   ()
#define CURRENT_TRIP_A              6.0f    // A   ()

// UI  
#define CURRENT_MIN_A               0.0f    // A  / 
#define CURRENT_MAX_A               8.0f    // A  / 

// ================================================================
//   (v3.9.5  )
// ================================================================
//     : 0~50C
//  : 50C , 60C , 70C  
// ================================================================

//   
#define TEMP_THRESHOLD_WARNING   50.0f   // C   
#define TEMP_ALARM_C             50.0f   // C  UI   ()

#define TEMP_THRESHOLD_CRITICAL  60.0f   // C  
#define TEMP_TRIP_C              60.0f   // C    ()

#define TEMP_THRESHOLD_SHUTDOWN  70.0f   // C    

// UI  
#define TEMP_MIN_C              -10.0f   // C   
#define TEMP_MAX_C               80.0f   // C   

// ================================================================
// 
// ================================================================
#define UPDATE_INTERVAL       100      // ms    
#define PID_UPDATE_INTERVAL    50      // ms  PID  
#define DEBOUNCE_TIME          50      // ms
#define WDT_TIMEOUT            10      // 
#define IDLE_TIMEOUT  (2UL * 60 * 1000) // 2

// ================================================================
// PWM
// ================================================================
#define PWM_FREQUENCY   1000
#define PWM_RESOLUTION     8
#define PWM_CHANNEL_PUMP   0
#define PWM_MIN           50
#define PWM_MAX          255

// ================================================================
// 
// ================================================================
#define SCREEN_WIDTH   480
#define SCREEN_HEIGHT  320

//   
enum SystemState {
  STATE_IDLE,
  STATE_VACUUM_ON,
  STATE_VACUUM_HOLD,
  STATE_VACUUM_BREAK,
  STATE_WAIT_REMOVAL,
  STATE_COMPLETE,
  STATE_ERROR,
  STATE_EMERGENCY_STOP,
  STATE_STANDBY 
};

enum ControlMode {
  MODE_MANUAL,
  MODE_AUTO,
  MODE_PID
};

enum ErrorCode {
  ERROR_NONE,
  ERROR_OVERCURRENT,
  ERROR_SENSOR_FAULT,
  ERROR_MOTOR_FAILURE,
  ERROR_PHOTO_TIMEOUT,
  ERROR_EMERGENCY_STOP,
  ERROR_WATCHDOG,
  ERROR_MEMORY,
  ERROR_OVERHEAT,           // 
  ERROR_TEMP_SENSOR_FAULT,  //  
  ERROR_VACUUM_FAIL
};

enum ScreenType {
  SCREEN_MAIN,
  SCREEN_SETTINGS,
  SCREEN_TIMING_SETUP,
  SCREEN_PID_SETUP,
  SCREEN_STATISTICS,
  SCREEN_ALARM,
  SCREEN_ABOUT,
  SCREEN_HELP,
  SCREEN_CALIBRATION,
  SCREEN_STATE_DIAGRAM,
  SCREEN_TREND_GRAPH,
  SCREEN_WATCHDOG_STATUS,
  SCREEN_ESTOP,             // [v3.9.5]    
  
  #ifdef ENABLE_PREDICTIVE_MAINTENANCE
  SCREEN_HEALTH,
  SCREEN_HEALTH_TREND,
  #endif
      
  #ifdef ENABLE_SMART_ALERTS
  SCREEN_SMART_ALERT_CONFIG,
  #endif  
      
  #ifdef ENABLE_VOICE_ALERTS
  SCREEN_VOICE_SETTINGS,
  #endif
    
  #ifdef ENABLE_ADVANCED_ANALYSIS
  SCREEN_ADVANCED_ANALYSIS,
  SCREEN_FAILURE_PREDICTION,
  SCREEN_COMPONENT_LIFE,
  SCREEN_OPTIMIZATION,
  SCREEN_COMPREHENSIVE_REPORT,
  SCREEN_COST_ANALYSIS,
  #endif
};

//   extern 

// 
#include "GFX_Wrapper.hpp"
// extern LGFX tft;

//  
#include "SystemController.h"
extern SystemController systemController;

//  
extern ScreenType currentScreen;
extern bool screenNeedsRedraw;

//  




//  
#define ERROR_HIST_MAX 10

enum ErrorSeverity {
    SEVERITY_INFO,
    SEVERITY_WARNING,
    SEVERITY_RECOVERABLE,
    SEVERITY_CRITICAL
};

// extern ErrorInfo errorHistory[ERROR_HIST_MAX];  // moved below struct ErrorInfo
extern uint8_t errorHistIdx;
extern uint8_t errorHistCnt;
extern bool errorActive;
// extern ErrorInfo currentError;  // moved below struct ErrorInfo

// ================================================================
//  
// ================================================================

#ifdef ENABLE_PREDICTIVE_MAINTENANCE
#include "HealthMonitor.h"
extern HealthMonitor healthMonitor;
#endif

#ifdef ENABLE_SMART_ALERTS
#include "SmartAlert.h"
// extern SmartAlert smartAlert;  // SmartAlert.h  
#endif

#ifdef ENABLE_VOICE_ALERTS
#include "VoiceAlert.h"
// extern SafeVoiceAlert safeVoiceAlert;
#endif

//   
struct SystemConfig {
  float      targetPressure;
  float      pressureHysteresis;
  float      pidKp, pidKi, pidKd;
  uint32_t   vacuumOnTime;
  uint32_t   vacuumHoldTime;
  uint32_t   vacuumHoldExtension;   //    (ms)
  uint8_t    maxHoldExtensions;     //   
  uint32_t   vacuumBreakTime;
  uint32_t   waitRemovalTime;
  float      tempWarning;           //  
  float      tempCritical;          //  
  float      tempShutdown;          //  
  ControlMode controlMode;
  bool       buzzerEnabled;
  bool       holdExtensionEnabled;  //   
  bool       tempSensorEnabled;     //  
  uint8_t  backlightLevel;  
  char      wifiSSID[32];
  char      wifiPassword[64];
  char       mqttBroker[64];    // MQTT  
  uint16_t mqttPort;
  char       mqttUser[32];       // MQTT 
  char       mqttPassword[64];   // MQTT 
  uint8_t    language;       // 0=EN, 1=KO
};

#ifndef SENSOR_DATA_DEFINED
#define SENSOR_DATA_DEFINED
struct SensorData {
  float    pressure;
  float    current;
  float    temperature;     //  (C)
  bool     limitSwitch;
  bool     photoSensor;
  bool     emergencyStop;
  uint32_t timestamp;
};
#endif

struct ErrorInfo {
  ErrorCode      code;
  ErrorSeverity  severity;
  uint32_t       timestamp;
  uint8_t        retryCount;
  char           message[128];
};
extern ErrorInfo errorHistory[ERROR_HIST_MAX];
extern ErrorInfo currentError;

#ifndef STATISTICS_DEFINED
#define STATISTICS_DEFINED
struct Statistics {
  uint32_t totalCycles;
  uint32_t successfulCycles;
  uint32_t failedCycles;
  uint32_t totalErrors;
  uint32_t uptime;
  float    averageCycleTime;
  float    minPressure;
  float    maxPressure;
  float    averageCurrent;
};
#endif

//    (extern) 
// () main.cpp    

//  
extern SystemState  currentState;
extern SystemState  previousState;
extern ControlMode  currentMode;
extern ScreenType   currentScreen;

// 
extern SystemConfig  config;
extern Preferences   preferences;

//  
extern ErrorInfo     errorInfo;

// 
extern Statistics    stats;

//  
extern uint32_t      stateStartTime;

// UI 
extern bool          screenNeedsRedraw;
extern uint8_t       helpPageIndex;

// 
extern bool          wifiConnected;
extern bool          mqttConnected;

// SD 
extern bool          sdCardAvailable;

//   (v3.9)
// extern Language currentLanguage;  // Lang.h currentLang 

extern SafeMode safeMode;
extern WiFiResilience wifiResilience;

// ================================================================
// v3.9.5  
// ================================================================
// - : v3.9.4  v3.9.5
// - : 2026-02-18
// - :
//   1. //    ( )
//   2.   :
//      - TARGET_PRESSURE = -60kPa (  90%)
//      - CURRENT_WARNING = 5A ( 1.4)
//      - CURRENT_CRITICAL = 6A ( 1.7)
//   3. UI   :
//      - PRESSURE_MIN/MAX_KPA
//      - CURRENT_MIN/MAX_A
//      - TEMP_MIN/MAX_C
//   4. SCREEN_ESTOP  (  )
//   5. PIN_ESTOP  (GPIO0, main_hardened )
//   6.   (    )
// ================================================================
