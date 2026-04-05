// ================================================================
// UI_Screen_EStop.cpp - 비상정지 전용 화면  [U5]
// E-Stop 발동 즉시 모든 화면 위에 오버레이
// ================================================================
#include <driver/gpio.h>        // gpio_get_level()
#include "../include/UIComponents.h"
#include "../include/UITheme.h"
#include "../include/UIManager.h"
#include "../include/Config.h"
#include "../include/SystemController.h"
#include "../include/SensorManager.h"

using namespace UIComponents;
using namespace UITheme;

extern LGFX             tft;
extern UIManager        uiManager;
extern SystemController systemController;
extern SensorManager    sensorManager;
extern bool             errorActive;
extern ErrorInfo        currentError;

// ================================================================
// 레이아웃 상수
// ================================================================
namespace EStopLayout {
    // 상단 경보 배너
    constexpr int16_t BANNER_H     = 64;

    // 원인/상태 카드
    constexpr int16_t CAUSE_CARD_Y = BANNER_H + SPACING_SM;
    constexpr int16_t CAUSE_CARD_H = 90;

    // 조치 카드
    constexpr int16_t ACTION_CARD_Y = CAUSE_CARD_Y + CAUSE_CARD_H + SPACING_SM;
    constexpr int16_t ACTION_CARD_H = 88;

    // 이전 경보 줄
    constexpr int16_t HIST_Y       = ACTION_CARD_Y + ACTION_CARD_H + SPACING_SM;

    // 해제 버튼
    constexpr int16_t RELEASE_BTN_Y = SCREEN_HEIGHT - FOOTER_HEIGHT - 4;
    constexpr int16_t RELEASE_BTN_W = 200;
    constexpr int16_t RELEASE_BTN_H = 44;
}

// ================================================================
// 경보 발생 시각 (발동 시 기록)
// ================================================================
static uint32_t g_estopStartMs   = 0;
static ScreenType g_prevScreen   = SCREEN_MAIN;  // E-Stop 이전 화면 기억

void recordEStopStart(ScreenType prevScreen) {
    g_estopStartMs = millis();
    g_prevScreen   = prevScreen;
}

// ================================================================
// 비상정지 화면 그리기
// ================================================================
void drawEStopScreen() {
    tft.fillScreen(COLOR_BG_DARK);

    // ── 상단 경보 배너 (전체 너비, 빨간 배경) ──
    tft.fillRect(0, 0, SCREEN_WIDTH, EStopLayout::BANNER_H, COLOR_DANGER);

    // 깜빡임 효과: 홀수 초면 밝기 조절 (LovyanGFX setAdjacentColor 활용)
    uint32_t elapsed = (millis() - g_estopStartMs) / 1000;
    bool blink = (elapsed % 2 == 0);

    tft.setTextSize(3);
    tft.setTextColor(blink ? TFT_WHITE : 0xF800);
    const char* title = "⚠  비상정지 발생  ⚠";
    int16_t tx = (SCREEN_WIDTH - tft.textWidth(title)) / 2;
    tft.setCursor(tx > 0 ? tx : 4, 10);
    tft.print("비상정지 발생");

    // 경과 시간
    uint32_t secs = (millis() - g_estopStartMs) / 1000;
    char timeStr[24];
    if (secs < 60) {
        snprintf(timeStr, sizeof(timeStr), "경과: %lu초", secs);
    } else {
        snprintf(timeStr, sizeof(timeStr), "경과: %lu분 %lu초",
                 secs / 60, secs % 60);
    }
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(TFT_WHITE);
    int16_t tmx = SCREEN_WIDTH - SPACING_SM - tft.textWidth(timeStr);
    tft.setCursor(tmx, EStopLayout::BANNER_H - 16);
    tft.print(timeStr);

    // ── 원인/상태 카드 ──
    {
        CardConfig card = {
            .x = SPACING_SM,
            .y = EStopLayout::CAUSE_CARD_Y,
            .w = SCREEN_WIDTH - SPACING_SM * 2,
            .h = EStopLayout::CAUSE_CARD_H,
            .bgColor = COLOR_BG_CARD,
            .borderColor = COLOR_DANGER
        };
        drawCard(card);

        drawIconWarning(card.x + CARD_PADDING, card.y + 16, COLOR_DANGER);

        // 원인
        tft.setTextSize(TEXT_SIZE_MEDIUM);
        tft.setTextColor(COLOR_DANGER);
        tft.setCursor(card.x + CARD_PADDING + 28, card.y + CARD_PADDING);
        tft.print("원인: ");
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        if (errorActive && strlen(currentError.message) > 0) {
            tft.print(currentError.message);
        } else {
            tft.print("비상정지 버튼 조작");
        }

        // 현재 센서값
        float p = sensorManager.getPressure();
        float t = sensorManager.getTemperature();
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(card.x + CARD_PADDING + 28, card.y + CARD_PADDING + 24);
        char sensorStr[48];
        snprintf(sensorStr, sizeof(sensorStr),
                 "압력: %.1f kPa  |  온도: %.1f°C", p, t);
        tft.print(sensorStr);

        // 발생 시각
        tft.setCursor(card.x + CARD_PADDING + 28, card.y + CARD_PADDING + 42);
        char tsStr[32];
        snprintf(tsStr, sizeof(tsStr), "발생: %lu분 전",
                 (millis() - g_estopStartMs) / 60000);
        tft.print(tsStr);

        // 자동 조치 상태
        tft.setTextColor(COLOR_SUCCESS);
        tft.setCursor(card.x + CARD_PADDING + 28, card.y + CARD_PADDING + 60);
        tft.print("✓ 펌프 자동 정지  ✓ 밸브 전체 닫힘");
    }

    // ── 조치 방법 카드 ──
    {
        CardConfig card = {
            .x = SPACING_SM,
            .y = EStopLayout::ACTION_CARD_Y,
            .w = SCREEN_WIDTH - SPACING_SM * 2,
            .h = EStopLayout::ACTION_CARD_H,
            .bgColor = COLOR_BG_CARD,
            .borderColor = COLOR_WARNING
        };
        drawCard(card);

        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_WARNING);
        tft.setCursor(card.x + CARD_PADDING, card.y + CARD_PADDING);
        tft.print("▶ 복구 절차");

        tft.setTextColor(COLOR_TEXT_PRIMARY);
        const char* steps[] = {
            "1. 비상정지 원인 현장 확인",
            "2. 원인 제거 후 비상정지 버튼 복귀",
            "3. 아래 [해제] 버튼 터치"
        };
        for (uint8_t i = 0; i < 3; i++) {
            tft.setCursor(card.x + CARD_PADDING + 8,
                          card.y + CARD_PADDING + 18 + i * 20);
            tft.print(steps[i]);
        }
    }

    // ── 이전 경보 한 줄 요약 ──
    {
        extern ErrorInfo errorHistory[];
        extern uint8_t   errorHistCnt;
        extern uint8_t   errorHistIdx;

        if (errorHistCnt > 1) {
            uint8_t prevIdx = (errorHistIdx - 2 + ERROR_HIST_MAX) % ERROR_HIST_MAX;
            tft.setTextSize(1);
            tft.setTextColor(COLOR_TEXT_DISABLED);
            tft.setCursor(SPACING_SM, EStopLayout::HIST_Y);

            uint32_t ago = (millis() - errorHistory[prevIdx].timestamp) / 1000;
            char histBuf[64];
            snprintf(histBuf, sizeof(histBuf),
                     "이전: %s  (%lu초 전)",
                     errorHistory[prevIdx].message, ago);
            tft.print(histBuf);
        }
    }

    // ── 해제 버튼 (버튼이 눌려있는 동안은 비활성) ──
    bool btnPinReleased = (gpio_get_level((gpio_num_t)PIN_ESTOP) == 1);

    int16_t btnX = (SCREEN_WIDTH - EStopLayout::RELEASE_BTN_W) / 2;
    ButtonConfig releaseBtn = {
        .x = btnX,
        .y = EStopLayout::RELEASE_BTN_Y,
        .w = EStopLayout::RELEASE_BTN_W,
        .h = EStopLayout::RELEASE_BTN_H,
        .label = btnPinReleased ? "비상정지 해제" : "버튼 복귀 대기중...",
        .style = btnPinReleased ? BTN_SUCCESS : BTN_OUTLINE,
        .enabled = btnPinReleased
    };
    drawButton(releaseBtn);

    // 비상정지 중임을 명확히 (하단 상태 표시)
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_DANGER);
    const char* statusTxt = "■ 시스템 운전 중지됨";
    int16_t sx = (SCREEN_WIDTH - tft.textWidth(statusTxt)) / 2;
    tft.setCursor(sx, SCREEN_HEIGHT - 16);
    tft.print(statusTxt);
}

// ================================================================
// 비상정지 화면 터치 처리
// ================================================================
void handleEStopTouch(uint16_t x, uint16_t y) {
    uiManager.updateActivity();

    bool btnPinReleased = (gpio_get_level((gpio_num_t)PIN_ESTOP) == 1);
    if (!btnPinReleased) {
        uiManager.showToast("버튼을 먼저 복귀하세요", COLOR_WARNING);
        return;
    }

    int16_t btnX = (SCREEN_WIDTH - EStopLayout::RELEASE_BTN_W) / 2;
    ButtonConfig releaseBtn = {
        .x = btnX,
        .y = EStopLayout::RELEASE_BTN_Y,
        .w = EStopLayout::RELEASE_BTN_W,
        .h = EStopLayout::RELEASE_BTN_H,
        .label = "",
        .style = BTN_SUCCESS,
        .enabled = true
    };

    if (isButtonPressed(releaseBtn, x, y)) {
        // 비상정지 해제 명령 큐에 전송
        extern QueueHandle_t g_cmdQueue;
        SystemCommand cmd;
        cmd.type = CommandType::RELEASE_ESTOP;
        strncpy(cmd.origin, "UI_ESTOP", sizeof(cmd.origin) - 1);
        xQueueSend(g_cmdQueue, &cmd, 0);

        uiManager.showToast("비상정지 해제됨", COLOR_SUCCESS);
        // 이전 화면으로 복귀
        uiManager.setScreen(g_prevScreen);
    }
}
