// ================================================================
// UI_Screen_SmartAlertConfig.cpp - 재설계 스마트 알림 설정
// ================================================================
#include "../include/UIComponents.h"
#include "../include/Config.h"

#ifdef ENABLE_SMART_ALERTS
#include "../include/SmartAlert.h"
extern SmartAlert smartAlert;
#endif

using namespace UIComponents;
using namespace UITheme;

void drawSmartAlertConfigScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    // ── 헤더 ──
    drawHeader("스마트 알림 설정");
    
    // 권한 확인
    if (!canAccessScreen(SCREEN_SMART_ALERT_CONFIG)) {
        showAccessDenied("스마트 알림");
        NavButton navButtons[] = {{"뒤로", BTN_OUTLINE, true}};
        drawNavBar(navButtons, 1);
        return;
    }
    
    #ifdef ENABLE_SMART_ALERTS
    
    // ── 알림 활성화 상태 ──
    int16_t startY = HEADER_HEIGHT + SPACING_SM;
    
    CardConfig statusCard = {
        .x = SPACING_SM,
        .y = startY,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = 55,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(statusCard);
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(statusCard.x + CARD_PADDING, statusCard.y + CARD_PADDING);
    tft.print("스마트 알림");
    
    bool alertEnabled = smartAlert.isEnabled();
    
    drawBadge(statusCard.x + statusCard.w - 70, statusCard.y + CARD_PADDING,
              alertEnabled ? "활성" : "비활성",
              alertEnabled ? BADGE_SUCCESS : BADGE_DANGER);
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(statusCard.x + CARD_PADDING, statusCard.y + CARD_PADDING + 20);
    tft.print("AI 기반 예측 알림 시스템");
    
    // 토글 버튼
    ButtonConfig toggleBtn = {
        .x = (int16_t)(statusCard.x + statusCard.w - 70),
        .y = (int16_t)(statusCard.y + statusCard.h - 32),
        .w = 60,
        .h = 24,
        .label = alertEnabled ? "OFF" : "ON",
        .style = alertEnabled ? BTN_DANGER : BTN_SUCCESS,
        .enabled = true
    };
    drawButton(toggleBtn);
    
    // ── 알림 유형 설정 ──
    int16_t typeY = statusCard.y + statusCard.h + SPACING_SM;
    
    struct AlertType {
        const char* name;
        const char* description;
        bool enabled;
        uint16_t color;
    };
    
    AlertType types[] = {
        {"유지보수", "예측 유지보수 알림", true, COLOR_WARNING},
        {"온도", "온도 임계값 알림", true, COLOR_DANGER},
        {"전류", "과전류 알림", true, COLOR_WARNING},
        {"압력", "압력 이상 알림", false, COLOR_INFO}
    };
    
    int16_t typeH = 48;
    
    for (int i = 0; i < 4; i++) {
        int16_t y = typeY + i * (typeH + 4);
        
        CardConfig typeCard = {
            .x = SPACING_SM,
            .y = y,
            .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
            .h = typeH,
            .bgColor = types[i].enabled ? COLOR_BG_ELEVATED : COLOR_BG_CARD,
            .borderColor = types[i].enabled ? types[i].color : COLOR_BORDER
        };
        drawCard(typeCard);
        
        // 체크박스 아이콘
        int16_t checkX = typeCard.x + CARD_PADDING;
        int16_t checkY = typeCard.y + (typeH - 16) / 2;
        
        if (types[i].enabled) {
            tft.fillRoundRect(checkX, checkY, 16, 16, 4, types[i].color);
            drawIconCheck(checkX + 2, checkY + 2, COLOR_BG_DARK);
        } else {
            tft.drawRoundRect(checkX, checkY, 16, 16, 4, COLOR_BORDER);
        }
        
        // 알림 이름
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.setCursor(checkX + 24, typeCard.y + CARD_PADDING);
        tft.print(types[i].name);
        
        // 설명
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(checkX + 24, typeCard.y + CARD_PADDING + 18);
        tft.print(types[i].description);
    }
    
    #else
    // 기능 비활성화 메시지
    int16_t msgY = SCREEN_HEIGHT / 2 - 30;
    
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(80, msgY);
    tft.print("기능 비활성화됨");
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setCursor(60, msgY + 25);
    tft.print("Config.h에서 활성화하세요");
    #endif
    
    // ── 하단 네비게이션 ──
    NavButton navButtons[] = {
        {"뒤로", BTN_OUTLINE, true},
        #ifdef ENABLE_SMART_ALERTS
        {"테스트", BTN_PRIMARY, true}
        #endif
    };
    
    #ifdef ENABLE_SMART_ALERTS
    drawNavBar(navButtons, 2);
    #else
    drawNavBar(navButtons, 1);
    #endif
}

void handleSmartAlertConfigTouch(uint16_t x, uint16_t y) {
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    
    #ifdef ENABLE_SMART_ALERTS
    
    // 토글 버튼
    int16_t startY = HEADER_HEIGHT + SPACING_SM;
    bool alertEnabled = smartAlert.isEnabled();
    
    ButtonConfig toggleBtn = {
        .x = (int16_t)(SCREEN_WIDTH - SPACING_SM - 70),
        .y = (int16_t)(startY + 55 - 32),
        .w = 60,
        .h = 24,
        .label = alertEnabled ? "OFF" : "ON",
        .style = alertEnabled ? BTN_DANGER : BTN_SUCCESS,
        .enabled = true
    };
    
    if (isButtonPressed(toggleBtn, x, y)) {
        if (alertEnabled) {
            smartAlert.disable();
        } else {
            smartAlert.enable();
        }
        screenNeedsRedraw = true;
        return;
    }
    
    // 알림 유형 선택
    int16_t typeY = startY + 55 + SPACING_SM;
    int16_t typeH = 48;
    
    for (int i = 0; i < 4; i++) {
        int16_t cardY = typeY + i * (typeH + 4);
        
        if (x >= SPACING_SM && x <= SCREEN_WIDTH - SPACING_SM &&
            y >= cardY && y <= cardY + typeH) {
            // 알림 유형 토글 (실제 구현 필요)
            Serial.printf("[Alert] 알림 유형 %d 토글\n", i);
            screenNeedsRedraw = true;
            return;
        }
    }
    
    // 네비게이션
    if (y >= navY) {
        int16_t buttonW = (SCREEN_WIDTH - SPACING_SM * 3) / 2;
        
        // 뒤로
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
        
        // 테스트
        ButtonConfig testBtn = {
            .x = (int16_t)(SPACING_SM + buttonW + SPACING_SM),
            .y = (int16_t)(navY + 2),
            .w = buttonW,
            .h = (int16_t)(FOOTER_HEIGHT - 4),
            .label = "테스트",
            .style = BTN_PRIMARY,
            .enabled = true
        };
        
        if (isButtonPressed(testBtn, x, y)) {
            smartAlert.testAlert();
            return;
        }
    }
    
    #else
    
    // 기능 비활성화 시 뒤로만
    if (y >= navY) {
        currentScreen = SCREEN_SETTINGS;
        screenNeedsRedraw = true;
    }
    
    #endif
}