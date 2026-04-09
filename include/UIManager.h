// ================================================================
// UIManager.h - UI  
// [U3] showToast() , showMessage()  
// [U5] E-Stop  + drawEStopScreen()  
// [U8] screenNeedsRedraw/currentScreen Mutex 
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
    void drawCurrentScreen();
    //   /  
    void begin();
    void update();

    //    [U8] 
    void setScreen(ScreenType screen);       // Mutex  
    ScreenType getCurrentScreen() const { return currentScreen; }
    ScreenType getPreviousScreen() const { return previousScreen; }
    void redrawScreen();
    void requestRedraw();

    //    
    void handleTouch();

    //   ( , ) [U3] 
    void showMessage(const char* message, uint32_t duration = 2000);
    void hideMessage();
    bool isMessageActive() const { return messageActive; }

    //  Toast  [U3] 
    void showToast(const char* message, uint16_t color);
    void drawToastOverlay();
    bool isToastActive() const { return toastActive; }

    //   
    void showPopup(const char* label, float* target, float min, float max,
                   float step, uint8_t decimals);
    void showPopupU32(const char* label, uint32_t* target, uint32_t min,
                      uint32_t max, uint32_t step);
    void hidePopup();
    bool isPopupActive() const { return popupActive; }

    //   /  
    void setBrightness(uint8_t level);
    uint8_t getBrightness() const { return brightness; }
    void enterSleepMode();
    void exitSleepMode();
    bool isSleepMode() const { return sleepMode; }

    //     ( ) 
    void updateActivity();

    //    
    void printStatus();

private:
    //  
    ScreenType currentScreen  = SCREEN_MAIN;
    ScreenType previousScreen = SCREEN_MAIN;
    bool       needsRedraw    = true;
    uint32_t   lastUpdate     = 0;

    // 
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

    // 
    bool      popupActive        = false;
    char      popupLabel[50]     = {0};
    float     popupValue         = 0;
    float     popupMin           = 0;
    float     popupMax           = 0;
    float     popupStep          = 0;
    uint8_t   popupDecimals      = 0;
    float*    popupTargetFloat   = nullptr;
    uint32_t* popupTargetU32     = nullptr;

    // 
    uint8_t brightness       = 255;
    bool    sleepMode        = false;
    uint8_t savedBrightness  = 255;

    // 
    void handlePopupTouch(uint16_t x, uint16_t y);
};

extern UIManager uiManager;
