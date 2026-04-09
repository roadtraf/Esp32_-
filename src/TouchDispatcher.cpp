// ================================================================
// TouchDispatcher.cpp    
// 
// Tasks.cpp   handleTouch() 
// UIManager::handleTouch()    
// ================================================================
#include "Config.h"
#include "UI_Screens.h"
#include "UIManager.h"

extern TFT_GFX tft;
extern ScreenType currentScreen;
extern uint32_t lastIdleTime;
extern bool sleepMode;
extern UIManager uiManager;

// ================================================================
//   
// ================================================================
static bool getTouch(uint16_t* x, uint16_t* y) {
    return tft.getTouch(x, y);
}

// ================================================================
//       
// ================================================================
void handleTouch() {
    uint16_t x = 0, y = 0;
    
    if (!getTouch(&x, &y)) {
        return;  //  
    }
    
    //    ( )
    lastIdleTime = millis();
    if (sleepMode) {
        extern void exitSleepMode();
        exitSleepMode();
        return;  //    
    }
    
    //    
    switch (currentScreen) {
        case SCREEN_MAIN:
            handleMainTouch(x, y);
            break;
        
        case SCREEN_SETTINGS:
            handleSettingsTouch(x, y);
            break;
        
        case SCREEN_ALARM:
            handleAlarmTouch(x, y);
            break;
        
        case SCREEN_TIMING_SETUP:
            handleTimingTouch(x, y);
            break;
        
        case SCREEN_PID_SETUP:
            handlePIDTouch(x, y);
            break;
        
        case SCREEN_STATISTICS:
            handleStatisticsTouch(x, y);
            break;
        
        case SCREEN_CALIBRATION:
            handleCalibrationTouch(x, y);
            break;
        
        case SCREEN_ABOUT:
            handleAboutTouch(x, y);
            break;
        
        case SCREEN_HELP:
            handleHelpTouch(x, y);
            break;
        
        case SCREEN_TREND_GRAPH:
            handleTrendGraphTouch(x, y);
            break;
        
        case SCREEN_STATE_DIAGRAM:
            handleStateDiagramTouch(x, y);
            break;
        
        case SCREEN_WATCHDOG_STATUS:
            handleWatchdogStatusTouch(x, y);
            break;
        
        // [U5] E-Stop  
        case SCREEN_ESTOP:
            handleEStopTouch(x, y);
            break;
        
#ifdef ENABLE_PREDICTIVE_MAINTENANCE
        case SCREEN_HEALTH:
            handleHealthTouch(x, y);
            break;
        
        case SCREEN_HEALTH_TREND:
            handleHealthTrendTouch(x, y);
            break;
#endif

#ifdef ENABLE_SMART_ALERTS
        case SCREEN_SMART_ALERT_CONFIG:
            handleSmartAlertConfigTouch(x, y);
            break;
#endif

#ifdef ENABLE_VOICE_ALERTS
        case SCREEN_VOICE_SETTINGS:
            handleVoiceSettingsTouch(x, y);
            break;
#endif

#ifdef ENABLE_ADVANCED_ANALYSIS
        case SCREEN_ADVANCED_ANALYSIS:
            handleAdvancedAnalysisTouch(x, y);
            break;
#endif
        
        default:
            //       
            uiManager.setScreen(SCREEN_MAIN);
            break;
    }
}

// ================================================================
// updateUI  (Tasks.cpp )
// ================================================================
void updateUI() {
	extern UIManager uiManager;
    uiManager.drawCurrentScreen();
}
