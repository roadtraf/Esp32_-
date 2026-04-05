// ================================================================
// UI_Screens.h - 모든 UI 화면 함수 선언
// ================================================================
#pragma once

#include <Arduino.h>
 #include "UI_AccessControl.h"

// ================================================================
// 화면 enum (Config.h에서 정의되어야 함)
// ================================================================
// extern enum ScreenType {
//     SCREEN_MAIN,
//     SCREEN_SETTING
//     ...
// };

bool handleResetConfirmTouch(uint16_t x, uint16_t y);
bool isResetConfirmPending();

// ================================================================
// 메인 화면
// ================================================================
void drawMainScreen();
void handleMainTouch(uint16_t x, uint16_t y);

// ================================================================
// 설정 화면
// ================================================================
void drawSettingsScreen();
void handleSettingsTouch(uint16_t x, uint16_t y);

// ================================================================
// 캘리브레이션 화면
// ================================================================
void drawCalibrationScreen();
void handleCalibrationTouch(uint16_t x, uint16_t y);

// ================================================================
// 통계 화면
// ================================================================
void drawStatisticsScreen();
void handleStatisticsTouch(uint16_t x, uint16_t y);

// ================================================================
// 건강도 화면
// ================================================================
#ifdef ENABLE_PREDICTIVE_MAINTENANCE
void drawHealthScreen();
void handleHealthTouch(uint16_t x, uint16_t y);

void drawHealthTrendScreen();
void handleHealthTrendTouch(uint16_t x, uint16_t y);
#endif

// ================================================================
// 타이밍 설정 화면
// ================================================================
void drawTimingScreen();
void handleTimingTouch(uint16_t x, uint16_t y);

// ================================================================
// PID 설정 화면
// ================================================================
void drawPIDScreen();
void handlePIDTouch(uint16_t x, uint16_t y);

// ================================================================
// 정보 화면
// ================================================================
void drawAboutScreen();
void handleAboutTouch(uint16_t x, uint16_t y);

// ================================================================
// 도움말 화면
// ================================================================
void drawHelpScreen();
void handleHelpTouch(uint16_t x, uint16_t y);

// ================================================================
// 스마트 알림 설정 화면
// ================================================================
#ifdef ENABLE_SMART_ALERTS
void drawSmartAlertConfigScreen();
void handleSmartAlertConfigTouch(uint16_t x, uint16_t y);
#endif

// ================================================================
// 음성 설정 화면
// ================================================================
#ifdef ENABLE_VOICE_ALERTS
void drawVoiceSettingsScreen();
void handleVoiceSettingsTouch(uint16_t x, uint16_t y);
#endif

// ================================================================
// 알람 화면
// ================================================================
void drawAlarmScreen();
void handleAlarmTouch(uint16_t x, uint16_t y);

// ================================================================
// 추세 그래프 화면
// ================================================================
void drawTrendGraphScreen();
void handleTrendGraphTouch(uint16_t x, uint16_t y);

// ================================================================
// 고급 분석 화면
// ================================================================
void drawAdvancedAnalysisScreen();
void handleAdvancedAnalysisTouch(uint16_t x, uint16_t y);

// ================================================================
// 상태 다이어그램 화면
// ================================================================
void drawStateDiagramScreen();
void handleStateDiagramTouch(uint16_t x, uint16_t y);

// drawStateDiagramScreen 호환성 함수 (기존 이름)
void drawStateDiagram();

// ================================================================
// 워치독 상태 화면
// ================================================================
void drawWatchdogStatusScreen();
void handleWatchdogStatusTouch(uint16_t x, uint16_t y);

// ================================================================
// 비상 정지 버튼 화면
// ================================================================
void drawEStopScreen();
void handleEStopTouch(uint16_t x, uint16_t y);
void recordEStopStart(ScreenType prevScreen);

// ================================================================
// 헬퍼 함수
// ================================================================

// 권한 없음 팝업
void showAccessDenied(const char* screenName);

// 유지보수 완료 팝업
#ifdef ENABLE_PREDICTIVE_MAINTENANCE
void showMaintenanceCompletePopup();
#endif

// 통계 초기화 확인 팝업
void showResetConfirmation();

// 온도 센서 정보 팝업
void showTemperatureSensorInfo();

// ================================================================
// 센서 통계 구조체
// ================================================================
struct SensorStats {
    float avgTemperature;
    float avgPressure;
    float avgCurrent;
    uint32_t sampleCount;
};

// 센서 통계 계산
void calculateSensorStats(SensorStats& stats);
