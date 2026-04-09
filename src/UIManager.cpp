// ================================================================
// UIManager.cpp - UI  
// [U3] vTaskDelay     
// [U5] E-Stop      
// [U8] screenNeedsRedraw / currentScreen Race  
//       SemaphoreHandle 
// ================================================================
#include "UIManager.h"
#include "UI_Screens.h"
#include "UI_AccessControl.h"

//  
UIManager uiManager;

extern TFT_GFX tft;
// 

// ================================================================
// [U8]   Mutex 
// ================================================================
static SemaphoreHandle_t g_screenMutex = nullptr;

// ================================================================
// 
// ================================================================
void UIManager::begin() {
    Serial.println("[UIMgr]  ...");

    // [U8] Mutex 
    g_screenMutex = xSemaphoreCreateMutex();
    configASSERT(g_screenMutex);

    currentScreen  = SCREEN_MAIN;
    previousScreen = SCREEN_MAIN;
    needsRedraw    = true;
    lastUpdate     = 0;

    messageActive   = false;
    messageDuration = 0;
    popupActive     = false;
    popupTargetFloat = nullptr;
    popupTargetU32   = nullptr;
    brightness       = 255;
    sleepMode        = false;
    savedBrightness  = 255;

    // Toast 
    toastActive    = false;
    toastStartTime = 0;

    Serial.println("[UIMgr]   ");
}

// ================================================================
//  
// ================================================================
void UIManager::update() {
    uint32_t now = millis();

    // [U5] E-Stop       
    extern bool g_estopActive;  // SharedState or main_hardened
    if (g_estopActive && currentScreen != SCREEN_ESTOP) {
        extern void recordEStopStart(ScreenType);
        recordEStopStart(currentScreen);
        setScreen(SCREEN_ESTOP);
    }

    //   (150ms)
    if (now - lastUpdate >= 150) {
        lastUpdate = now;

        if (needsRedraw) {
            drawCurrentScreen();
            needsRedraw = false;
        }

        // E-Stop      
        if (currentScreen == SCREEN_ESTOP) {
            drawCurrentScreen();
        }
    }

    // [U3] /Toast   ( )
    if (messageActive && (now - messageStartTime >= messageDuration)) {
        hideMessage();
    }
    if (toastActive && (now - toastStartTime >= toastDuration)) {
        toastActive = false;
        needsRedraw = true;
    }

    // PIN    
    if (isPinScreenActive()) {
        drawPinInputScreen();
    }

    //   
    systemController.checkAutoLogout();
    
    // updatePopupLongPress();  //  
}

// ================================================================
//    [U8] Mutex 
// ================================================================
void UIManager::setScreen(ScreenType screen) {
    if (xSemaphoreTake(g_screenMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (currentScreen != screen) {
            previousScreen = currentScreen;
            currentScreen  = screen;
            needsRedraw    = true;
            Serial.printf("[UIMgr] : %d  %d\n",
                          (int)previousScreen, (int)currentScreen);
        }
        xSemaphoreGive(g_screenMutex);
    }
}

void UIManager::redrawScreen() {
    needsRedraw = true;
}

void UIManager::requestRedraw() {
    needsRedraw = true;
}

// ================================================================
//    (E-Stop )
// ================================================================
void UIManager::drawCurrentScreen() {
    // PIN   PIN 
    if (isPinScreenActive()) {
        drawPinInputScreen();
        return;
    }

    switch (currentScreen) {
        case SCREEN_MAIN:            drawMainScreen();           break;
        case SCREEN_SETTINGS:        drawSettingsScreen();       break;
        case SCREEN_ALARM:           drawAlarmScreen();          break;
        case SCREEN_TREND_GRAPH:     // drawGraphScreen();  break;  // 
            break;
        case SCREEN_TIMING_SETUP:    drawTimingScreen();         break;
        case SCREEN_PID_SETUP:       drawPIDScreen();            break;
        case SCREEN_STATISTICS:      drawStatisticsScreen();     break;
        case SCREEN_CALIBRATION:     drawCalibrationScreen();    break;
        case SCREEN_ABOUT:           drawAboutScreen();          break;
        case SCREEN_HELP:            drawHelpScreen();           break;
        case SCREEN_STATE_DIAGRAM:   drawStateDiagramScreen();   break;
        case SCREEN_WATCHDOG_STATUS: drawWatchdogStatusScreen(); break;
        // [U5] E-Stop  
        case SCREEN_ESTOP:           drawEStopScreen();          break;
#ifdef ENABLE_PREDICTIVE_MAINTENANCE
        case SCREEN_HEALTH:          drawHealthScreen();         break;
        case SCREEN_HEALTH_TREND:    drawHealthTrendScreen();    break;
#endif
#ifdef ENABLE_SMART_ALERTS
        case SCREEN_SMART_ALERT_CONFIG: drawSmartAlertConfigScreen(); break;
#endif
#ifdef ENABLE_VOICE_ALERTS
        case SCREEN_VOICE_SETTINGS:  drawVoiceSettingsScreen();  break;
#endif
#ifdef ENABLE_ADVANCED_ANALYSIS
        case SCREEN_ADVANCED_ANALYSIS: drawAdvancedAnalysisScreen(); break;
#endif
        default:
            Serial.printf("[UIMgr]     : %d\n", (int)currentScreen);
            break;
    }

    // Toast  (   )
    if (toastActive) {
        drawToastOverlay();
    }
}

// ================================================================
//   
// ================================================================
void UIManager::handleTouch() {
    // PIN  
    if (isPinScreenActive()) {
        uint16_t x = 0, y = 0;
        tft.getTouch(&x, &y);
        if (x != 0 || y != 0) handlePinInputTouch(x, y);
        return;
    }

    //  
    if (popupActive) {
        uint16_t x = 0, y = 0;
        tft.getTouch(&x, &y);
        if (x != 0 || y != 0) handlePopupTouch(x, y);
        return;
    }

    //  
    extern void handleTouchByScreen();
    handleTouchByScreen();
}

// ================================================================
// [U3]   ( ,  )
// ================================================================
void UIManager::showMessage(const char* message, uint32_t duration) {
    strncpy(messageText, message, sizeof(messageText) - 1);
    messageText[sizeof(messageText) - 1] = '\0';

    messageActive    = true;
    messageStartTime = millis();
    messageDuration  = duration;

    //      
    int16_t my = SCREEN_HEIGHT - 40;
    tft.fillRect(0, my, SCREEN_WIDTH, 40, UITheme::COLOR_INFO);
    tft.setTextSize(UITheme::TEXT_SIZE_SMALL);
    tft.setTextColor(UITheme::COLOR_TEXT_PRIMARY);
    tft.setCursor(UITheme::SPACING_SM, my + 12);
    tft.print(message);
}

void UIManager::hideMessage() {
    if (!messageActive) return;
    messageActive = false;
    needsRedraw   = true;
}

// ================================================================
// Toast  (  )
// ================================================================
void UIManager::showToast(const char* message, uint16_t color) {
    strncpy(toastText, message, sizeof(toastText) - 1);
    toastText[sizeof(toastText) - 1] = '\0';
    toastColor     = color;
    toastActive    = true;
    toastStartTime = millis();
    toastDuration  = 2000;
    drawToastOverlay();
}

void UIManager::drawToastOverlay() {
    tft.setTextSize(UITheme::TEXT_SIZE_SMALL);
    int16_t tw = tft.textWidth(toastText);
    int16_t pw = tw + UITheme::SPACING_LG;
    int16_t px = (SCREEN_WIDTH - pw) / 2;
    int16_t py = UITheme::HEADER_HEIGHT + UITheme::SPACING_MD;

    tft.fillRoundRect(px, py, pw, 28, UITheme::BUTTON_RADIUS, toastColor);
    tft.setTextColor(UITheme::COLOR_TEXT_PRIMARY);
    tft.setCursor(px + UITheme::SPACING_MD, py + 8);
    tft.print(toastText);
}

// ================================================================
//   ( )
// ================================================================
void UIManager::showPopup(const char* label, float* target, float min,
                          float max, float step, uint8_t decimals) {
    strncpy(popupLabel, label, sizeof(popupLabel) - 1);
    popupLabel[sizeof(popupLabel) - 1] = '\0';
    popupValue       = *target;
    popupMin         = min;
    popupMax         = max;
    popupStep        = step;
    popupDecimals    = decimals;
    popupTargetFloat = target;
    popupTargetU32   = nullptr;
    popupActive      = true;
    extern void drawPopup();
    drawPopup();
}

void UIManager::showPopupU32(const char* label, uint32_t* target,
                             uint32_t min, uint32_t max, uint32_t step) {
    strncpy(popupLabel, label, sizeof(popupLabel) - 1);
    popupLabel[sizeof(popupLabel) - 1] = '\0';
    popupValue       = (float)*target;
    popupMin         = (float)min;
    popupMax         = (float)max;
    popupStep        = (float)step;
    popupDecimals    = 0;
    popupTargetFloat = nullptr;
    popupTargetU32   = target;
    popupActive      = true;
    extern void drawPopup();
    drawPopup();
}

void UIManager::hidePopup() {
    popupActive = false;
    needsRedraw = true;
}

// ================================================================
// 
// ================================================================
void UIManager::setBrightness(uint8_t level) {
    brightness = constrain(level, 0, 255);
    tft.setBrightness(brightness);  // LovyanGFX   
}

// ================================================================
// 
// ================================================================
void UIManager::enterSleepMode() {
    if (sleepMode) return;
    sleepMode       = true;
    savedBrightness = brightness;
    setBrightness(0);
    Serial.println("[UIMgr]  ");
}

void UIManager::exitSleepMode() {
    if (!sleepMode) return;
    sleepMode = false;
    setBrightness(savedBrightness);
    needsRedraw = true;
    Serial.println("[UIMgr]  ");
}

// ================================================================
//   
// ================================================================
void UIManager::updateActivity() {
    systemController.updateActivity();
    if (sleepMode) exitSleepMode();
}

// ================================================================
//  
// ================================================================
void UIManager::printStatus() {
    Serial.printf(
        "[UIMgr] =%d, =%s, =%s, Toast=%s, Popup=%s, =%d\n",
        (int)currentScreen,
        needsRedraw   ? "Y" : "N",
        messageActive ? "Y" : "N",
        toastActive   ? "Y" : "N",
        popupActive   ? "Y" : "N",
        brightness);
}
