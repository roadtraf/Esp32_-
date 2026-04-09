// ================================================================
// UI_Screen_HealthTrend.cpp -    
// ================================================================
#include "UIComponents.h"
#include "Config.h"
#include "ManagerUI.h"
#include "HealthMonitor.h"

using namespace UIComponents;
using namespace UITheme;

extern HealthMonitor healthMonitor;

void drawHealthTrendScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    //   
    drawHeader(" ");
    
    //  
    if (false && !canAccessScreen(SCREEN_HEALTH_TREND)) {
        Serial.println("Access Denied");  // showAccessDenied
        NavButton navButtons[] = {{"", BTN_OUTLINE, true}};
        drawNavBar(navButtons, 1);
        return;
    }
    
    //     
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
    tft.print(" ");
    
    tft.setTextSize(3);
    uint16_t healthColor = (currentHealth >= 90) ? COLOR_SUCCESS :
                           (currentHealth >= 75) ? COLOR_WARNING : COLOR_DANGER;
    tft.setTextColor(healthColor);
    tft.setCursor(summaryCard.x + 120, summaryCard.y + CARD_PADDING + 5);
    tft.printf("%.0f%%", currentHealth);
    
    //  
    tft.setTextSize(TEXT_SIZE_SMALL);
    const char* trend = "";
    uint16_t trendColor = COLOR_INFO;
    
    //    (   )
    if (currentHealth >= 95) {
        trend = "";
        trendColor = COLOR_SUCCESS;
    } else if (currentHealth < 70) {
        trend = "";
        trendColor = COLOR_DANGER;
    }
    
    drawBadge(summaryCard.x + summaryCard.w - 60, summaryCard.y + CARD_PADDING + 5,
              trend, (currentHealth >= 90) ? BADGE_SUCCESS : 
                     (currentHealth >= 70) ? BADGE_WARNING : BADGE_DANGER);
    
    //    ( ) 
    int16_t graphY = summaryCard.y + summaryCard.h + SPACING_SM;
    
    CardConfig graphCard = {
        .x = SPACING_SM,
        .y = graphY,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = 120,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(graphCard);
    
    //  
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(graphCard.x + CARD_PADDING, graphCard.y + CARD_PADDING);
    tft.print(" 24  ()");
    
    //    ( )
    int16_t barStartX = graphCard.x + CARD_PADDING;
    int16_t barStartY = graphCard.y + graphCard.h - CARD_PADDING - 5;
    int16_t barWidth = 15;
    int16_t barSpacing = 4;
    int16_t maxBarHeight = 70;
    
    //   ( healthMonitor  )
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
    
    // Y 
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(graphCard.x + CARD_PADDING, graphCard.y + 30);
    tft.print("100%");
    tft.setCursor(graphCard.x + CARD_PADDING, graphCard.y + 70);
    tft.print("50%");
    tft.setCursor(graphCard.x + CARD_PADDING, barStartY - 5);
    tft.print("0%");
    
    //    
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
        {" ", healthMonitor.getPeakTemperature(), "C", COLOR_PRIMARY},
        {" ", healthMonitor.getPeakCurrent(), "A", COLOR_WARNING},
        {" ", (float)healthMonitor.getTotalRuntime() / 3600.0f, "h", COLOR_ACCENT}
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
    
    //    
    NavButton navButtons[] = {
        {"", BTN_OUTLINE, true}
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
            .label = "",
            .style = BTN_OUTLINE,
            .enabled = true
        };
        
        if (isButtonPressed(backBtn, x, y)) {
            currentScreen = SCREEN_HEALTH;
            screenNeedsRedraw = true;
        }
    }
}