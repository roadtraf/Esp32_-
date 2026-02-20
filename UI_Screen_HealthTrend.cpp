// ================================================================
// UI_Screen_HealthTrend.cpp - 재설계 건강도 추세 화면
// ================================================================
#include "../include/UIComponents.h"
#include "../include/Config.h"
#include "../include/HealthMonitor.h"

using namespace UIComponents;
using namespace UITheme;

extern HealthMonitor healthMonitor;

void drawHealthTrendScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    // ── 헤더 ──
    drawHeader("건강도 추세");
    
    // 권한 확인
    if (!canAccessScreen(SCREEN_HEALTH_TREND)) {
        showAccessDenied("건강도 추세");
        NavButton navButtons[] = {{"뒤로", BTN_OUTLINE, true}};
        drawNavBar(navButtons, 1);
        return;
    }
    
    // ── 현재 건강도 요약 ──
    int16_t startY = HEADER_HEIGHT + SPACING_SM;
    
    CardConfig summaryCard = {
        .x = SPACING_SM,
        .y = startY,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = 50,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(summaryCard);
    
    float currentHealth = healthMonitor.getHealthScore();
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(summaryCard.x + CARD_PADDING, summaryCard.y + CARD_PADDING);
    tft.print("현재 건강도");
    
    tft.setTextSize(3);
    uint16_t healthColor = (currentHealth >= 90) ? COLOR_SUCCESS :
                           (currentHealth >= 75) ? COLOR_WARNING : COLOR_DANGER;
    tft.setTextColor(healthColor);
    tft.setCursor(summaryCard.x + 120, summaryCard.y + CARD_PADDING + 5);
    tft.printf("%.0f%%", currentHealth);
    
    // 추세 표시
    tft.setTextSize(TEXT_SIZE_SMALL);
    const char* trend = "안정";
    uint16_t trendColor = COLOR_INFO;
    
    // 간단한 추세 계산 (실제로는 이력 데이터 필요)
    if (currentHealth >= 95) {
        trend = "우수";
        trendColor = COLOR_SUCCESS;
    } else if (currentHealth < 70) {
        trend = "하락";
        trendColor = COLOR_DANGER;
    }
    
    drawBadge(summaryCard.x + summaryCard.w - 60, summaryCard.y + CARD_PADDING + 5,
              trend, (currentHealth >= 90) ? BADGE_SUCCESS : 
                     (currentHealth >= 70) ? BADGE_WARNING : BADGE_DANGER);
    
    // ── 그래프 영역 (간소화된 시각화) ──
    int16_t graphY = summaryCard.y + summaryCard.h + SPACING_SM;
    
    CardConfig graphCard = {
        .x = SPACING_SM,
        .y = graphY,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = 120,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(graphCard);
    
    // 그래프 타이틀
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(graphCard.x + CARD_PADDING, graphCard.y + CARD_PADDING);
    tft.print("최근 24시간 추세 (시뮬레이션)");
    
    // 간단한 막대 그래프 (샘플 데이터)
    int16_t barStartX = graphCard.x + CARD_PADDING;
    int16_t barStartY = graphCard.y + graphCard.h - CARD_PADDING - 5;
    int16_t barWidth = 15;
    int16_t barSpacing = 4;
    int16_t maxBarHeight = 70;
    
    // 샘플 데이터 (실제로는 healthMonitor에서 가져와야 함)
    float healthData[] = {98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85};
    
    for (int i = 0; i < 14; i++) {
        float value = healthData[i];
        int16_t barHeight = (value / 100.0f) * maxBarHeight;
        int16_t barX = barStartX + i * (barWidth + barSpacing);
        int16_t barY = barStartY - barHeight;
        
        uint16_t barColor;
        if (value >= 90) barColor = COLOR_SUCCESS;
        else if (value >= 75) barColor = COLOR_WARNING;
        else barColor = COLOR_DANGER;
        
        tft.fillRect(barX, barY, barWidth, barHeight, barColor);
    }
    
    // Y축 라벨
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(graphCard.x + CARD_PADDING, graphCard.y + 30);
    tft.print("100%");
    tft.setCursor(graphCard.x + CARD_PADDING, graphCard.y + 70);
    tft.print("50%");
    tft.setCursor(graphCard.x + CARD_PADDING, barStartY - 5);
    tft.print("0%");
    
    // ── 주요 지표 ──
    int16_t metricsY = graphCard.y + graphCard.h + SPACING_SM;
    int16_t metricW = (SCREEN_WIDTH - SPACING_SM * 4) / 3;
    int16_t metricH = 50;
    
    struct Metric {
        const char* label;
        float value;
        const char* unit;
        uint16_t color;
    };
    
    Metric metrics[] = {
        {"평균 온도", healthMonitor.getAvgTemperature(), "°C", COLOR_PRIMARY},
        {"최대 전류", healthMonitor.getMaxCurrent(), "A", COLOR_WARNING},
        {"가동 시간", (float)healthMonitor.getTotalRuntime() / 3600.0f, "h", COLOR_ACCENT}
    };
    
    for (int i = 0; i < 3; i++) {
        int16_t x = SPACING_SM + i * (metricW + SPACING_SM);
        
        CardConfig metricCard = {
            .x = x,
            .y = metricsY,
            .w = metricW,
            .h = metricH,
            .bgColor = COLOR_BG_CARD
        };
        drawCard(metricCard);
        
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(x + 4, metricsY + 4);
        tft.print(metrics[i].label);
        
        tft.setTextSize(TEXT_SIZE_MEDIUM);
        tft.setTextColor(metrics[i].color);
        tft.setCursor(x + 4, metricsY + 18);
        tft.printf("%.1f", metrics[i].value);
        
        tft.setTextSize(1);
        tft.print(metrics[i].unit);
    }
    
    // ── 하단 네비게이션 ──
    NavButton navButtons[] = {
        {"뒤로", BTN_OUTLINE, true}
    };
    drawNavBar(navButtons, 1);
}

void handleHealthTrendTouch(uint16_t x, uint16_t y) {
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
            currentScreen = SCREEN_HEALTH;
            screenNeedsRedraw = true;
        }
    }
}