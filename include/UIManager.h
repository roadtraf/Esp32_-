// ================================================================
// UIManager.h - UI 관리자 개선판
// [U3] showToast() 비동기, showMessage() 타이머 기반
// [U5] E-Stop 감지 + drawEStopScreen() 자동 전환
// [U8] screenNeedsRedraw/currentScreen Mutex 보호
// ================================================================
#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "../include/Config.h"
#include "../include/UITheme.h"
#include "../include/SystemController.h"

class UIManager {
public:
    // ── 초기화 / 업데이트 ──
    void begin();
    void update();

    // ── 화면 관리 [U8] ──
    void setScreen(ScreenType screen);       // Mutex 보호 포함
    ScreenType getCurrentScreen() const { return currentScreen; }
    ScreenType getPreviousScreen() const { return previousScreen; }
    void redrawScreen();
    void requestRedraw();

    // ── 터치 처리 ──
    void handleTouch();

    // ── 메시지 (타이머 기반, 비블로킹) [U3] ──
    void showMessage(const char* message, uint32_t duration = 2000);
    void hideMessage();
    bool isMessageActive() const { return messageActive; }

    // ── Toast 오버레이 [U3] ──
    void showToast(const char* message, uint16_t color);
    void drawToastOverlay();
    bool isToastActive() const { return toastActive; }

    // ── 팝업 ──
    void showPopup(const char* label, float* target, float min, float max,
                   float step, uint8_t decimals);
    void showPopupU32(const char* label, uint32_t* target, uint32_t min,
                      uint32_t max, uint32_t step);
    void hidePopup();
    bool isPopupActive() const { return popupActive; }

    // ── 백라이트 / 절전 ──
    void setBrightness(uint8_t level);
    uint8_t getBrightness() const { return brightness; }
    void enterSleepMode();
    void exitSleepMode();
    bool isSleepMode() const { return sleepMode; }

    // ── 활동 시간 업데이트 (자동 로그아웃) ──
    void updateActivity();

    // ── 상태 출력 ──
    void printStatus();

private:
    // 화면 상태
    ScreenType currentScreen  = SCREEN_MAIN;
    ScreenType previousScreen = SCREEN_MAIN;
    bool       needsRedraw    = true;
    uint32_t   lastUpdate     = 0;

    // 메시지
    bool     messageActive    = false;
    char     messageText[100] = {0};
    uint32_t messageStartTime = 0;
    uint32_t messageDuration  = 0;

    // Toast
    bool     toastActive      = false;
    char     toastText[64]    = {0};
    uint16_t toastColor       = 0;
    uint32_t toastStartTime   = 0;
    uint32_t toastDuration    = 2000;

    // 팝업
    bool      popupActive        = false;
    char      popupLabel[50]     = {0};
    float     popupValue         = 0;
    float     popupMin           = 0;
    float     popupMax           = 0;
    float     popupStep          = 0;
    uint8_t   popupDecimals      = 0;
    float*    popupTargetFloat   = nullptr;
    uint32_t* popupTargetU32     = nullptr;

    // 백라이트
    uint8_t brightness       = 255;
    bool    sleepMode        = false;
    uint8_t savedBrightness  = 255;

    // 내부
    void drawCurrentScreen();
    void handlePopupTouch(uint16_t x, uint16_t y);
};

extern UIManager uiManager;
