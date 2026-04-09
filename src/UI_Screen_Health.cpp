// ================================================================
// UI_Screen_Health.cpp -   
// ================================================================
#include "UIComponents.h"
#include "Config.h"
#include "ManagerUI.h"
#include "HealthMonitor.h"

using namespace UIComponents;
using namespace UITheme;

extern HealthMonitor healthMonitor;

void drawHealthScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    //   
    drawHeader(" ");
    
    //  
    if (false && !canAccessScreen(SCREEN_HEALTH)) {
        Serial.println("Access Denied");  // showAccessDenied
        NavButton navButtons[] = {{"", BTN_OUTLINE, true}};
        drawNavBar(navButtons, 1);
        return;
    }
    
    //     
    int16_t startY = HEADER_HEIGHT + SPACING_MD;
    
    CardConfig healthCard = {
        .x = SPACING_SM,
        .y = startY,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = 90,
        .bgColor = COLOR_BG_CARD,
        .elevated = true
    };
    drawCard(healthCard);
    
    //  
    float healthScore = healthMonitor.getHealthScore();
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(healthCard.x + CARD_PADDING, healthCard.y + CARD_PADDING);
    tft.print(" ");
    
    //    
    tft.setTextSize(4);
    uint16_t scoreColor;
    if (healthScore >= 90.0f) scoreColor = COLOR_SUCCESS;
    else if (healthScore >= 75.0f) scoreColor = COLOR_WARNING;
    else if (healthScore >= 50.0f) scoreColor = 0xFD20; // 
    else scoreColor = COLOR_DANGER;
    
    tft.setTextColor(scoreColor);
    tft.setCursor(healthCard.x + CARD_PADDING, healthCard.y + CARD_PADDING + 20);
    tft.printf("%.0f", healthScore);
    
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.print("%");
    
    //  
    int16_t barX = healthCard.x + 120;
    int16_t barY = healthCard.y + CARD_PADDING + 25;
    int16_t barW = healthCard.w - 140;
    
    drawProgressBar(barX, barY, barW, 20, healthScore, scoreColor);
    
    //  
    MaintenanceLevel level = healthMonitor.getMaintenanceLevel();
    const char* levelText;
    BadgeType badgeType;
    
    switch (level) {
        case MAINTENANCE_NONE:
            levelText = "";
            badgeType = BADGE_SUCCESS;
            break;
        case MAINTENANCE_SOON:
            levelText = "";
            badgeType = BADGE_WARNING;
            break;
        case MAINTENANCE_REQUIRED:
            levelText = "";
            badgeType = BADGE_DANGER;
            break;
        case MAINTENANCE_URGENT:
            levelText = "";
            badgeType = BADGE_DANGER;
            break;
        default:
            levelText = "  ";
            badgeType = BADGE_INFO;
    }
    
    drawBadge(healthCard.x + CARD_PADDING, healthCard.y + healthCard.h - 25, 
              levelText, badgeType);
    
    //    (2x4 ) 
    int16_t gridY = healthCard.y + healthCard.h + SPACING_SM;
    int16_t itemW = (SCREEN_WIDTH - SPACING_SM * 3) / 2;
    int16_t itemH = 38;
    
    //  
    struct HealthItem {
        const char* label;
        float value;
        const char* unit;
        uint16_t color;
    };
    
    HealthItem items[] = {
        {" ", (float)healthMonitor.getTotalRuntime() / 3600.0f, "h", COLOR_PRIMARY},
        {"", (float)stats.totalCycles, "", COLOR_ACCENT},
        {" ", healthMonitor.getPeakTemperature(), "C", COLOR_INFO},
        {" ", healthMonitor.getPeakTemperature(), "C", COLOR_WARNING},
        {" ", healthMonitor.getPeakCurrent(), "A", COLOR_PRIMARY},
        {" ", healthMonitor.getPeakCurrent(), "A", COLOR_DANGER},
        {"", (stats.totalCycles > 0) ? (float)stats.successfulCycles / stats.totalCycles * 100 : 0, "%", COLOR_SUCCESS},
        {" ", (float)errorHistCnt, "", COLOR_DANGER}
    };
    
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 2; col++) {
            int idx = row * 2 + col;
            
            int16_t x = SPACING_SM + col * (itemW + SPACING_SM);
            int16_t y = gridY + row * (itemH + 4);
            
            //  
            CardConfig itemCard = {
                .x = x,
                .y = y,
                .w = itemW,
                .h = itemH,
                .bgColor = COLOR_BG_CARD
            };
            drawCard(itemCard);
            
            // 
            tft.setTextSize(1);
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(x + 6, y + 6);
            tft.print(items[idx].label);
            
            // 
            tft.setTextSize(TEXT_SIZE_SMALL);
            tft.setTextColor(items[idx].color);
            tft.setCursor(x + 6, y + 18);
            tft.printf("%.1f %s", items[idx].value, items[idx].unit);
        }
    }
    
    //    
    NavButton navButtons[] = {
        {"", BTN_OUTLINE, true},
        {"", BTN_PRIMARY, true},
        {"", BTN_DANGER, systemController.getPermissions().canCalibrate}
    };
    drawNavBar(navButtons, 3);
}

void handleHealthTouch(uint16_t x, uint16_t y) {
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    
    if (y >= navY) {
        int16_t buttonW = (SCREEN_WIDTH - SPACING_SM * 4) / 3;
        
        //  
        ButtonConfig backBtn = {
            .x = SPACING_SM,
            .y = (int16_t)(navY + 2),
            .w = buttonW,
            .h = (int16_t)(FOOTER_HEIGHT - 4),
            .label = "",
            .style = BTN_OUTLINE,
            .enabled = true
        };
        
        if (isButtonPressed(backBtn, x, y)) {
            currentScreen = SCREEN_SETTINGS;
            screenNeedsRedraw = true;
            return;
        }
        
        //  
        ButtonConfig trendBtn = {
            .x = (int16_t)(SPACING_SM + buttonW + SPACING_SM),
            .y = (int16_t)(navY + 2),
            .w = buttonW,
            .h = (int16_t)(FOOTER_HEIGHT - 4),
            .label = "",
            .style = BTN_PRIMARY,
            .enabled = true
        };
        
        if (isButtonPressed(trendBtn, x, y)) {
            currentScreen = SCREEN_HEALTH_TREND;
            screenNeedsRedraw = true;
            return;
        }
        
        //   ( )
        if (systemController.getPermissions().canCalibrate) {
            ButtonConfig resetBtn = {
                .x = (int16_t)(SPACING_SM + (buttonW + SPACING_SM) * 2),
                .y = (int16_t)(navY + 2),
                .w = buttonW,
                .h = (int16_t)(FOOTER_HEIGHT - 4),
                .label = "",
                .style = BTN_DANGER,
                .enabled = true
            };
            
            if (isButtonPressed(resetBtn, x, y)) {
                healthMonitor.performMaintenance();
                screenNeedsRedraw = true;
                return;
            }
        }
    }
}