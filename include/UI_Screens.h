// ================================================================
// UI_Screens.h -  UI   
// ================================================================
#pragma once

#include <Arduino.h>
 #include "UI_AccessControl.h"

// ================================================================
//  enum (Config.h  )
// ================================================================
// extern enum ScreenType {
//     SCREEN_MAIN,
//     SCREEN_SETTING
//     ...
// };

bool handleResetConfirmTouch(uint16_t x, uint16_t y);
bool isResetConfirmPending();

// ================================================================
//  
// ================================================================
void drawMainScreen();
void handleMainTouch(uint16_t x, uint16_t y);

// ================================================================
//  
// ================================================================
void drawSettingsScreen();
void handleSettingsTouch(uint16_t x, uint16_t y);

// ================================================================
//  
// ================================================================
void drawCalibrationScreen();
void handleCalibrationTouch(uint16_t x, uint16_t y);

// ================================================================
//  
// ================================================================
void drawStatisticsScreen();
void handleStatisticsTouch(uint16_t x, uint16_t y);

// ================================================================
//  
// ================================================================
#ifdef ENABLE_PREDICTIVE_MAINTENANCE
void drawHealthScreen();
void handleHealthTouch(uint16_t x, uint16_t y);

void drawHealthTrendScreen();
void handleHealthTrendTouch(uint16_t x, uint16_t y);
#endif

// ================================================================
//   
// ================================================================
void drawTimingScreen();
void handleTimingTouch(uint16_t x, uint16_t y);

// ================================================================
// PID  
// ================================================================
void drawPIDScreen();
void handlePIDTouch(uint16_t x, uint16_t y);

// ================================================================
//  
// ================================================================
void drawAboutScreen();
void handleAboutTouch(uint16_t x, uint16_t y);

// ================================================================
//  
// ================================================================
void drawHelpScreen();
void handleHelpTouch(uint16_t x, uint16_t y);

// ================================================================
//    
// ================================================================
#ifdef ENABLE_SMART_ALERTS
void drawSmartAlertConfigScreen();
void handleSmartAlertConfigTouch(uint16_t x, uint16_t y);
#endif

// ================================================================
//   
// ================================================================
#ifdef ENABLE_VOICE_ALERTS
void drawVoiceSettingsScreen();
void handleVoiceSettingsTouch(uint16_t x, uint16_t y);
#endif

// ================================================================
//  
// ================================================================
void drawAlarmScreen();
void handleAlarmTouch(uint16_t x, uint16_t y);

// ================================================================
//   
// ================================================================
void drawTrendGraphScreen();
void handleTrendGraphTouch(uint16_t x, uint16_t y);

// ================================================================
//   
// ================================================================
void drawAdvancedAnalysisScreen();
void handleAdvancedAnalysisTouch(uint16_t x, uint16_t y);

// ================================================================
//   
// ================================================================
void drawStateDiagramScreen();
void handleStateDiagramTouch(uint16_t x, uint16_t y);

// drawStateDiagramScreen   ( )
void drawStateDiagram();

// ================================================================
//   
// ================================================================
void drawWatchdogStatusScreen();
void handleWatchdogStatusTouch(uint16_t x, uint16_t y);

// ================================================================
//    
// ================================================================
void drawEStopScreen();
void handleEStopTouch(uint16_t x, uint16_t y);
void recordEStopStart(ScreenType prevScreen);

// ================================================================
//  
// ================================================================

//   
void showAccessDenied(const char* screenName);

//   
#ifdef ENABLE_PREDICTIVE_MAINTENANCE
void showMaintenanceCompletePopup();
#endif

//    
void showResetConfirmation();

//    
void showTemperatureSensorInfo();

// ================================================================
//   
// ================================================================
#ifndef SENSORSTATS_DEFINED
#define SENSORSTATS_DEFINED
struct SensorStats {
    float avgTemperature;
    float avgPressure;
    float avgCurrent;
    uint32_t sampleCount;
};
#endif


//   
void calculateSensorStats(SensorStats& stats);
