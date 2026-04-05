// ================================================================
// UI_Screen_Calibration.cpp - 재설계 캘리브레이션 화면
// ================================================================
#include "../include/UIComponents.h"
#include "../include/Config.h"
#include "../include/SystemController.h"

// FreeRTOS (delay 개선)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

using namespace UIComponents;
using namespace UITheme;

// ================================================================
// 캘리브레이션 화면
// ================================================================
void drawCalibrationScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    // ── 헤더 ──
    drawHeader("캘리브레이션");
    
    // 권한 확인
    if (!systemController.getPermissions().canCalibrate) {
        // 권한 없음 안내
        int16_t msgY = SCREEN_HEIGHT / 2 - 40;
        
        drawIconWarning(SCREEN_WIDTH / 2 - 8, msgY, COLOR_WARNING);
        
        tft.setTextSize(TEXT_SIZE_MEDIUM);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.setCursor(120, msgY + 30);
        tft.print("권한 필요");
        
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(80, msgY + 55);
        tft.print("관리자 권한이 필요합니다");
        
        // 뒤로 버튼만
        NavButton navButtons[] = {
            {"뒤로", BTN_OUTLINE, true}
        };
        drawNavBar(navButtons, 1);
        return;
    }
    
    // ── 센서 카드들 ──
    int16_t startY = HEADER_HEIGHT + SPACING_MD;
    int16_t cardH = 70;
    
    // 압력 센서
    CardConfig pressureCard = {
        .x = SPACING_SM,
        .y = startY,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = cardH,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(pressureCard);
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(pressureCard.x + CARD_PADDING, pressureCard.y + CARD_PADDING);
    tft.print("1. 압력 센서");
    
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(pressureCard.x + CARD_PADDING, pressureCard.y + CARD_PADDING + 18);
    tft.print("현재 오프셋:");
    
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(COLOR_PRIMARY);
    tft.setCursor(pressureCard.x + CARD_PADDING + 90, pressureCard.y + CARD_PADDING + 15);
    tft.printf("%.2f kPa", pressureOffset);
    
    // 캘리브레이션 버튼
    ButtonConfig pressureBtn = {
        .x = (int16_t)(pressureCard.x + pressureCard.w - 90),
        .y = (int16_t)(pressureCard.y + CARD_PADDING + 10),
        .w = 80,
        .h = 28,
        .label = "조정",
        .style = BTN_PRIMARY,
        .enabled = true
    };
    drawButton(pressureBtn);
    
    // 전류 센서
    CardConfig currentCard = {
        .x = SPACING_SM,
        .y = (int16_t)(startY + cardH + SPACING_SM),
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = cardH,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(currentCard);
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(currentCard.x + CARD_PADDING, currentCard.y + CARD_PADDING);
    tft.print("2. 전류 센서");
    
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(currentCard.x + CARD_PADDING, currentCard.y + CARD_PADDING + 18);
    tft.print("현재 오프셋:");
    
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(COLOR_ACCENT);
    tft.setCursor(currentCard.x + CARD_PADDING + 90, currentCard.y + CARD_PADDING + 15);
    tft.printf("%.3f A", currentOffset);
    
    ButtonConfig currentBtn = {
        .x = (int16_t)(currentCard.x + currentCard.w - 90),
        .y = (int16_t)(currentCard.y + CARD_PADDING + 10),
        .w = 80,
        .h = 28,
        .label = "조정",
        .style = BTN_PRIMARY,
        .enabled = true
    };
    drawButton(currentBtn);
    
    // 온도 센서
    CardConfig tempCard = {
        .x = SPACING_SM,
        .y = (int16_t)(startY + (cardH + SPACING_SM) * 2),
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = cardH,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(tempCard);
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(tempCard.x + CARD_PADDING, tempCard.y + CARD_PADDING);
    tft.print("3. 온도 센서 (DS18B20)");
    
    if (isTemperatureSensorConnected()) {
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(tempCard.x + CARD_PADDING, tempCard.y + CARD_PADDING + 18);
        tft.print("공장 캘리브레이션 사용");
        
        ButtonConfig tempBtn = {
            .x = (int16_t)(tempCard.x + tempCard.w - 90),
            .y = (int16_t)(tempCard.y + CARD_PADDING + 10),
            .w = 80,
            .h = 28,
            .label = "정보",
            .style = BTN_SECONDARY,
            .enabled = true
        };
        drawButton(tempBtn);
    } else {
        tft.setTextColor(COLOR_DANGER);
        tft.setCursor(tempCard.x + CARD_PADDING, tempCard.y + CARD_PADDING + 18);
        tft.print("센서 연결 안 됨");
    }
    
    // 안내 메시지
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(SPACING_SM, startY + (cardH + SPACING_SM) * 3 + 10);
    tft.print("센서 조정 시 부하를 제거하세요");
    
    // ── 하단 네비게이션 ──
    NavButton navButtons[] = {
        {"뒤로", BTN_OUTLINE, true}
    };
    drawNavBar(navButtons, 1);
}

// ================================================================
// 캘리브레이션 터치 처리
// ================================================================
void handleCalibrationTouch(uint16_t x, uint16_t y) {
    // 권한 확인
    if (!systemController.getPermissions().canCalibrate) {
        int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
        
        if (y >= navY) {
            currentScreen = SCREEN_SETTINGS;
            screenNeedsRedraw = true;
        }
        return;
    }
    
    // ── 하단 네비게이션 ──
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    
    if (y >= navY) {
        ButtonConfig backBtn = {
            .x = SPACING_SM,
            .y = (int16_t)(navY + 2),
            .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
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
    }
    
    // ── 캘리브레이션 버튼들 ──
    int16_t startY = HEADER_HEIGHT + SPACING_MD;
    int16_t cardH = 70;
    
    // 압력 센서 버튼
    ButtonConfig pressureBtn = {
        .x = (int16_t)(SCREEN_WIDTH - SPACING_SM - 90),
        .y = (int16_t)(startY + CARD_PADDING + 10),
        .w = 80,
        .h = 28,
        .label = "조정",
        .style = BTN_PRIMARY,
        .enabled = true
    };
    
    if (isButtonPressed(pressureBtn, x, y)) {
        calibratePressure();
        screenNeedsRedraw = true;
        return;
    }
    
    // 전류 센서 버튼
    ButtonConfig currentBtn = {
        .x = (int16_t)(SCREEN_WIDTH - SPACING_SM - 90),
        .y = (int16_t)(startY + (cardH + SPACING_SM) + CARD_PADDING + 10),
        .w = 80,
        .h = 28,
        .label = "조정",
        .style = BTN_PRIMARY,
        .enabled = true
    };
    
    if (isButtonPressed(currentBtn, x, y)) {
        calibrateCurrent();
        screenNeedsRedraw = true;
        return;
    }
    
    // 온도 센서 정보 버튼
    if (isTemperatureSensorConnected()) {
        ButtonConfig tempBtn = {
            .x = (int16_t)(SCREEN_WIDTH - SPACING_SM - 90),
            .y = (int16_t)(startY + (cardH + SPACING_SM) * 2 + CARD_PADDING + 10),
            .w = 80,
            .h = 28,
            .label = "정보",
            .style = BTN_SECONDARY,
            .enabled = true
        };
        
        if (isButtonPressed(tempBtn, x, y)) {
            // 온도 센서 정보 팝업
            showTemperatureSensorInfo();
            return;
        }
    }
}

// ================================================================
// 온도 센서 정보 팝업
// ================================================================
void showTemperatureSensorInfo() {
    int16_t popupW = 300;
    int16_t popupH = 160;
    int16_t popupX = (SCREEN_WIDTH - popupW) / 2;
    int16_t popupY = (SCREEN_HEIGHT - popupH) / 2;
    
    // 배경
    tft.fillScreen(0x18E3);
    
    CardConfig popup = {
        .x = popupX,
        .y = popupY,
        .w = popupW,
        .h = popupH,
        .bgColor = COLOR_BG_CARD,
        .borderColor = COLOR_PRIMARY,
        .elevated = true
    };
    drawCard(popup);
    
    // 제목
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(COLOR_PRIMARY);
    tft.setCursor(popupX + CARD_PADDING, popupY + CARD_PADDING);
    tft.print("DS18B20 온도 센서");
    
    // 정보
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    
    int16_t infoY = popupY + CARD_PADDING + 30;
    tft.setCursor(popupX + CARD_PADDING, infoY);
    tft.printf("센서 개수: %d개", getTemperatureSensorCount());
    
    tft.setCursor(popupX + CARD_PADDING, infoY + 20);
    tft.print("해상도: 12비트 (0.0625°C)");
    
    tft.setCursor(popupX + CARD_PADDING, infoY + 40);
    tft.print("정확도: ±0.5°C");
    
    tft.setCursor(popupX + CARD_PADDING, infoY + 60);
    tft.print("공장 캘리브레이션 적용됨");
    
    // 닫기 버튼
    ButtonConfig closeBtn = {
        .x = (int16_t)(popupX + (popupW - 100) / 2),
        .y = (int16_t)(popupY + popupH - 35),
        .w = 100,
        .h = 28,
        .label = "닫기",
        .style = BTN_PRIMARY,
        .enabled = true
    };
    drawButton(closeBtn);
    
    // [R4] 비블로킹: 3초 후 자동으로 원래 화면 복귀
    uiManager.showMessage("터치하면 닫힙니다", 3000);
}