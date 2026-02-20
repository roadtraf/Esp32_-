// ================================================================
// UI_Screen_TrendGraph.cpp - 재설계 추세 그래프 화면
// ================================================================
#include "../include/UIComponents.h"
#include "../include/Config.h"

using namespace UIComponents;
using namespace UITheme;

void drawTrendGraphScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    // ── 헤더 ──
    drawHeader("추세 그래프");
    
    // ── 데이터 선택 ──
    int16_t startY = HEADER_HEIGHT + SPACING_SM;
    
    struct DataType {
        const char* label;
        uint16_t color;
        bool selected;
    };
    
    static DataType dataTypes[] = {
        {"압력", COLOR_PRIMARY, true},
        {"전류", COLOR_WARNING, false},
        {"온도", COLOR_DANGER, false}
    };
    
    int16_t btnW = (SCREEN_WIDTH - SPACING_SM * 4) / 3;
    int16_t btnH = 28;
    
    for (int i = 0; i < 3; i++) {
        int16_t btnX = SPACING_SM + i * (btnW + SPACING_SM);
        
        ButtonConfig typeBtn = {
            .x = btnX,
            .y = startY,
            .w = btnW,
            .h = btnH,
            .label = dataTypes[i].label,
            .style = dataTypes[i].selected ? BTN_PRIMARY : BTN_SECONDARY,
            .enabled = true
        };
        drawButton(typeBtn);
    }
    
    // ── 그래프 영역 ──
    int16_t graphY = startY + btnH + SPACING_SM;
    
    CardConfig graphCard = {
        .x = SPACING_SM,
        .y = graphY,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = 150,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(graphCard);
    
    // 그래프 타이틀
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(graphCard.x + CARD_PADDING, graphCard.y + CARD_PADDING);
    
    for (int i = 0; i < 3; i++) {
        if (dataTypes[i].selected) {
            tft.print(dataTypes[i].label);
            tft.print(" 추세 (최근 1분)");
            break;
        }
    }
    
    // 간단한 라인 그래프
    int16_t plotX = graphCard.x + CARD_PADDING + 25;
    int16_t plotY = graphCard.y + CARD_PADDING + 25;
    int16_t plotW = graphCard.w - CARD_PADDING * 2 - 30;
    int16_t plotH = graphCard.h - CARD_PADDING * 2 - 30;
    
    // 그리드
    tft.drawRect(plotX, plotY, plotW, plotH, COLOR_DIVIDER);
    for (int i = 1; i < 4; i++) {
        int16_t y = plotY + (plotH / 4) * i;
        tft.drawFastHLine(plotX, y, plotW, COLOR_DIVIDER);
    }
    
    // 샘플 데이터 그리기 (실제로는 센서 버퍼에서 가져와야 함)
    uint16_t lineColor = COLOR_PRIMARY;
    for (int i = 0; i < 3; i++) {
        if (dataTypes[i].selected) {
            lineColor = dataTypes[i].color;
            break;
        }
    }
    
    // 간단한 사인파 샘플
    int16_t prevX = plotX;
    int16_t prevY = plotY + plotH / 2;
    
    for (int x = 0; x < plotW; x += 5) {
        float angle = (x / (float)plotW) * 6.28f * 2;
        int16_t y = plotY + plotH / 2 - (sin(angle) * plotH / 4);
        
        tft.drawLine(prevX, prevY, plotX + x, y, lineColor);
        prevX = plotX + x;
        prevY = y;
    }
    
    // Y축 라벨
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(graphCard.x + 4, plotY);
    tft.print("100");
    tft.setCursor(graphCard.x + 4, plotY + plotH / 2 - 4);
    tft.print("50");
    tft.setCursor(graphCard.x + 4, plotY + plotH - 8);
    tft.print("0");
    
    // ── 통계 정보 ──
    int16_t statsY = graphCard.y + graphCard.h + SPACING_SM;
    
    struct StatInfo {
        const char* label;
        float value;
        const char* unit;
    };
    
    StatInfo stats[] = {
        {"평균", 75.5f, "kPa"},
        {"최소", 68.2f, "kPa"},
        {"최대", 82.1f, "kPa"}
    };
    
    int16_t statW = (SCREEN_WIDTH - SPACING_SM * 4) / 3;
    int16_t statH = 45;
    
    for (int i = 0; i < 3; i++) {
        int16_t statX = SPACING_SM + i * (statW + SPACING_SM);
        
        CardConfig statCard = {
            .x = statX,
            .y = statsY,
            .w = statW,
            .h = statH,
            .bgColor = COLOR_BG_CARD
        };
        drawCard(statCard);
        
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(statX + 4, statsY + 4);
        tft.print(stats[i].label);
        
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(lineColor);
        tft.setCursor(statX + 4, statsY + 18);
        tft.printf("%.1f", stats[i].value);
        
        tft.setTextSize(1);
        tft.print(" ");
        tft.print(stats[i].unit);
    }
    
    // ── 하단 네비게이션 ──
    NavButton navButtons[] = {
        {"뒤로", BTN_OUTLINE, true}
    };
    drawNavBar(navButtons, 1);
}

void handleTrendGraphTouch(uint16_t x, uint16_t y) {
    int16_t startY = HEADER_HEIGHT + SPACING_SM;
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    
    // 데이터 타입 선택 버튼
    int16_t btnW = (SCREEN_WIDTH - SPACING_SM * 4) / 3;
    int16_t btnH = 28;
    
    if (y >= startY && y <= startY + btnH) {
        for (int i = 0; i < 3; i++) {
            int16_t btnX = SPACING_SM + i * (btnW + SPACING_SM);
            
            if (x >= btnX && x <= btnX + btnW) {
                // 모든 선택 해제 후 선택
                // (실제로는 static 배열 수정 필요)
                screenNeedsRedraw = true;
                return;
            }
        }
    }
    
    // 네비게이션
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
        }
    }
}
