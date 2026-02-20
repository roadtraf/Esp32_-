// ================================================================
// UIManager.cpp - UI 관리자 개선판
// [U3] vTaskDelay → 타이머 기반 비동기 메시지
// [U5] E-Stop 발생 시 즉시 전용 화면 전환
// [U8] screenNeedsRedraw / currentScreen Race 조건 수정
//      → SemaphoreHandle 보호
// ================================================================
#include "UIManager.h"
#include "../include/UI_Screens.h"
#include "../include/UI_AccessControl.h"

// 전역 인스턴스
UIManager uiManager;

extern LGFX tft;
extern LGFX_Sprite ts;

// ================================================================
// [U8] 화면 상태 Mutex 보호
// ================================================================
static SemaphoreHandle_t g_screenMutex = nullptr;

// ================================================================
// 초기화
// ================================================================
void UIManager::begin() {
    Serial.println("[UIMgr] 초기화 시작...");

    // [U8] Mutex 생성
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

    // Toast 초기화
    toastActive    = false;
    toastStartTime = 0;

    Serial.println("[UIMgr] ✅ 초기화 완료");
}

// ================================================================
// 업데이트 루프
// ================================================================
void UIManager::update() {
    uint32_t now = millis();

    // [U5] E-Stop 이벤트 감지 → 즉시 전용 화면 전환
    extern bool g_estopActive;  // SharedState or main_hardened
    if (g_estopActive && currentScreen != SCREEN_ESTOP) {
        extern void recordEStopStart(ScreenType);
        recordEStopStart(currentScreen);
        setScreen(SCREEN_ESTOP);
    }

    // 화면 업데이트 (150ms마다)
    if (now - lastUpdate >= 150) {
        lastUpdate = now;

        if (needsRedraw) {
            drawCurrentScreen();
            needsRedraw = false;
        }

        // E-Stop 화면은 깜빡임 때문에 강제 주기 갱신
        if (currentScreen == SCREEN_ESTOP) {
            drawCurrentScreen();
        }
    }

    // [U3] 메시지/Toast 자동 소멸 (블로킹 없음)
    if (messageActive && (now - messageStartTime >= messageDuration)) {
        hideMessage();
    }
    if (toastActive && (now - toastStartTime >= toastDuration)) {
        toastActive = false;
        needsRedraw = true;
    }

    // PIN 화면 잠금 타이머 갱신
    if (isPinScreenActive()) {
        drawPinInputScreen();
    }

    // 자동 로그아웃 체크
    systemController.checkAutoLogout();
    
    updatePopupLongPress();   // ← [U9] 
}

// ================================================================
// 화면 전환  [U8] Mutex 보호
// ================================================================
void UIManager::setScreen(ScreenType screen) {
    if (xSemaphoreTake(g_screenMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (currentScreen != screen) {
            previousScreen = currentScreen;
            currentScreen  = screen;
            needsRedraw    = true;
            Serial.printf("[UIMgr] 화면: %d → %d\n",
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
// 화면 그리기 디스패치 (E-Stop 추가)
// ================================================================
void UIManager::drawCurrentScreen() {
    // PIN 화면이 활성이면 PIN만 그림
    if (isPinScreenActive()) {
        drawPinInputScreen();
        return;
    }

    switch (currentScreen) {
        case SCREEN_MAIN:            drawMainScreen();           break;
        case SCREEN_SETTINGS:        drawSettingsScreen();       break;
        case SCREEN_ALARM:           drawAlarmScreen();          break;
        case SCREEN_TREND_GRAPH:     drawGraphScreen();          break;
        case SCREEN_TIMING_SETUP:    drawTimingScreen();         break;
        case SCREEN_PID_SETUP:       drawPIDScreen();            break;
        case SCREEN_STATISTICS:      drawStatisticsScreen();     break;
        case SCREEN_CALIBRATION:     drawCalibrationScreen();    break;
        case SCREEN_ABOUT:           drawAboutScreen();          break;
        case SCREEN_HELP:            drawHelpScreen();           break;
        case SCREEN_STATE_DIAGRAM:   drawStateDiagramScreen();   break;
        case SCREEN_WATCHDOG_STATUS: drawWatchdogStatusScreen(); break;
        // [U5] E-Stop 전용 화면
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
            Serial.printf("[UIMgr] ⚠ 알 수 없는 화면: %d\n", (int)currentScreen);
            break;
    }

    // Toast 오버레이 (화면 위에 항상 표시)
    if (toastActive) {
        drawToastOverlay();
    }
}

// ================================================================
// 터치 처리 디스패치
// ================================================================
void UIManager::handleTouch() {
    // PIN 화면 최우선
    if (isPinScreenActive()) {
        uint16_t x = 0, y = 0;
        tft.getTouch(&x, &y);
        if (x != 0 || y != 0) handlePinInputTouch(x, y);
        return;
    }

    // 팝업 우선
    if (popupActive) {
        uint16_t x = 0, y = 0;
        tft.getTouch(&x, &y);
        if (x != 0 || y != 0) handlePopupTouch(x, y);
        return;
    }

    // 화면별 터치
    extern void handleTouchByScreen();
    handleTouchByScreen();
}

// ================================================================
// [U3] 메시지 표시 (타이머 기반, 블로킹 없음)
// ================================================================
void UIManager::showMessage(const char* message, uint32_t duration) {
    strncpy(messageText, message, sizeof(messageText) - 1);
    messageText[sizeof(messageText) - 1] = '\0';

    messageActive    = true;
    messageStartTime = millis();
    messageDuration  = duration;

    // 화면 하단 메시지 바에 즉시 표시
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
// Toast 오버레이 (화면 중앙 상단)
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
    int16_t px = (UITheme::SCREEN_WIDTH - pw) / 2;
    int16_t py = UITheme::HEADER_HEIGHT + UITheme::SPACING_MD;

    tft.fillRoundRect(px, py, pw, 28, UITheme::BUTTON_RADIUS, toastColor);
    tft.setTextColor(UITheme::COLOR_TEXT_PRIMARY);
    tft.setCursor(px + UITheme::SPACING_MD, py + 8);
    tft.print(toastText);
}

// ================================================================
// 팝업 관리 (기존 유지)
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
// 백라이트
// ================================================================
void UIManager::setBrightness(uint8_t level) {
    brightness = constrain(level, 0, 255);
    tft.setBrightness(brightness);  // LovyanGFX 내장 함수 사용
}

// ================================================================
// 절전
// ================================================================
void UIManager::enterSleepMode() {
    if (sleepMode) return;
    sleepMode       = true;
    savedBrightness = brightness;
    setBrightness(0);
    Serial.println("[UIMgr] 절전 진입");
}

void UIManager::exitSleepMode() {
    if (!sleepMode) return;
    sleepMode = false;
    setBrightness(savedBrightness);
    needsRedraw = true;
    Serial.println("[UIMgr] 절전 해제");
}

// ================================================================
// 활동 시간 업데이트
// ================================================================
void UIManager::updateActivity() {
    systemController.updateActivity();
    if (sleepMode) exitSleepMode();
}

// ================================================================
// 상태 출력
// ================================================================
void UIManager::printStatus() {
    Serial.printf(
        "[UIMgr] 화면=%d, 재그리기=%s, 메시지=%s, Toast=%s, Popup=%s, 밝기=%d\n",
        (int)currentScreen,
        needsRedraw   ? "Y" : "N",
        messageActive ? "Y" : "N",
        toastActive   ? "Y" : "N",
        popupActive   ? "Y" : "N",
        brightness);
}
