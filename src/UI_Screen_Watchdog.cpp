// ================================================================
// UI_Screen_Watchdog.cpp - Enhanced Watchdog 상태 화면
// Phase 3-1: 안정성 강화
// ================================================================
#include "UIComponents.h"
#include "Config.h"
#include "EnhancedWatchdog.h"

using namespace UIComponents;
using namespace UITheme;

extern SystemController systemController;

void drawWatchdogScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    // ── 헤더 ──
    drawHeader("시스템 상태 모니터");
    
    int16_t startY = HEADER_HEIGHT + SPACING_SM;
    
    // ── 전체 시스템 상태 카드 ──
    bool systemHealthy = enhancedWatchdog.isSystemHealthy();
    
    CardConfig statusCard = {
        .x = SPACING_SM,
        .y = startY,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = 70,
        .bgColor = systemHealthy ? COLOR_SUCCESS : COLOR_DANGER,
        .elevated = true
    };
    drawCard(statusCard);
    
    // 시스템 상태
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(statusCard.x + CARD_PADDING, statusCard.y + CARD_PADDING);
    
    if (systemHealthy) {
        tft.print("✅ 시스템 정상");
    } else {
        tft.print("⚠️ 시스템 이상 감지");
    }
    
    // 통계 정보
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(statusCard.x + CARD_PADDING, statusCard.y + CARD_PADDING + 25);
    
    uint32_t uptime = enhancedWatchdog.getUptimeSeconds();
    uint32_t hours = uptime / 3600;
    uint32_t minutes = (uptime % 3600) / 60;
    
    tft.printf("가동: %luh %lum", hours, minutes);
    
    tft.setCursor(statusCard.x + CARD_PADDING, statusCard.y + CARD_PADDING + 40);
    tft.printf("재시작: %d회 | 태스크: %d/%d 정상",
               enhancedWatchdog.getRestartCount(),
               enhancedWatchdog.getHealthyTaskCount(),
               enhancedWatchdog.getTotalTaskCount());
    
    // ── 재시작 루프 경고 ──
    if (enhancedWatchdog.isRestartLoopDetected()) {
        int16_t warningY = statusCard.y + statusCard.h + SPACING_SM;
        
        CardConfig warningCard = {
            .x = SPACING_SM,
            .y = warningY,
            .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
            .h = 40,
            .bgColor = COLOR_DANGER,
            .elevated = true
        };
        drawCard(warningCard);
        
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.setCursor(warningCard.x + CARD_PADDING, warningCard.y + CARD_PADDING);
        tft.print("🔥 재시작 루프 감지!");
        
        tft.setTextSize(1);
        tft.setCursor(warningCard.x + CARD_PADDING, warningCard.y + CARD_PADDING + 18);
        tft.print("안전 모드 진입 권장");
        
        startY = warningY + warningCard.h + SPACING_SM;
    } else {
        startY = statusCard.y + statusCard.h + SPACING_SM;
    }
    
    // ── 태스크 목록 ──
    const char* taskNames[] = {"VacuumCtrl", "SensorRead", "UIUpdate", "WiFiMgr"};
    int16_t taskCardH = 38;
    
    for (int i = 0; i < 4; i++) {
        int16_t taskY = startY + i * (taskCardH + 4);
        
        // 화면 하단 네비게이션 영역 피하기
        if (taskY + taskCardH > SCREEN_HEIGHT - FOOTER_HEIGHT - SPACING_SM) {
            break;
        }
        
        CardConfig taskCard = {
            .x = SPACING_SM,
            .y = taskY,
            .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
            .h = taskCardH,
            .bgColor = COLOR_BG_CARD
        };
        drawCard(taskCard);
        
        // 태스크 상태 가져오기
        TaskHealthStatus status = enhancedWatchdog.getTaskStatus(taskNames[i]);
        uint32_t lastCheckIn = enhancedWatchdog.getTaskLastCheckIn(taskNames[i]);
        uint32_t timeSince = (millis() - lastCheckIn) / 1000;
        
        // 상태 아이콘 색상
        uint16_t iconColor;
        
        switch (status) {
            case TASK_HEALTHY:
                iconColor = COLOR_SUCCESS;
                break;
            case TASK_SLOW:
                iconColor = COLOR_WARNING;
                break;
            case TASK_STALLED:
            case TASK_DEADLOCKED:
                iconColor = COLOR_DANGER;
                break;
            default:
                iconColor = COLOR_TEXT_DISABLED;
        }
        
        // 상태 표시 (원으로 간단히)
        tft.fillCircle(taskCard.x + 15, taskCard.y + taskCardH / 2, 7, iconColor);
        
        // 태스크 이름
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.setCursor(taskCard.x + 30, taskCard.y + 8);
        tft.print(taskNames[i]);
        
        // 마지막 체크인 시간
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(taskCard.x + 30, taskCard.y + 24);
        
        if (timeSince < 60) {
            tft.printf("%lus ago", timeSince);
        } else {
            tft.printf("%lum ago", timeSince / 60);
        }
        
        // 우측에 상태 텍스트
        tft.setTextSize(1);
        
        const char* statusText;
        switch (status) {
            case TASK_HEALTHY:    statusText = "정상"; break;
            case TASK_SLOW:       statusText = "느림"; break;
            case TASK_STALLED:    statusText = "정지"; break;
            case TASK_DEADLOCKED: statusText = "데드락"; break;
            default:              statusText = "알 수 없음";
        }
        
        int16_t textX = taskCard.x + taskCard.w - 40;
        tft.setCursor(textX, taskCard.y + taskCardH / 2 - 4);
        tft.print(statusText);
    }
    
    // ── 하단 네비게이션 ──
    NavButton navButtons[] = {
        {"새로고침", BTN_PRIMARY, true},
        {"뒤로", BTN_OUTLINE, true}
    };
    drawNavBar(navButtons, 2);
}

void handleWatchdogTouch(uint16_t x, uint16_t y) {
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    
    if (y >= navY) {
        int16_t buttonW = (SCREEN_WIDTH - SPACING_SM * 3) / 2;
        
        // 새로고침 버튼
        ButtonConfig refreshBtn = {
            .x = SPACING_SM,
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
        
        // 뒤로 버튼
        ButtonConfig backBtn = {
            .x = (int16_t)(SPACING_SM + buttonW + SPACING_SM),
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
    }
}
