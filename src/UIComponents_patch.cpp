// ================================================================
// UIComponents.cpp 패치 파일 — 개선된 함수들
// [U7]  strlen()*6 → tft.textWidth() 전면 교체
// [U12] drawHeader() 건강도 아이콘 재설계
// ================================================================
// ▶ 이 파일의 함수들이 기존 UIComponents.cpp의 해당 함수를 대체합니다.
//   기존 UIComponents.cpp에서 아래 함수 본체를 이 내용으로 교체하세요.
// ================================================================

#include "../include/UIComponents.h"
#include "../include/Config.h"
#include "../include/SystemController.h"
#include "../include/HealthMonitor.h"

extern LGFX tft;
extern SystemController systemController;
extern HealthMonitor healthMonitor;

namespace UIComponents {

// ================================================================
// [U12] drawHeader() — 건강도 아이콘 재설계
//       하트 근사(원+삼각) 대신 숫자 + 색상 바 방식으로 명확하게
// ================================================================
void drawHeader(const char* title, bool showManagerBadge) {
    using namespace UITheme;

    // 배경
    tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BG_DARK);
    tft.drawFastHLine(0, HEADER_HEIGHT - 1, SCREEN_WIDTH, COLOR_DIVIDER);

    // 타이틀
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(SPACING_SM, 10);
    tft.print(title);

    // 관리자 배지
    if (showManagerBadge && !systemController.isOperatorMode()) {
        AccessLevel level = systemController.getCurrentLevel();
        const char* badgeText  = (level == ACCESS_DEVELOPER) ? "DEV" : "MGR";
        uint16_t    badgeColor = (level == ACCESS_DEVELOPER) ? COLOR_DEVELOPER : COLOR_MANAGER;

        // [U7] textWidth 기반 배지 위치 계산
        int16_t titleW  = tft.textWidth(title);
        int16_t badgeX  = SPACING_SM + titleW + BADGE_X_OFFSET;

        tft.fillRoundRect(badgeX, BADGE_Y_OFFSET, BADGE_WIDTH, BADGE_HEIGHT, 7, badgeColor);
        tft.setTextSize(1);
        tft.setTextColor(COLOR_BG_DARK);

        // [U7] 배지 내 텍스트 중앙 정렬
        int16_t btw = tft.textWidth(badgeText);
        tft.setCursor(badgeX + (BADGE_WIDTH - btw) / 2, BADGE_Y_OFFSET + 3);
        tft.print(badgeText);
    }

    // [U12] 건강도 표시 재설계 — 심볼 + 숫자 + 색상 바
#ifdef ENABLE_PREDICTIVE_MAINTENANCE
    float   healthScore = healthMonitor.getHealthScore();
    uint16_t hColor = (healthScore >= 90.0f) ? COLOR_SUCCESS
                    : (healthScore >= 75.0f) ? COLOR_WARNING
                    :                          COLOR_DANGER;

    // 아이콘 영역 (우측 상단)
    int16_t iconX = SCREEN_WIDTH - 62;
    int16_t iconY = 4;

    // 하트 심볼 (♥ UTF-8: E2 99 A5) 대신 H 텍스트 + 색상 원
    tft.fillCircle(iconX, iconY + 8, 6, hColor);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_BG_DARK);
    tft.setCursor(iconX - 2, iconY + 5);
    tft.print("H");

    // 건강도 수치
    char hBuf[8];
    snprintf(hBuf, sizeof(hBuf), "%.0f%%", healthScore);
    tft.setTextSize(1);
    tft.setTextColor(hColor);

    // [U7] textWidth 기반 정렬
    int16_t hw = tft.textWidth(hBuf);
    tft.setCursor(SCREEN_WIDTH - SPACING_SM - hw, iconY + 5);
    tft.print(hBuf);

    // 작은 상태 바 (아이콘 아래)
    int16_t barX = iconX - 4;
    int16_t barW = SCREEN_WIDTH - SPACING_SM - barX;
    tft.fillRoundRect(barX, iconY + 17, barW, 4, 2, COLOR_BG_ELEVATED);
    int16_t fillW = (int16_t)(barW * healthScore / 100.0f);
    if (fillW > 2) {
        tft.fillRoundRect(barX, iconY + 17, fillW, 4, 2, hColor);
    }
#endif
}

// ================================================================
// drawButton() — [U7] 텍스트 중앙 정렬 수정
// ================================================================
void drawButton(const ButtonConfig& config) {
    using namespace UITheme;

    uint16_t bgColor, borderColor, textColor;

    switch (config.style) {
        case BTN_PRIMARY:
            bgColor = config.enabled ? COLOR_PRIMARY : COLOR_TEXT_DISABLED;
            borderColor = bgColor; textColor = COLOR_TEXT_PRIMARY; break;
        case BTN_SECONDARY:
            bgColor = config.enabled ? COLOR_ACCENT : COLOR_TEXT_DISABLED;
            borderColor = bgColor; textColor = COLOR_TEXT_PRIMARY; break;
        case BTN_SUCCESS:
            bgColor = config.enabled ? COLOR_SUCCESS : COLOR_TEXT_DISABLED;
            borderColor = bgColor; textColor = COLOR_TEXT_PRIMARY; break;
        case BTN_DANGER:
            bgColor = config.enabled ? COLOR_DANGER : COLOR_TEXT_DISABLED;
            borderColor = bgColor; textColor = COLOR_TEXT_PRIMARY; break;
        case BTN_OUTLINE:
            bgColor = COLOR_BG_CARD;
            borderColor = config.enabled ? COLOR_BORDER : COLOR_TEXT_DISABLED;
            textColor   = config.enabled ? COLOR_TEXT_PRIMARY : COLOR_TEXT_DISABLED; break;
        default:
            bgColor = COLOR_BG_CARD; borderColor = COLOR_BORDER;
            textColor = COLOR_TEXT_PRIMARY;
    }

    tft.fillRoundRect(config.x, config.y, config.w, config.h, BUTTON_RADIUS, bgColor);
    tft.drawRoundRect(config.x, config.y, config.w, config.h, BUTTON_RADIUS, borderColor);

    if (!config.label || strlen(config.label) == 0) return;

    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(textColor);

    // [U7] textWidth() 기반 중앙 정렬 (strlen()*6 제거)
    int16_t tw = tft.textWidth(config.label);
    int16_t th = 8 * TEXT_SIZE_SMALL;  // 폰트 높이
    int16_t tx = config.x + (config.w - tw) / 2;
    int16_t ty = config.y + (config.h - th) / 2;

    tft.setCursor(tx, ty);
    tft.print(config.label);
}

// ================================================================
// drawNavBar() — [U7] 버튼 라벨 중앙 정렬 수정
// ================================================================
void drawNavBar(NavButton buttons[], uint8_t count) {
    using namespace UITheme;

    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    tft.fillRect(0, navY, SCREEN_WIDTH, FOOTER_HEIGHT, COLOR_BG_DARK);
    tft.drawFastHLine(0, navY, SCREEN_WIDTH, COLOR_DIVIDER);

    int16_t totalSpacing = SPACING_SM * (count + 1);
    int16_t buttonW = (SCREEN_WIDTH - totalSpacing) / count;
    int16_t buttonH = FOOTER_HEIGHT - 4;

    for (uint8_t i = 0; i < count; i++) {
        int16_t btnX = SPACING_SM + i * (buttonW + SPACING_SM);
        int16_t btnY = navY + 2;

        ButtonConfig btn = {
            .x = btnX, .y = btnY,
            .w = buttonW, .h = buttonH,
            .label = buttons[i].label,
            .style = buttons[i].style,
            .enabled = buttons[i].enabled
        };
        drawButton(btn);  // 위에서 고친 drawButton 호출
    }
}

// ================================================================
// drawBadge() — [U7] textWidth 기반 크기 계산
// ================================================================
void drawBadge(int16_t x, int16_t y, const char* text, BadgeType type) {
    using namespace UITheme;

    uint16_t bgColor;
    switch (type) {
        case BADGE_SUCCESS: bgColor = COLOR_SUCCESS; break;
        case BADGE_WARNING: bgColor = COLOR_WARNING; break;
        case BADGE_DANGER:  bgColor = COLOR_DANGER;  break;
        default:            bgColor = COLOR_INFO;    break;
    }

    tft.setTextSize(1);
    // [U7] textWidth 기반 배지 너비 (strlen()*6 제거)
    int16_t tw      = tft.textWidth(text);
    int16_t badgeW  = tw + SPACING_SM * 2;
    int16_t badgeH  = 18;

    tft.fillRoundRect(x, y, badgeW, badgeH, BADGE_RADIUS, bgColor);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(x + (badgeW - tw) / 2, y + 5);
    tft.print(text);
}

// ================================================================
// drawManagerBadge() — [U11] TFT_* 색상 → UITheme 색상으로 통일
// ================================================================
void drawManagerBadge() {
    using namespace UITheme;

    SystemMode mode = systemController.getMode();
    if (mode == SystemMode::OPERATOR) return;

    int16_t  x = SCREEN_WIDTH - 100;
    int16_t  y = 5;
    int16_t  w = 95;
    int16_t  h = 25;

    // [U11] TFT_ORANGE/TFT_RED → UITheme 색상
    uint16_t bgColor = (mode == SystemMode::MANAGER)
                       ? COLOR_MANAGER : COLOR_DEVELOPER;

    tft.fillRoundRect(x, y, w, h, 5, bgColor);
    tft.drawRoundRect(x, y, w, h, 5, COLOR_TEXT_PRIMARY);  // [U11] TFT_WHITE 제거

    tft.setTextSize(1);
    tft.setTextColor(COLOR_BG_DARK);  // [U11] TFT_WHITE → 다크 배경

    const char* modeText = systemController.getModeString();
    // [U7] textWidth 기반 중앙 정렬
    int16_t tw = tft.textWidth(modeText);
    tft.setCursor(x + (w - tw) / 2, y + (h - 8) / 2);
    tft.print(modeText);
}

} // namespace UIComponents
