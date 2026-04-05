// ================================================================
// UI_Screen_PID.cpp - 재설계 PID 설정 화면
// ================================================================
#include "../include/UIComponents.h"
#include "../include/Config.h"

using namespace UIComponents;
using namespace UITheme;

// 선택된 PID 파라미터
static int8_t selectedPIDParam = -1;

void drawPIDScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    // ── 헤더 ──
    drawHeader("PID 제어 설정");
    
    // ── PID 파라미터 카드들 ──
    int16_t startY = HEADER_HEIGHT + SPACING_SM;
    int16_t cardH = 70;
    
    struct PIDParam {
        const char* label;
        const char* description;
        float* value;
        float minVal;
        float maxVal;
        float step;
        uint16_t color;
    };
    
    PIDParam params[] = {
        {"Kp (비례)", "비례 게인", &config.pidKp, 0.0f, 10.0f, 0.1f, COLOR_PRIMARY},
        {"Ki (적분)", "적분 게인", &config.pidKi, 0.0f, 5.0f, 0.05f, COLOR_ACCENT},
        {"Kd (미분)", "미분 게인", &config.pidKd, 0.0f, 5.0f, 0.1f, COLOR_INFO}
    };
    
    for (int i = 0; i < 3; i++) {
        int16_t y = startY + i * (cardH + SPACING_SM);
        bool isSelected = (selectedPIDParam == i);
        
        CardConfig paramCard = {
            .x = SPACING_SM,
            .y = y,
            .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
            .h = cardH,
            .bgColor = isSelected ? COLOR_BG_ELEVATED : COLOR_BG_CARD,
            .borderColor = isSelected ? params[i].color : COLOR_BORDER
        };
        drawCard(paramCard);
        
        // 파라미터 이름
        tft.setTextSize(TEXT_SIZE_MEDIUM);
        tft.setTextColor(params[i].color);
        tft.setCursor(paramCard.x + CARD_PADDING, paramCard.y + CARD_PADDING);
        tft.print(params[i].label);
        
        // 설명
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(paramCard.x + CARD_PADDING, paramCard.y + CARD_PADDING + 20);
        tft.print(params[i].description);
        
        // 현재 값
        tft.setTextSize(3);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.setCursor(paramCard.x + CARD_PADDING, paramCard.y + CARD_PADDING + 35);
        tft.printf("%.2f", *params[i].value);
        
        // 편집 버튼
        if (!isSelected) {
            ButtonConfig editBtn = {
                .x = (int16_t)(paramCard.x + paramCard.w - 70),
                .y = (int16_t)(paramCard.y + CARD_PADDING + 20),
                .w = 60,
                .h = 28,
                .label = "편집",
                .style = BTN_SECONDARY,
                .enabled = true
            };
            drawButton(editBtn);
        }
    }
    
    // 편집 모드일 때 조절 패널
    if (selectedPIDParam >= 0) {
        int16_t panelY = startY + 3 * (cardH + SPACING_SM) + SPACING_SM;
        
        CardConfig editPanel = {
            .x = SPACING_SM,
            .y = panelY,
            .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
            .h = 55,
            .bgColor = COLOR_PRIMARY_DARK,
            .elevated = true
        };
        drawCard(editPanel);
        
        // 큰 감소 버튼
        ButtonConfig minus10Btn = {
            .x = (int16_t)(editPanel.x + SPACING_SM),
            .y = (int16_t)(panelY + 8),
            .w = 50,
            .h = 38,
            .label = "--",
            .style = BTN_DANGER,
            .enabled = true
        };
        drawButton(minus10Btn);
        
        // 작은 감소 버튼
        ButtonConfig minus1Btn = {
            .x = (int16_t)(editPanel.x + SPACING_SM + 55),
            .y = (int16_t)(panelY + 8),
            .w = 50,
            .h = 38,
            .label = "-",
            .style = BTN_SECONDARY,
            .enabled = true
        };
        drawButton(minus1Btn);
        
        // 현재 값 표시
        tft.setTextSize(TEXT_SIZE_MEDIUM);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        char valueStr[16];
        snprintf(valueStr, sizeof(valueStr), "%.2f", *params[selectedPIDParam].value);
        int16_t textW = strlen(valueStr) * 12;
        tft.setCursor(editPanel.x + (editPanel.w - textW) / 2, panelY + 20);
        tft.print(valueStr);
        
        // 작은 증가 버튼
        ButtonConfig plus1Btn = {
            .x = (int16_t)(editPanel.x + editPanel.w - 110),
            .y = (int16_t)(panelY + 8),
            .w = 50,
            .h = 38,
            .label = "+",
            .style = BTN_SECONDARY,
            .enabled = true
        };
        drawButton(plus1Btn);
        
        // 큰 증가 버튼
        ButtonConfig plus10Btn = {
            .x = (int16_t)(editPanel.x + editPanel.w - 55),
            .y = (int16_t)(panelY + 8),
            .w = 50,
            .h = 38,
            .label = "++",
            .style = BTN_SUCCESS,
            .enabled = true
        };
        drawButton(plus10Btn);
    }
    
    // 현재 PID 출력 미리보기
    if (selectedPIDParam < 0) {
        int16_t previewY = startY + 3 * (cardH + SPACING_SM) + SPACING_SM;
        
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(SPACING_SM + 4, previewY);
        tft.print("현재 PID 출력:");
        
        tft.setTextSize(TEXT_SIZE_MEDIUM);
        tft.setTextColor(COLOR_ACCENT);
        tft.setCursor(SPACING_SM + 4, previewY + 16);
        tft.printf("%.2f", pidOutput);
        
        // PID 오류값
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(SCREEN_WIDTH / 2, previewY);
        tft.print("오차:");
        
        tft.setTextSize(TEXT_SIZE_MEDIUM);
        tft.setTextColor(pidError > 0 ? COLOR_WARNING : COLOR_SUCCESS);
        tft.setCursor(SCREEN_WIDTH / 2, previewY + 16);
        tft.printf("%.1f kPa", pidError);
    }
    
    // ── 하단 네비게이션 ──
    if (selectedPIDParam >= 0) {
        NavButton navButtons[] = {
            {"취소", BTN_DANGER, true},
            {"저장", BTN_SUCCESS, true}
        };
        drawNavBar(navButtons, 2);
    } else {
        NavButton navButtons[] = {
            {"뒤로", BTN_OUTLINE, true},
            {"기본값", BTN_SECONDARY, systemController.getPermissions().canChangeSettings},
            {"Auto", BTN_PRIMARY, false} // 향후 자동 튜닝 기능
        };
        drawNavBar(navButtons, 3);
    }
}

void handlePIDTouch(uint16_t x, uint16_t y) {
    int16_t startY = HEADER_HEIGHT + SPACING_SM;
    int16_t cardH = 70;
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    
    struct PIDParam {
        float* value;
        float minVal;
        float maxVal;
        float step;
    };
    
    PIDParam params[] = {
        {&config.pidKp, 0.0f, 10.0f, 0.1f},
        {&config.pidKi, 0.0f, 5.0f, 0.05f},
        {&config.pidKd, 0.0f, 5.0f, 0.1f}
    };
    
    // 편집 모드일 때
    if (selectedPIDParam >= 0) {
        int16_t panelY = startY + 3 * (cardH + SPACING_SM) + SPACING_SM;
        
        // -- 버튼 (큰 감소)
        ButtonConfig minus10Btn = {
            .x = (int16_t)(SPACING_SM + SPACING_SM),
            .y = (int16_t)(panelY + 8),
            .w = 50,
            .h = 38,
            .label = "--",
            .style = BTN_DANGER,
            .enabled = true
        };
        
        if (isButtonPressed(minus10Btn, x, y)) {
            float newVal = *params[selectedPIDParam].value - params[selectedPIDParam].step * 10;
            if (newVal >= params[selectedPIDParam].minVal) {
                *params[selectedPIDParam].value = newVal;
            }
            screenNeedsRedraw = true;
            return;
        }
        
        // - 버튼 (작은 감소)
        ButtonConfig minus1Btn = {
            .x = (int16_t)(SPACING_SM + SPACING_SM + 55),
            .y = (int16_t)(panelY + 8),
            .w = 50,
            .h = 38,
            .label = "-",
            .style = BTN_SECONDARY,
            .enabled = true
        };
        
        if (isButtonPressed(minus1Btn, x, y)) {
            float newVal = *params[selectedPIDParam].value - params[selectedPIDParam].step;
            if (newVal >= params[selectedPIDParam].minVal) {
                *params[selectedPIDParam].value = newVal;
            }
            screenNeedsRedraw = true;
            return;
        }
        
        // + 버튼 (작은 증가)
        ButtonConfig plus1Btn = {
            .x = (int16_t)(SCREEN_WIDTH - SPACING_SM - 110),
            .y = (int16_t)(panelY + 8),
            .w = 50,
            .h = 38,
            .label = "+",
            .style = BTN_SECONDARY,
            .enabled = true
        };
        
        if (isButtonPressed(plus1Btn, x, y)) {
            float newVal = *params[selectedPIDParam].value + params[selectedPIDParam].step;
            if (newVal <= params[selectedPIDParam].maxVal) {
                *params[selectedPIDParam].value = newVal;
            }
            screenNeedsRedraw = true;
            return;
        }
        
        // ++ 버튼 (큰 증가)
        ButtonConfig plus10Btn = {
            .x = (int16_t)(SCREEN_WIDTH - SPACING_SM - 55),
            .y = (int16_t)(panelY + 8),
            .w = 50,
            .h = 38,
            .label = "++",
            .style = BTN_SUCCESS,
            .enabled = true
        };
        
        if (isButtonPressed(plus10Btn, x, y)) {
            float newVal = *params[selectedPIDParam].value + params[selectedPIDParam].step * 10;
            if (newVal <= params[selectedPIDParam].maxVal) {
                *params[selectedPIDParam].value = newVal;
            }
            screenNeedsRedraw = true;
            return;
        }
        
        // 네비게이션 (저장/취소)
        if (y >= navY) {
            int16_t buttonW = (SCREEN_WIDTH - SPACING_SM * 3) / 2;
            
            // 취소
            ButtonConfig cancelBtn = {
                .x = SPACING_SM,
                .y = (int16_t)(navY + 2),
                .w = buttonW,
                .h = (int16_t)(FOOTER_HEIGHT - 4),
                .label = "취소",
                .style = BTN_DANGER,
                .enabled = true
            };
            
            if (isButtonPressed(cancelBtn, x, y)) {
                loadConfig();
                selectedPIDParam = -1;
                screenNeedsRedraw = true;
                return;
            }
            
            // 저장
            ButtonConfig saveBtn = {
                .x = (int16_t)(SPACING_SM + buttonW + SPACING_SM),
                .y = (int16_t)(navY + 2),
                .w = buttonW,
                .h = (int16_t)(FOOTER_HEIGHT - 4),
                .label = "저장",
                .style = BTN_SUCCESS,
                .enabled = true
            };
            
            if (isButtonPressed(saveBtn, x, y)) {
                saveConfig();
                selectedPIDParam = -1;
                screenNeedsRedraw = true;
                return;
            }
        }
    }
    // 일반 모드일 때
    else {
        // 파라미터 카드의 편집 버튼 클릭
        for (int i = 0; i < 3; i++) {
            int16_t cardY = startY + i * (cardH + SPACING_SM);
            
            ButtonConfig editBtn = {
                .x = (int16_t)(SCREEN_WIDTH - SPACING_SM - 70),
                .y = (int16_t)(cardY + CARD_PADDING + 20),
                .w = 60,
                .h = 28,
                .label = "편집",
                .style = BTN_SECONDARY,
                .enabled = true
            };
            
            if (isButtonPressed(editBtn, x, y)) {
                selectedPIDParam = i;
                screenNeedsRedraw = true;
                return;
            }
        }
        
        // 네비게이션
        if (y >= navY) {
            int16_t buttonW = (SCREEN_WIDTH - SPACING_SM * 4) / 3;
            
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
            
            // 기본값
            if (systemController.getPermissions().canChangeSettings) {
                ButtonConfig defaultBtn = {
                    .x = (int16_t)(SPACING_SM + buttonW + SPACING_SM),
                    .y = (int16_t)(navY + 2),
                    .w = buttonW,
                    .h = (int16_t)(FOOTER_HEIGHT - 4),
                    .label = "기본값",
                    .style = BTN_SECONDARY,
                    .enabled = true
                };
                
                if (isButtonPressed(defaultBtn, x, y)) {
                    config.pidKp = PID_KP;
                    config.pidKi = PID_KI;
                    config.pidKd = PID_KD;
                    saveConfig();
                    screenNeedsRedraw = true;
                    return;
                }
            }
        }
    }
}