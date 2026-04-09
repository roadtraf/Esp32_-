// ================================================================
// UI_Screens.cpp - UI     
// UI_Screens.cpp  vTaskDelay   4 
//
// [R4] : showAccessDenied(), showMaintenanceCompletePopup(),
//            showResetConfirmation(), showTemperatureSensorInfo() 
//            vTaskDelay(2000~3000ms) 
//                 ,    ,
//              WDT feed  
//
// :     .  uiManager.showMessage()
//            4  
// ================================================================

#include "UI_Screens.h"
#include "UIComponents.h"
#include "Config.h"
#include "SystemController.h"
#include "UI_AccessControl.h"
#include "UIManager.h"
#include "SensorBuffer.h"

#ifdef ENABLE_PREDICTIVE_MAINTENANCE
#include "HealthMonitor.h"

// FreeRTOS (delay )
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
extern HealthMonitor healthMonitor;
#endif

extern TFT_GFX tft;
extern SystemController systemController;
extern ScreenType currentScreen;
extern bool screenNeedsRedraw;

//  
// extern std::vector<float> temperatureBuffer;
// extern std::vector<float> pressureBuffer;
// extern std::vector<float> currentBuffer;

//   (    )
// extern Statistics stats;  //   

//  
extern ErrorInfo errorHistory[];
extern uint8_t errorHistIdx;
extern uint8_t errorHistCnt;
extern bool errorActive;
extern ErrorInfo currentError;

using namespace UIComponents;
using namespace UITheme;

//  2. showMaintenanceCompletePopup() 
// [R4] vTaskDelay(2000) 
// ================================================================
#ifdef ENABLE_PREDICTIVE_MAINTENANCE
void showMaintenanceCompletePopup() {
    int16_t popupW = 280, popupH = 140;
    int16_t popupX = (SCREEN_WIDTH  - popupW) / 2;
    int16_t popupY = (SCREEN_HEIGHT - popupH) / 2;

    tft.fillScreen(UITheme::COLOR_BG_DARK);

    CardConfig popup = {
        .x = popupX, .y = popupY,
        .w = popupW, .h = popupH,
        .bgColor = UITheme::COLOR_SUCCESS,
        .borderColor = UITheme::COLOR_TEXT_PRIMARY,
        .elevated = true
    };
    UIComponents::drawCard(popup);

    UIComponents::drawIconCheck(popupX + popupW / 2 - 8,
                                 popupY + 20,
                                 UITheme::COLOR_TEXT_PRIMARY);

    tft.setTextSize(UITheme::TEXT_SIZE_MEDIUM);
    tft.setTextColor(UITheme::COLOR_TEXT_PRIMARY);
    // [U7] textWidth   
    const char* t = "!";
    tft.setCursor(popupX + (popupW - tft.textWidth(t)) / 2, popupY + 50);
    tft.print(t);

    tft.setTextSize(UITheme::TEXT_SIZE_SMALL);
    const char* m1 = " ";
    const char* m2 = " 100% ";
    tft.setCursor(popupX + (popupW - tft.textWidth(m1)) / 2, popupY + 80);
    tft.print(m1);
    tft.setCursor(popupX + (popupW - tft.textWidth(m2)) / 2, popupY + 95);
    tft.print(m2);

    //    (   )
    UIComponents::ButtonConfig okBtn = {
        .x = (int16_t)(popupX + (popupW - 100) / 2),
        .y = (int16_t)(popupY + popupH - 35),
        .w = 100, .h = 28,
        .label = "",
        .style = UIComponents::BTN_OUTLINE,
        .enabled = true
    };
    UIComponents::drawButton(okBtn);

    // [R4] 2     ( )
    uiManager.showMessage(" ", 2000);
    // vTaskDelay(pdMS_TO_TICKS(2000));   
}
#endif

// ================================================================
//    
// ================================================================
//  3. showResetConfirmation() 
// [R4] vTaskDelay(2000) 
//      /     
//          
static bool s_resetConfirmPending = false;

void showResetConfirmation() {
    int16_t popupW = 280, popupH = 140;
    int16_t popupX = (SCREEN_WIDTH  - popupW) / 2;
    int16_t popupY = (SCREEN_HEIGHT - popupH) / 2;

    tft.fillScreen(UITheme::COLOR_BG_DARK);

    CardConfig popup = {
        .x = popupX, .y = popupY,
        .w = popupW, .h = popupH,
        .bgColor = UITheme::COLOR_BG_CARD,
        .borderColor = UITheme::COLOR_DANGER
    };
    UIComponents::drawCard(popup);

    UIComponents::drawIconWarning(popupX + popupW / 2 - 8,
                                   popupY + 15,
                                   UITheme::COLOR_DANGER);

    tft.setTextSize(UITheme::TEXT_SIZE_MEDIUM);
    tft.setTextColor(UITheme::COLOR_TEXT_PRIMARY);
    const char* t = " ";
    tft.setCursor(popupX + (popupW - tft.textWidth(t)) / 2, popupY + 45);
    tft.print(t);

    tft.setTextSize(UITheme::TEXT_SIZE_SMALL);
    tft.setTextColor(UITheme::COLOR_TEXT_SECONDARY);
    const char* m1 = "  ";
    const char* m2 = "?";
    tft.setCursor(popupX + (popupW - tft.textWidth(m1)) / 2, popupY + 70);
    tft.print(m1);
    tft.setCursor(popupX + (popupW - tft.textWidth(m2)) / 2, popupY + 85);
    tft.print(m2);

    UIComponents::ButtonConfig cancelBtn = {
        .x = (int16_t)(popupX + 20),
        .y = (int16_t)(popupY + popupH - 35),
        .w = 110, .h = 28,
        .label = "",
        .style = UIComponents::BTN_OUTLINE, .enabled = true
    };
    UIComponents::drawButton(cancelBtn);

    UIComponents::ButtonConfig confirmBtn = {
        .x = (int16_t)(popupX + popupW - 130),
        .y = (int16_t)(popupY + popupH - 35),
        .w = 110, .h = 28,
        .label = "",
        .style = UIComponents::BTN_DANGER, .enabled = true
    };
    UIComponents::drawButton(confirmBtn);

    s_resetConfirmPending = true;
    // [R4] vTaskDelay(2000)    
    //   handleResetConfirmTouch()
}

//      (handleStatisticsTouch  )
bool handleResetConfirmTouch(uint16_t x, uint16_t y) {
    if (!s_resetConfirmPending) return false;

    int16_t popupW = 280, popupH = 140;
    int16_t popupX = (SCREEN_WIDTH  - popupW) / 2;
    int16_t popupY = (SCREEN_HEIGHT - popupH) / 2;

    // 
    if (x >= popupX + 20 && x <= popupX + 130 &&
        y >= popupY + popupH - 35 && y <= popupY + popupH - 7) {
        s_resetConfirmPending = false;
        screenNeedsRedraw = true;
        return true;
    }
    //  
    if (x >= popupX + popupW - 130 && x <= popupX + popupW - 20 &&
        y >= popupY + popupH - 35  && y <= popupY + popupH - 7) {
        extern void resetStatistics();
        resetStatistics();
        s_resetConfirmPending = false;
        uiManager.showToast("  ", UITheme::COLOR_SUCCESS);
        screenNeedsRedraw = true;
        return true;
    }
    return true;  //      ( )
}

bool isResetConfirmPending() { return s_resetConfirmPending; }
