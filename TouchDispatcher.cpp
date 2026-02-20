// ================================================================
// TouchDispatcher.cpp — 화면별 터치 라우팅
// 
// Tasks.cpp에서 호출하는 전역 handleTouch() 구현
// UIManager::handleTouch()와는 별개로 실제 디스패치 수행
// ================================================================
#include "../include/Config.h"
#include "../include/UI_Screens.h"
#include "../include/UIManager.h"

extern LGFX tft;
extern ScreenType currentScreen;
extern uint32_t lastIdleTime;
extern bool sleepMode;
extern UIManager uiManager;

// ================================================================
// 터치 좌표 읽기
// ================================================================
static bool getTouch(uint16_t* x, uint16_t* y) {
    return tft.getTouch(x, y);
}

// ================================================================
// 전역 터치 디스패처 — 화면별 핸들러 호출
// ================================================================
void handleTouch() {
    uint16_t x = 0, y = 0;
    
    if (!getTouch(&x, &y)) {
        return;  // 터치 없음
    }
    
    // 활동 시간 갱신 (절전 해제)
    lastIdleTime = millis();
    if (sleepMode) {
        extern void exitSleepMode();
        exitSleepMode();
        return;  // 첫 터치는 화면만 켬
    }
    
    // 화면별 터치 핸들러 디스패치
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
        
        // [U5] E-Stop 화면 추가
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
            // 알 수 없는 화면 — 메인으로 복귀
            uiManager.setScreen(SCREEN_MAIN);
            break;
    }
}

// ================================================================
// updateUI 디스패처 (Tasks.cpp에서 호출)
// ================================================================
void updateUI() {
	extern UIManager uiManager;
    uiManager.drawCurrentScreen();
}
