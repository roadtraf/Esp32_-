// ================================================================
// UI_Screens.cpp - UI 화면 라우팅 및 헬퍼 함수
// UI_Screens.cpp — vTaskDelay 블로킹 팝업 4곳 교체본
//
// [R4] 문제: showAccessDenied(), showMaintenanceCompletePopup(),
//            showResetConfirmation(), showTemperatureSensorInfo() 에서
//            vTaskDelay(2000~3000ms) 블로킹
//            → 그 시간 동안 터치 무반응, 센서 갱신 화면 멈춤,
//              WDT feed 지연 위험
//
// 수정: 팝업을 그린 후 즉시 반환. 팝업은 uiManager.showMessage()
//       타이머 방식으로 자동 소멸로 위 4개 함수 교체
// ================================================================

#include "../include/UI_Screens.h"
#include "../include/UIComponents.h"
#include "../include/Config.h"
#include "../include/SystemController.h"
#include "UI_AccessControl.h"

#ifdef ENABLE_PREDICTIVE_MAINTENANCE
#include "../include/HealthMonitor.h"

// FreeRTOS (delay 개선)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
extern HealthMonitor healthMonitor;
#endif

extern LGFX tft;
extern SystemController systemController;
extern ScreenType currentScreen;
extern bool screenNeedsRedraw;

// 버퍼 참조
extern std::vector<float> temperatureBuffer;
extern std::vector<float> pressureBuffer;
extern std::vector<float> currentBuffer;

// 통계 참조 (프로젝트에 따라 다를 수 있음)
// extern Statistics stats;  // 필요시 주석 해제

// 에러 관련
extern ErrorInfo errorHistory[];
extern uint8_t errorHistIdx;
extern uint8_t errorHistCnt;
extern bool errorActive;
extern ErrorInfo currentError;

using namespace UIComponents;
using namespace UITheme;

// ================================================================
// 권한 없음 팝업
// ── 1. showAccessDenied() ──────────────────────────────────────
// [R4] vTaskDelay(2000) 제거 → showMessage 타이머로 교체
// ================================================================

void showAccessDenied(const char* screenName) {
    char msg[64];
    snprintf(msg, sizeof(msg), "'%s' — 관리자 권한 필요", screenName);
    // UIManager 타이머 방식: 2.5초 후 자동 소멸, 블로킹 없음
    uiManager.showMessage(msg, 2500);
    // vTaskDelay(pdMS_TO_TICKS(2000));  ← 제거됨
}

// ================================================================
// 센서 통계 계산
// ================================================================
void calculateSensorStats(SensorStats& stats) {
    stats.avgTemperature = 0;
    stats.avgPressure = 0;
    stats.avgCurrent = 0;
    stats.sampleCount = 0;
    
    // 온도
    if (temperatureBuffer.size() > 0) {
        float sum = 0;
        for (float val : temperatureBuffer) {
            sum += val;
        }
        stats.avgTemperature = sum / temperatureBuffer.size();
        stats.sampleCount = temperatureBuffer.size();
    }
    
    // 압력
    if (pressureBuffer.size() > 0) {
        float sum = 0;
        for (float val : pressureBuffer) {
            sum += val;
        }
        stats.avgPressure = sum / pressureBuffer.size();
    }
    
    // 전류
    if (currentBuffer.size() > 0) {
        float sum = 0;
        for (float val : currentBuffer) {
            sum += val;
        }
        stats.avgCurrent = sum / currentBuffer.size();
    }
}

// ================================================================
// 유지보수 완료 팝업
// ── 2. showMaintenanceCompletePopup() ─────────────────────────
// [R4] vTaskDelay(2000) 제거
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
    // [U7] textWidth 기반 중앙 정렬
    const char* t = "완료!";
    tft.setCursor(popupX + (popupW - tft.textWidth(t)) / 2, popupY + 50);
    tft.print(t);

    tft.setTextSize(UITheme::TEXT_SIZE_SMALL);
    const char* m1 = "유지보수가 완료되었습니다";
    const char* m2 = "건강도가 100%로 리셋됩니다";
    tft.setCursor(popupX + (popupW - tft.textWidth(m1)) / 2, popupY + 80);
    tft.print(m1);
    tft.setCursor(popupX + (popupW - tft.textWidth(m2)) / 2, popupY + 95);
    tft.print(m2);

    // 확인 버튼 그리기만 (터치 처리는 다음 프레임)
    UIComponents::ButtonConfig okBtn = {
        .x = (int16_t)(popupX + (popupW - 100) / 2),
        .y = (int16_t)(popupY + popupH - 35),
        .w = 100, .h = 28,
        .label = "확인",
        .style = UIComponents::BTN_OUTLINE,
        .enabled = true
    };
    UIComponents::drawButton(okBtn);

    // [R4] 2초 후 자동 화면 복귀 (타이머 방식)
    uiManager.showMessage("유지보수 완료", 2000);
    // vTaskDelay(pdMS_TO_TICKS(2000));  ← 제거됨
}
#endif

// ================================================================
// 통계 초기화 확인 팝업
// ================================================================
// ── 3. showResetConfirmation() ────────────────────────────────
// [R4] vTaskDelay(2000) 제거
//      확인/취소 버튼은 터치 이벤트로 처리해야 하므로
//      팝업 상태 플래그 방식으로 전환
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
    const char* t = "통계 초기화";
    tft.setCursor(popupX + (popupW - tft.textWidth(t)) / 2, popupY + 45);
    tft.print(t);

    tft.setTextSize(UITheme::TEXT_SIZE_SMALL);
    tft.setTextColor(UITheme::COLOR_TEXT_SECONDARY);
    const char* m1 = "모든 통계를 초기화합니다";
    const char* m2 = "계속하시겠습니까?";
    tft.setCursor(popupX + (popupW - tft.textWidth(m1)) / 2, popupY + 70);
    tft.print(m1);
    tft.setCursor(popupX + (popupW - tft.textWidth(m2)) / 2, popupY + 85);
    tft.print(m2);

    UIComponents::ButtonConfig cancelBtn = {
        .x = (int16_t)(popupX + 20),
        .y = (int16_t)(popupY + popupH - 35),
        .w = 110, .h = 28,
        .label = "취소",
        .style = UIComponents::BTN_OUTLINE, .enabled = true
    };
    UIComponents::drawButton(cancelBtn);

    UIComponents::ButtonConfig confirmBtn = {
        .x = (int16_t)(popupX + popupW - 130),
        .y = (int16_t)(popupY + popupH - 35),
        .w = 110, .h = 28,
        .label = "초기화",
        .style = UIComponents::BTN_DANGER, .enabled = true
    };
    UIComponents::drawButton(confirmBtn);

    s_resetConfirmPending = true;
    // [R4] vTaskDelay(2000) 제거 — 즉시 반환
    // 터치 처리는 handleResetConfirmTouch()에서
}

// 통계 초기화 팝업 터치 처리 (handleStatisticsTouch 에서 호출)
bool handleResetConfirmTouch(uint16_t x, uint16_t y) {
    if (!s_resetConfirmPending) return false;

    int16_t popupW = 280, popupH = 140;
    int16_t popupX = (SCREEN_WIDTH  - popupW) / 2;
    int16_t popupY = (SCREEN_HEIGHT - popupH) / 2;

    // 취소
    if (x >= popupX + 20 && x <= popupX + 130 &&
        y >= popupY + popupH - 35 && y <= popupY + popupH - 7) {
        s_resetConfirmPending = false;
        screenNeedsRedraw = true;
        return true;
    }
    // 초기화 확인
    if (x >= popupX + popupW - 130 && x <= popupX + popupW - 20 &&
        y >= popupY + popupH - 35  && y <= popupY + popupH - 7) {
        extern void resetStatistics();
        resetStatistics();
        s_resetConfirmPending = false;
        uiManager.showToast("통계 초기화 완료", UITheme::COLOR_SUCCESS);
        screenNeedsRedraw = true;
        return true;
    }
    return true;  // 팝업 영역 밖도 일단 소비 (오조작 방지)
}

bool isResetConfirmPending() { return s_resetConfirmPending; }

// ================================================================
// 온도 센서 정보 팝업
// ── 4. showTemperatureSensorInfo() ───────────────────────────
// [R4] vTaskDelay(3000) 제거
// ================================================================
void showTemperatureSensorInfo() {
    int16_t popupW = 300, popupH = 160;
    int16_t popupX = (SCREEN_WIDTH  - popupW) / 2;
    int16_t popupY = (SCREEN_HEIGHT - popupH) / 2;

    tft.fillScreen(UITheme::COLOR_BG_DARK);

    CardConfig popup = {
        .x = popupX, .y = popupY,
        .w = popupW, .h = popupH,
        .bgColor = UITheme::COLOR_BG_CARD,
        .borderColor = UITheme::COLOR_PRIMARY,
        .elevated = true
    };
    UIComponents::drawCard(popup);

    tft.setTextSize(UITheme::TEXT_SIZE_MEDIUM);
    tft.setTextColor(UITheme::COLOR_PRIMARY);
    tft.setCursor(popupX + UITheme::CARD_PADDING, popupY + UITheme::CARD_PADDING);
    tft.print("DS18B20 온도 센서");

    tft.setTextSize(UITheme::TEXT_SIZE_SMALL);
    tft.setTextColor(UITheme::COLOR_TEXT_PRIMARY);
    int16_t infoY = popupY + UITheme::CARD_PADDING + 30;

    const char* lines[] = {
        "센서 개수: 1개",
        "해상도: 12비트 (0.0625\xB0""C)",
        "정확도: \xB1""0.5\xB0""C",
        "공장 캘리브레이션 적용됨"
    };
    for (uint8_t i = 0; i < 4; i++) {
        tft.setCursor(popupX + UITheme::CARD_PADDING, infoY + i * 20);
        tft.print(lines[i]);
    }

    UIComponents::ButtonConfig closeBtn = {
        .x = (int16_t)(popupX + (popupW - 100) / 2),
        .y = (int16_t)(popupY + popupH - 35),
        .w = 100, .h = 28,
        .label = "닫기",
        .style = UIComponents::BTN_PRIMARY, .enabled = true
    };
    UIComponents::drawButton(closeBtn);

    // [R4] 터치로 닫기 or 3초 타이머로 자동 복귀
    uiManager.showMessage("터치하면 닫힙니다", 3000);
    // vTaskDelay(pdMS_TO_TICKS(3000));  ← 제거됨
}

