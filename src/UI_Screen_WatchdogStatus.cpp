// UI_Screen_WatchdogStatus.cpp

#include "UIComponents.h"
#include "Config.h"
#include "EnhancedWatchdog.h"

using namespace UIComponents;
using namespace UITheme;

void drawWatchdogStatusScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    // 
    drawHeader(" ");
    
    int16_t y = HEADER_HEIGHT + SPACING_MD;
    
    //   
    CardConfig statusCard = {
        .x = SPACING_SM,
        .y = y,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = 60,
        .bgColor = enhancedWatchdog.isHealthy() ? COLOR_SUCCESS : COLOR_DANGER
    };
    drawCard(statusCard);
    
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(statusCard.x + CARD_PADDING, statusCard.y + CARD_PADDING);
    tft.print(enhancedWatchdog.isHealthy() ? "" : "");
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setCursor(statusCard.x + CARD_PADDING, statusCard.y + CARD_PADDING + 22);
    tft.printf(": %lu", enhancedWatchdog.getUptimeSeconds());
    
    y += statusCard.h + SPACING_SM;
    
    //   
    const char* taskNames[] = {"VacuumCtrl", "SensorRead", "UIUpdate", "WiFiMgr"};
    
    for (int i = 0; i < 4; i++) {
        TaskInfo* task = enhancedWatchdog.getTaskInfo(taskNames[i]);
        if (!task) continue;
        
        CardConfig taskCard = {
            .x = SPACING_SM,
            .y = y,
            .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
            .h = 40,
            .bgColor = COLOR_BG_CARD
        };
        drawCard(taskCard);
        
        //  
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.setCursor(taskCard.x + CARD_PADDING, taskCard.y + CARD_PADDING);
        tft.print(task->name);
        
        //  
        const char* statusText;
        BadgeType badgeType;
        
        switch (task->status) {
            case TASK_HEALTHY:
                statusText = "";
                badgeType = BADGE_SUCCESS;
                break;
            case TASK_SLOW:
                statusText = "";
                badgeType = BADGE_WARNING;
                break;
            case TASK_STALLED:
                statusText = "";
                badgeType = BADGE_DANGER;
                break;
            default:
                statusText = "";
                badgeType = BADGE_INFO;
                break;
        }
        
        drawBadge(taskCard.x + taskCard.w - 60, taskCard.y + CARD_PADDING, 
                  statusText, badgeType);
        
        y += taskCard.h + 4;
    }
    
    // 
    NavButton navButtons[] = {
        {"", BTN_OUTLINE, true}
    };
    drawNavBar(navButtons, 1);
}

void handleWatchdogStatusTouch(uint16_t x, uint16_t y) {
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    
    if (y >= navY) {
        currentScreen = SCREEN_SETTINGS;
        screenNeedsRedraw = true;
    }
}