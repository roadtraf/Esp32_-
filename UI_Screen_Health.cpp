// ================================================================
// UI_Screen_Health.cpp - 재설계 건강도 화면
// ================================================================
#include "../include/UIComponents.h"
#include "../include/Config.h"
#include "../include/HealthMonitor.h"

using namespace UIComponents;
using namespace UITheme;

extern HealthMonitor healthMonitor;

void drawHealthScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    // ── 헤더 ──
    drawHeader("건강도 모니터");
    
    // 권한 확인
    if (!canAccessScreen(SCREEN_HEALTH)) {
        showAccessDenied("건강도");
        NavButton navButtons[] = {{"뒤로", BTN_OUTLINE, true}};
        drawNavBar(navButtons, 1);
        return;
    }
    
    // ── 전체 건강도 카드 ──
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
    
    // 건강도 점수
    float healthScore = healthMonitor.getHealthScore();
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(healthCard.x + CARD_PADDING, healthCard.y + CARD_PADDING);
    tft.print("전체 건강도");
    
    // 큰 숫자로 점수 표시
    tft.setTextSize(4);
    uint16_t scoreColor;
    if (healthScore >= 90.0f) scoreColor = COLOR_SUCCESS;
    else if (healthScore >= 75.0f) scoreColor = COLOR_WARNING;
    else if (healthScore >= 50.0f) scoreColor = 0xFD20; // 주황
    else scoreColor = COLOR_DANGER;
    
    tft.setTextColor(scoreColor);
    tft.setCursor(healthCard.x + CARD_PADDING, healthCard.y + CARD_PADDING + 20);
    tft.printf("%.0f", healthScore);
    
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.print("%");
    
    // 프로그레스 바
    int16_t barX = healthCard.x + 120;
    int16_t barY = healthCard.y + CARD_PADDING + 25;
    int16_t barW = healthCard.w - 140;
    
    drawProgressBar(barX, barY, barW, 20, healthScore, scoreColor);
    
    // 유지보수 레벨
    MaintenanceLevel level = healthMonitor.getMaintenanceLevel();
    const char* levelText;
    BadgeType badgeType;
    
    switch (level) {
        case MAINTENANCE_GOOD:
            levelText = "양호";
            badgeType = BADGE_SUCCESS;
            break;
        case MAINTENANCE_ATTENTION:
            levelText = "주의";
            badgeType = BADGE_WARNING;
            break;
        case MAINTENANCE_REQUIRED:
            levelText = "필요";
            badgeType = BADGE_DANGER;
            break;
        case MAINTENANCE_CRITICAL:
            levelText = "긴급";
            badgeType = BADGE_DANGER;
            break;
        default:
            levelText = "알 수 없음";
            badgeType = BADGE_INFO;
    }
    
    drawBadge(healthCard.x + CARD_PADDING, healthCard.y + healthCard.h - 25, 
              levelText, badgeType);
    
    // ── 세부 항목 (2x4 그리드) ──
    int16_t gridY = healthCard.y + healthCard.h + SPACING_SM;
    int16_t itemW = (SCREEN_WIDTH - SPACING_SM * 3) / 2;
    int16_t itemH = 38;
    
    // 항목 데이터
    struct HealthItem {
        const char* label;
        float value;
        const char* unit;
        uint16_t color;
    };
    
    HealthItem items[] = {
        {"가동 시간", (float)healthMonitor.getTotalRuntime() / 3600.0f, "h", COLOR_PRIMARY},
        {"사이클", (float)stats.totalCycles, "회", COLOR_ACCENT},
        {"평균 온도", healthMonitor.getAvgTemperature(), "°C", COLOR_INFO},
        {"최대 온도", healthMonitor.getMaxTemperature(), "°C", COLOR_WARNING},
        {"평균 전류", healthMonitor.getAvgCurrent(), "A", COLOR_PRIMARY},
        {"최대 전류", healthMonitor.getMaxCurrent(), "A", COLOR_DANGER},
        {"성공률", (stats.totalCycles > 0) ? (float)stats.successfulCycles / stats.totalCycles * 100 : 0, "%", COLOR_SUCCESS},
        {"오류 횟수", (float)errorHistCnt, "회", COLOR_DANGER}
    };
    
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 2; col++) {
            int idx = row * 2 + col;
            
            int16_t x = SPACING_SM + col * (itemW + SPACING_SM);
            int16_t y = gridY + row * (itemH + 4);
            
            // 작은 카드
            CardConfig itemCard = {
                .x = x,
                .y = y,
                .w = itemW,
                .h = itemH,
                .bgColor = COLOR_BG_CARD
            };
            drawCard(itemCard);
            
            // 라벨
            tft.setTextSize(1);
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(x + 6, y + 6);
            tft.print(items[idx].label);
            
            // 값
            tft.setTextSize(TEXT_SIZE_SMALL);
            tft.setTextColor(items[idx].color);
            tft.setCursor(x + 6, y + 18);
            tft.printf("%.1f %s", items[idx].value, items[idx].unit);
        }
    }
    
    // ── 하단 네비게이션 ──
    NavButton navButtons[] = {
        {"뒤로", BTN_OUTLINE, true},
        {"추세", BTN_PRIMARY, true},
        {"리셋", BTN_DANGER, systemController.getPermissions().canCalibrate}
    };
    drawNavBar(navButtons, 3);
}

void handleHealthTouch(uint16_t x, uint16_t y) {
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    
    if (y >= navY) {
        int16_t buttonW = (SCREEN_WIDTH - SPACING_SM * 4) / 3;
        
        // 뒤로 버튼
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
        
        // 추세 버튼
        ButtonConfig trendBtn = {
            .x = (int16_t)(SPACING_SM + buttonW + SPACING_SM),
            .y = (int16_t)(navY + 2),
            .w = buttonW,
            .h = (int16_t)(FOOTER_HEIGHT - 4),
            .label = "추세",
            .style = BTN_PRIMARY,
            .enabled = true
        };
        
        if (isButtonPressed(trendBtn, x, y)) {
            currentScreen = SCREEN_HEALTH_TREND;
            screenNeedsRedraw = true;
            return;
        }
        
        // 리셋 버튼 (관리자 전용)
        if (systemController.getPermissions().canCalibrate) {
            ButtonConfig resetBtn = {
                .x = (int16_t)(SPACING_SM + (buttonW + SPACING_SM) * 2),
                .y = (int16_t)(navY + 2),
                .w = buttonW,
                .h = (int16_t)(FOOTER_HEIGHT - 4),
                .label = "리셋",
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