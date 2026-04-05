// ================================================================
// UI_Screen_Timing.cpp - 재설계 타이밍 설정 화면
// ================================================================
#include "../include/UIComponents.h"
#include "../include/Config.h"

using namespace UIComponents;
using namespace UITheme;

// 선택된 항목 (편집용)
static int8_t selectedTimingItem = -1;

void drawTimingScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    // ── 헤더 ──
    drawHeader("타이밍 설정");
    
    // ── 설정 항목들 ──
    int16_t startY = HEADER_HEIGHT + SPACING_SM;
    int16_t itemH = 48;
    
    struct TimingItem {
        const char* label;
        const char* description;
        uint32_t* value;
        uint32_t minVal;
        uint32_t maxVal;
        uint32_t step;
        const char* unit;
    };
    
    TimingItem items[] = {
        {"진공 ON", "펌프 작동 시간", &config.vacuumOnTime, 100, 5000, 100, "ms"},
        {"진공 유지", "진공 유지 시간", &config.vacuumHoldTime, 1000, 30000, 500, "ms"},
        {"유지 연장", "1회 연장 시간", &config.vacuumHoldExtension, 500, 10000, 500, "ms"},
        {"진공 해제", "펌프 정지 시간", &config.vacuumBreakTime, 100, 5000, 100, "ms"},
        {"제거 대기", "제거 대기 시간", &config.waitRemovalTime, 5000, 60000, 1000, "ms"}
    };
    
    const int itemCount = 5;
    
    for (int i = 0; i < itemCount; i++) {
        int16_t y = startY + i * (itemH + 4);
        
        // 카드
        bool isSelected = (selectedTimingItem == i);
        
        CardConfig itemCard = {
            .x = SPACING_SM,
            .y = y,
            .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
            .h = itemH,
            .bgColor = isSelected ? COLOR_BG_ELEVATED : COLOR_BG_CARD,
            .borderColor = isSelected ? COLOR_PRIMARY : COLOR_BORDER
        };
        drawCard(itemCard);
        
        // 라벨
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.setCursor(itemCard.x + CARD_PADDING, itemCard.y + CARD_PADDING);
        tft.print(items[i].label);
        
        // 설명
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(itemCard.x + CARD_PADDING, itemCard.y + CARD_PADDING + 14);
        tft.print(items[i].description);
        
        // 값
        tft.setTextSize(TEXT_SIZE_MEDIUM);
        tft.setTextColor(COLOR_PRIMARY);
        tft.setCursor(itemCard.x + itemCard.w - 120, itemCard.y + CARD_PADDING + 5);
        tft.printf("%lu %s", *items[i].value, items[i].unit);
        
        // 편집 버튼
        if (!isSelected) {
            tft.setTextSize(1);
            tft.setTextColor(COLOR_ACCENT);
            tft.setCursor(itemCard.x + itemCard.w - 40, itemCard.y + itemCard.h - 16);
            tft.print("편집 >");
        }
    }
    
    // 편집 모드일 때 조절 버튼 표시
    if (selectedTimingItem >= 0 && selectedTimingItem < itemCount) {
        int16_t editY = startY + itemCount * (itemH + 4) + SPACING_SM;
        
        CardConfig editCard = {
            .x = SPACING_SM,
            .y = editY,
            .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
            .h = 50,
            .bgColor = COLOR_PRIMARY_DARK
        };
        drawCard(editCard);
        
        // - 버튼
        ButtonConfig minusBtn = {
            .x = (int16_t)(editCard.x + CARD_PADDING),
            .y = (int16_t)(editCard.y + 9),
            .w = 60,
            .h = 32,
            .label = "-",
            .style = BTN_SECONDARY,
            .enabled = true
        };
        drawButton(minusBtn);
        
        // 현재 값
        tft.setTextSize(TEXT_SIZE_MEDIUM);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        char valueStr[32];
        snprintf(valueStr, sizeof(valueStr), "%lu %s", 
                 *items[selectedTimingItem].value, 
                 items[selectedTimingItem].unit);
        int16_t textW = strlen(valueStr) * 12;
        tft.setCursor(editCard.x + (editCard.w - textW) / 2, editCard.y + 17);
        tft.print(valueStr);
        
        // + 버튼
        ButtonConfig plusBtn = {
            .x = (int16_t)(editCard.x + editCard.w - 70),
            .y = (int16_t)(editCard.y + 9),
            .w = 60,
            .h = 32,
            .label = "+",
            .style = BTN_SECONDARY,
            .enabled = true
        };
        drawButton(plusBtn);
    }
    
    // ── 하단 네비게이션 ──
    if (selectedTimingItem >= 0) {
        NavButton navButtons[] = {
            {"취소", BTN_DANGER, true},
            {"저장", BTN_SUCCESS, true}
        };
        drawNavBar(navButtons, 2);
    } else {
        NavButton navButtons[] = {
            {"뒤로", BTN_OUTLINE, true},
            {"기본값", BTN_SECONDARY, systemController.getPermissions().canChangeSettings}
        };
        drawNavBar(navButtons, 2);
    }
}

void handleTimingTouch(uint16_t x, uint16_t y) {
    int16_t startY = HEADER_HEIGHT + SPACING_SM;
    int16_t itemH = 48;
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    
    struct TimingItem {
        uint32_t* value;
        uint32_t minVal;
        uint32_t maxVal;
        uint32_t step;
    };
    
    TimingItem items[] = {
        {&config.vacuumOnTime, 100, 5000, 100},
        {&config.vacuumHoldTime, 1000, 30000, 500},
        {&config.vacuumHoldExtension, 500, 10000, 500},
        {&config.vacuumBreakTime, 100, 5000, 100},
        {&config.waitRemovalTime, 5000, 60000, 1000}
    };
    
    // 편집 모드일 때
    if (selectedTimingItem >= 0) {
        int16_t editY = startY + 5 * (itemH + 4) + SPACING_SM;
        
        // - 버튼
        ButtonConfig minusBtn = {
            .x = (int16_t)(SPACING_SM + CARD_PADDING),
            .y = (int16_t)(editY + 9),
            .w = 60,
            .h = 32,
            .label = "-",
            .style = BTN_SECONDARY,
            .enabled = true
        };
        
        if (isButtonPressed(minusBtn, x, y)) {
            if (*items[selectedTimingItem].value > items[selectedTimingItem].minVal) {
                *items[selectedTimingItem].value -= items[selectedTimingItem].step;
                screenNeedsRedraw = true;
            }
            return;
        }
        
        // + 버튼
        ButtonConfig plusBtn = {
            .x = (int16_t)(SCREEN_WIDTH - SPACING_SM - CARD_PADDING - 60),
            .y = (int16_t)(editY + 9),
            .w = 60,
            .h = 32,
            .label = "+",
            .style = BTN_SECONDARY,
            .enabled = true
        };
        
        if (isButtonPressed(plusBtn, x, y)) {
            if (*items[selectedTimingItem].value < items[selectedTimingItem].maxVal) {
                *items[selectedTimingItem].value += items[selectedTimingItem].step;
                screenNeedsRedraw = true;
            }
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
                loadConfig(); // 원래 값 복구
                selectedTimingItem = -1;
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
                selectedTimingItem = -1;
                screenNeedsRedraw = true;
                return;
            }
        }
    }
    // 일반 모드일 때
    else {
        // 항목 선택
        for (int i = 0; i < 5; i++) {
            int16_t itemY = startY + i * (itemH + 4);
            
            if (x >= SPACING_SM && x <= SCREEN_WIDTH - SPACING_SM &&
                y >= itemY && y <= itemY + itemH) {
                selectedTimingItem = i;
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
            
            // 기본값 (관리자 전용)
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
                    // 기본값 복원
                    config.vacuumOnTime = 200;
                    config.vacuumHoldTime = 5000;
                    config.vacuumHoldExtension = 2000;
                    config.vacuumBreakTime = 700;
                    config.waitRemovalTime = 30000;
                    saveConfig();
                    screenNeedsRedraw = true;
                    return;
                }
            }
        }
    }
}