// UI_Screen_WatchdogStatus.cpp

#include "../include/UIComponents.h"
#include "../include/Config.h"
#include "EnhancedWatchdog.h"

using namespace UIComponents;
using namespace UITheme;

void drawWatchdogStatusScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    // 헤더
    drawHeader("시스템 모니터");
    
    int16_t y = HEADER_HEIGHT + SPACING_MD;
    
    // 전체 상태 카드
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
    tft.print(enhancedWatchdog.isHealthy() ? "정상" : "경고");
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setCursor(statusCard.x + CARD_PADDING, statusCard.y + CARD_PADDING + 22);
    tft.printf("가동: %lu초", enhancedWatchdog.getUptimeSeconds());
    
    y += statusCard.h + SPACING_SM;
    
    // 태스크 상태 목록
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
        
        // 태스크 이름
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.setCursor(taskCard.x + CARD_PADDING, taskCard.y + CARD_PADDING);
        tft.print(task->name);
        
        // 상태 배지
        const char* statusText;
        BadgeType badgeType;
        
        switch (task->status) {
            case TASK_HEALTHY:
                statusText = "정상";
                badgeType = BADGE_SUCCESS;
                break;
            case TASK_SLOW:
                statusText = "느림";
                badgeType = BADGE_WARNING;
                break;
            case TASK_STALLED:
                statusText = "정지";
                badgeType = BADGE_DANGER;
                break;
            default:
                statusText = "미확인";
                badgeType = BADGE_INFO;
                break;
        }
        
        drawBadge(taskCard.x + taskCard.w - 60, taskCard.y + CARD_PADDING, 
                  statusText, badgeType);
        
        y += taskCard.h + 4;
    }
    
    // 네비게이션
    NavButton navButtons[] = {
        {"뒤로", BTN_OUTLINE, true}
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