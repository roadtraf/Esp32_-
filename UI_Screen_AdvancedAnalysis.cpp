// ================================================================
// UI_Screen_AdvancedAnalysis.cpp - 재설계 고급 분석 화면
// ================================================================
#include "../include/UIComponents.h"
#include "../include/Config.h"

using namespace UIComponents;
using namespace UITheme;

void drawAdvancedAnalysisScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    // ── 헤더 ──
    drawHeader("고급 분석");
    
    // 권한 확인
    if (!systemController.getPermissions().canViewAdvanced) {
        showAccessDenied("고급 분석");
        NavButton navButtons[] = {{"뒤로", BTN_OUTLINE, true}};
        drawNavBar(navButtons, 1);
        return;
    }
    
    // ── 시스템 성능 ──
    int16_t startY = HEADER_HEIGHT + SPACING_SM;
    
    CardConfig perfCard = {
        .x = SPACING_SM,
        .y = startY,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = 60,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(perfCard);
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(perfCard.x + CARD_PADDING, perfCard.y + CARD_PADDING);
    tft.print("시스템 성능");
    
    // CPU 사용률 (샘플)
    float cpuUsage = 45.2f;
    drawProgressBar(perfCard.x + CARD_PADDING, perfCard.y + CARD_PADDING + 20,
                    perfCard.w - CARD_PADDING * 2, 18, cpuUsage, COLOR_PRIMARY);
    
    // ── 메모리 분석 ──
    int16_t memY = perfCard.y + perfCard.h + SPACING_SM;
    int16_t cardW = (SCREEN_WIDTH - SPACING_SM * 3) / 2;
    int16_t cardH = 65;
    
    // Heap 메모리
    CardConfig heapCard = {
        .x = SPACING_SM,
        .y = memY,
        .w = cardW,
        .h = cardH,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(heapCard);
    
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t minFreeHeap = ESP.getMinFreeHeap();
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(heapCard.x + CARD_PADDING, heapCard.y + CARD_PADDING);
    tft.print("Heap 메모리");
    
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(COLOR_SUCCESS);
    tft.setCursor(heapCard.x + CARD_PADDING, heapCard.y + CARD_PADDING + 18);
    tft.printf("%lu", freeHeap / 1024);
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.print(" KB");
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(heapCard.x + CARD_PADDING, heapCard.y + cardH - 14);
    tft.printf("Min: %lu KB", minFreeHeap / 1024);
    
    // 스택 메모리
    CardConfig stackCard = {
        .x = (int16_t)(SPACING_SM + cardW + SPACING_SM),
        .y = memY,
        .w = cardW,
        .h = cardH,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(stackCard);
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(stackCard.x + CARD_PADDING, stackCard.y + CARD_PADDING);
    tft.print("버퍼 사용");
    
    uint32_t bufferUsed = temperatureBuffer.size();
    uint32_t bufferMax = TEMP_BUFFER_SIZE;
    
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(COLOR_ACCENT);
    tft.setCursor(stackCard.x + CARD_PADDING, stackCard.y + CARD_PADDING + 18);
    tft.printf("%lu", bufferUsed);
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.print("/");
    tft.print(bufferMax);
    
    float bufferPercent = (float)bufferUsed / bufferMax * 100;
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(stackCard.x + CARD_PADDING, stackCard.y + cardH - 14);
    tft.printf("%.1f%% 사용 중", bufferPercent);
    
    // ── RTOS 태스크 분석 ──
    int16_t taskY = memY + cardH + SPACING_SM;
    
    CardConfig taskCard = {
        .x = SPACING_SM,
        .y = taskY,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = 85,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(taskCard);
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(taskCard.x + CARD_PADDING, taskCard.y + CARD_PADDING);
    tft.print("RTOS 태스크 (8개 실행 중)");
    
    // 태스크 목록 (샘플)
    struct TaskInfo {
        const char* name;
        uint8_t priority;
        uint16_t stackRemain;
    };
    
    TaskInfo tasks[] = {
        {"Vacuum", 3, 2048},
        {"Sensor", 2, 1536},
        {"UI", 1, 1024},
        {"Network", 1, 2048}
    };
    
    tft.setTextSize(1);
    int16_t lineY = taskCard.y + CARD_PADDING + 20;
    
    for (int i = 0; i < 4; i++) {
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(taskCard.x + CARD_PADDING, lineY);
        tft.print(tasks[i].name);
        
        tft.setTextColor(COLOR_PRIMARY);
        tft.setCursor(taskCard.x + CARD_PADDING + 70, lineY);
        tft.printf("P%d", tasks[i].priority);
        
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(taskCard.x + CARD_PADDING + 100, lineY);
        tft.printf("Stack: %u", tasks[i].stackRemain);
        
        lineY += 14;
    }
    
    // ── 하단 네비게이션 ──
    NavButton navButtons[] = {
        {"뒤로", BTN_OUTLINE, true},
        {"새로고침", BTN_PRIMARY, true}
    };
    drawNavBar(navButtons, 2);
}

void handleAdvancedAnalysisTouch(uint16_t x, uint16_t y) {
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    
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
        
        // 새로고침
        ButtonConfig refreshBtn = {
            .x = (int16_t)(SPACING_SM + buttonW + SPACING_SM),
            .y = (int16_t)(navY + 2),
            .w = buttonW,
            .h = (int16_t)(FOOTER_HEIGHT - 4),
            .label = "새로고침",
            .style = BTN_PRIMARY,
            .enabled = true
        };
        
        if (isButtonPressed(refreshBtn, x, y)) {
            screenNeedsRedraw = true;
            return;
        }
    }
}
