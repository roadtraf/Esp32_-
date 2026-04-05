UI_Screen_Statistics.cpp (재설계)
// ================================================================
// UI_Screen_Statistics.cpp - 재설계 통계 화면
// ================================================================
#include "../include/UIComponents.h"
#include "../include/Config.h"

// FreeRTOS (delay 개선)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

using namespace UIComponents;
using namespace UITheme;

void drawStatisticsScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    // ── 헤더 ──
    drawHeader("통계");
    
    // ── 통계 카드들 ──
    int16_t startY = HEADER_HEIGHT + SPACING_SM;
    int16_t cardW = (SCREEN_WIDTH - SPACING_SM * 3) / 2;
    int16_t cardH = 80;
    
    // 총 사이클
    CardConfig cycleCard = {
        .x = SPACING_SM,
        .y = startY,
        .w = cardW,
        .h = cardH,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(cycleCard);
    
    char cycleVal[16];
    snprintf(cycleVal, sizeof(cycleVal), "%lu", stats.totalCycles);
    
    ValueDisplayConfig cycleDisplay = {
        .x = (int16_t)(cycleCard.x + CARD_PADDING),
        .y = (int16_t)(cycleCard.y + CARD_PADDING),
        .label = "총 사이클",
        .value = cycleVal,
        .unit = "회",
        .valueColor = COLOR_PRIMARY
    };
    drawValueDisplay(cycleDisplay);
    
    // 성공률
    CardConfig successCard = {
        .x = (int16_t)(SPACING_SM + cardW + SPACING_SM),
        .y = startY,
        .w = cardW,
        .h = cardH,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(successCard);
    
    float successRate = (stats.totalCycles > 0) 
                        ? (float)stats.successfulCycles / stats.totalCycles * 100 
                        : 0;
    
    char successVal[16];
    snprintf(successVal, sizeof(successVal), "%.1f", successRate);
    
    ValueDisplayConfig successDisplay = {
        .x = (int16_t)(successCard.x + CARD_PADDING),
        .y = (int16_t)(successCard.y + CARD_PADDING),
        .label = "성공률",
        .value = successVal,
        .unit = "%",
        .valueColor = (successRate >= 95) ? COLOR_SUCCESS : COLOR_WARNING
    };
    drawValueDisplay(successDisplay);
    
    // 가동 시간
    CardConfig uptimeCard = {
        .x = SPACING_SM,
        .y = (int16_t)(startY + cardH + SPACING_SM),
        .w = cardW,
        .h = cardH,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(uptimeCard);
    
    uint32_t uptimeHours = stats.uptime / 3600;
    
    char uptimeVal[16];
    snprintf(uptimeVal, sizeof(uptimeVal), "%lu", uptimeHours);
    
    ValueDisplayConfig uptimeDisplay = {
        .x = (int16_t)(uptimeCard.x + CARD_PADDING),
        .y = (int16_t)(uptimeCard.y + CARD_PADDING),
        .label = "가동 시간",
        .value = uptimeVal,
        .unit = "시간",
        .valueColor = COLOR_ACCENT
    };
    drawValueDisplay(uptimeDisplay);
    
    // 에러 발생
    CardConfig errorCard = {
        .x = (int16_t)(SPACING_SM + cardW + SPACING_SM),
        .y = (int16_t)(startY + cardH + SPACING_SM),
        .w = cardW,
        .h = cardH,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(errorCard);
    
    char errorVal[16];
    snprintf(errorVal, sizeof(errorVal), "%u", errorHistCnt);
    
    ValueDisplayConfig errorDisplay = {
        .x = (int16_t)(errorCard.x + CARD_PADDING),
        .y = (int16_t)(errorCard.y + CARD_PADDING),
        .label = "에러 발생",
        .value = errorVal,
        .unit = "회",
        .valueColor = (errorHistCnt > 10) ? COLOR_DANGER : COLOR_INFO
    };
    drawValueDisplay(errorDisplay);
    
    // 센서 통계 카드 (Phase 2-3)
    CardConfig sensorCard = {
        .x = SPACING_SM,
        .y = (int16_t)(startY + (cardH + SPACING_SM) * 2),
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = 90,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(sensorCard);
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(sensorCard.x + CARD_PADDING, sensorCard.y + CARD_PADDING);
    tft.print("센서 평균값 (최근 1분)");
    
    SensorStats sensorStats;
    calculateSensorStats(sensorStats);
    
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    int16_t lineY = sensorCard.y + CARD_PADDING + 20;
    
    tft.setCursor(sensorCard.x + CARD_PADDING, lineY);
    tft.printf("온도: %.1f°C", sensorStats.avgTemperature);
    
    tft.setCursor(sensorCard.x + CARD_PADDING + 150, lineY);
    tft.printf("압력: %.1f kPa", sensorStats.avgPressure);
    
    tft.setCursor(sensorCard.x + CARD_PADDING, lineY + 20);
    tft.printf("전류: %.2f A", sensorStats.avgCurrent);
    
    tft.setCursor(sensorCard.x + CARD_PADDING + 150, lineY + 20);
    tft.printf("샘플: %lu개", sensorStats.sampleCount);
    
    // ── 하단 네비게이션 ──
    NavButton navButtons[] = {
        {"뒤로", BTN_OUTLINE, true},
        {"초기화", BTN_DANGER, systemController.getPermissions().canChangeSettings}
    };
    drawNavBar(navButtons, 2);
}

void handleStatisticsTouch(uint16_t x, uint16_t y) {
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;

 // 통계 초기화 팝업이 활성화되어 있으면 우선 처리
if (isResetConfirmPending()) {
    if (handleResetConfirmTouch(x, y)) return;
}
   
 if (y >= navY) {
        int16_t buttonW = (SCREEN_WIDTH - SPACING_SM * 3) / 2;
        
        // 뒤로 버튼
        ButtonConfig backBtn = {
            .x = SPACING_SM,
            .y = (int16_t)(navY + 2),
            .w = buttonW,
            .h = (int16_t)(FOOTER_HEIGHT - 4),
            .label = "뒤로",
            .style = BTN_OUTLINE,
            .enabled = true
        };
        
        if (isButtonPressed(backBtn, x, y)) {
            currentScreen = SCREEN_SETTINGS;
            screenNeedsRedraw = true;
            return;
        }
        
        // 초기화 버튼 (관리자 전용)
        if (systemController.getPermissions().canChangeSettings) {
            ButtonConfig resetBtn = {
                .x = (int16_t)(SPACING_SM + buttonW + SPACING_SM),
                .y = (int16_t)(navY + 2),
                .w = buttonW,
                .h = (int16_t)(FOOTER_HEIGHT - 4),
                .label = "초기화",
                .style = BTN_DANGER,
                .enabled = true
            };
            
            if (isButtonPressed(resetBtn, x, y)) {
                // 통계 초기화 확인 팝업
                showResetConfirmation();
                return;
            }
        }
    }
}

void showResetConfirmation() {
    int16_t popupW = 280;
    int16_t popupH = 140;
    int16_t popupX = (SCREEN_WIDTH - popupW) / 2;
    int16_t popupY = (SCREEN_HEIGHT - popupH) / 2;
    
    tft.fillScreen(0x18E3);
    
    CardConfig popup = {
        .x = popupX,
        .y = popupY,
        .w = popupW,
        .h = popupH,
        .bgColor = COLOR_BG_CARD,
        .borderColor = COLOR_DANGER
    };
    drawCard(popup);
    
    drawIconWarning(popupX + popupW / 2 - 8, popupY + 15, COLOR_DANGER);
    
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(popupX + 80, popupY + 45);
    tft.print("통계 초기화");
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(popupX + 40, popupY + 70);
    tft.print("모든 통계를 초기화합니다");
    
    tft.setCursor(popupX + 70, popupY + 85);
    tft.print("계속하시겠습니까?");
    
    // 버튼
    ButtonConfig cancelBtn = {
        .x = (int16_t)(popupX + 20),
        .y = (int16_t)(popupY + popupH - 35),
        .w = 110,
        .h = 28,
        .label = "취소",
        .style = BTN_OUTLINE,
        .enabled = true
    };
    drawButton(cancelBtn);
    
    ButtonConfig confirmBtn = {
        .x = (int16_t)(popupX + popupW - 130),
        .y = (int16_t)(popupY + popupH - 35),
        .w = 110,
        .h = 28,
        .label = "초기화",
        .style = BTN_DANGER,
        .enabled = true
    };
    drawButton(confirmBtn);
    
    // [R4] 비블로킹: 초기화 확인은 터치 이벤트로 처리
    // 실제 초기화는 handleStatisticsTouch에서 버튼 터치 감지 필요
    uiManager.showMessage("버튼을 선택하세요", 2000);
}