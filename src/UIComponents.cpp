// ================================================================
// UIComponents.cpp -   UI  
// ================================================================
#include "Config.h"
#include "UIComponents.h"
#include "SystemController.h"
#include "HealthMonitor.h"

extern TFT_GFX tft;
extern SystemController systemController;
extern HealthMonitor healthMonitor;

namespace UIComponents {

// ================================================================
// [U12] drawHeader()    
//        (+)   +    
// ================================================================
void drawHeader(const char* title, bool showManagerBadge) {
    using namespace UITheme;

    // 
    tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BG_DARK);
    tft.drawFastHLine(0, HEADER_HEIGHT - 1, SCREEN_WIDTH, COLOR_DIVIDER);

    // 
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(SPACING_SM, 10);
    tft.print(title);

    //  
    if (showManagerBadge && !systemController.isOperatorMode()) {
        // AccessLevel level = systemController.getCurrentLevel();  // 
        const char* badgeText  = "DEV";  // level 
        uint16_t    badgeColor = UITheme::COLOR_PRIMARY;  // level 

        // [U7] textWidth    
        int16_t titleW  = tft.textWidth(title);
        int16_t badgeX  = SPACING_SM + titleW + BADGE_X_OFFSET;

        tft.fillRoundRect(badgeX, BADGE_Y_OFFSET, BADGE_WIDTH, BADGE_HEIGHT, 7, badgeColor);
        tft.setTextSize(1);
        tft.setTextColor(COLOR_BG_DARK);

        // [U7]     
        int16_t btw = tft.textWidth(badgeText);
        tft.setCursor(badgeX + (BADGE_WIDTH - btw) / 2, BADGE_Y_OFFSET + 3);
        tft.print(badgeText);
    }

    // [U12]      +  +  
#ifdef ENABLE_PREDICTIVE_MAINTENANCE
    float   healthScore = healthMonitor.getHealthScore();
    uint16_t hColor = (healthScore >= 90.0f) ? COLOR_SUCCESS
                    : (healthScore >= 75.0f) ? COLOR_WARNING
                    :                          COLOR_DANGER;

    //   ( )
    int16_t iconX = SCREEN_WIDTH - 62;
    int16_t iconY = 4;

    //   ( UTF-8: E2 99 A5)  H  +  
    tft.fillCircle(iconX, iconY + 8, 6, hColor);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_BG_DARK);
    tft.setCursor(iconX - 2, iconY + 5);
    tft.print("H");

    //  
    char hBuf[8];
    snprintf(hBuf, sizeof(hBuf), "%.0f%%", healthScore);
    tft.setTextSize(1);
    tft.setTextColor(hColor);

    // [U7] textWidth  
    int16_t hw = tft.textWidth(hBuf);
    tft.setCursor(SCREEN_WIDTH - SPACING_SM - hw, iconY + 5);
    tft.print(hBuf);

    //    ( )
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
//  
// ================================================================
void drawCard(const CardConfig& config) {
    using namespace UITheme;
    
    //   (elevated)
    if (config.elevated) {
        tft.fillRoundRect(config.x + 2, config.y + 2, 
                         config.w, config.h, 
                         CARD_RADIUS, COLOR_DIVIDER);
    }
    
    //  
    tft.fillRoundRect(config.x, config.y, config.w, config.h, 
                     CARD_RADIUS, config.bgColor);
    
    // 
    if (config.borderColor != config.bgColor) {
        tft.drawRoundRect(config.x, config.y, config.w, config.h, 
                         CARD_RADIUS, config.borderColor);
    }
}

// ================================================================
//   [U7]    
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

    // [U7] textWidth()    (strlen()*6 )
    int16_t tw = tft.textWidth(config.label);
    int16_t th = 8 * TEXT_SIZE_SMALL;  //  
    int16_t tx = config.x + (config.w - tw) / 2;
    int16_t ty = config.y + (config.h - th) / 2;

    tft.setCursor(tx, ty);
    tft.print(config.label);
}

// ================================================================
//   
// ================================================================
void drawValueDisplay(const ValueDisplayConfig& config) {
    using namespace UITheme;
    
    // 
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(config.x, config.y);
    tft.print(config.label);
    
    // 
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(config.valueColor);
    tft.setCursor(config.x, config.y + 12);
    tft.print(config.value);
    
    // 
    if (config.unit != nullptr) {
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.print(" ");
        tft.print(config.unit);
    }
}

// ================================================================
//    [U7] textWidth   
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
    // [U7] textWidth    (strlen()*6 )
    int16_t tw      = tft.textWidth(text);
    int16_t badgeW  = tw + SPACING_SM * 2;
    int16_t badgeH  = 18;

    tft.fillRoundRect(x, y, badgeW, badgeH, BADGE_RADIUS, bgColor);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(x + (badgeW - tw) / 2, y + 5);
    tft.print(text);
}

// ================================================================
//  
// ================================================================
void drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, 
                     float percentage, uint16_t color) {
    using namespace UITheme;
    
    //  
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;
    
    //  ()
    tft.fillRoundRect(x, y, w, h, 4, COLOR_BG_ELEVATED);
    
    //  
    int16_t fillW = (w - 4) * percentage / 100;
    if (fillW > 2) {
        tft.fillRoundRect(x + 2, y + 2, fillW, h - 4, 3, color);
    }
    
    // 
    tft.drawRoundRect(x, y, w, h, 4, COLOR_BORDER);
}

// ================================================================
//  - 
// ================================================================
void drawIconHome(int16_t x, int16_t y, uint16_t color) {
    // 
    tft.fillTriangle(x + 8, y, x, y + 6, x + 16, y + 6, color);
    
    // 
    tft.fillRect(x + 2, y + 6, 12, 10, color);
    
    // 
    tft.fillRect(x + 6, y + 10, 4, 6, UITheme::COLOR_BG_DARK);
}

// ================================================================
//  -  ()
// ================================================================
void drawIconSettings(int16_t x, int16_t y, uint16_t color) {
    //  
    tft.fillCircle(x + 8, y + 8, 3, color);
    
    //  (8)
    for (int i = 0; i < 8; i++) {
        float angle = i * PI / 4;
        int16_t x1 = x + 8 + cos(angle) * 6;
        int16_t y1 = y + 8 + sin(angle) * 6;
        tft.fillCircle(x1, y1, 2, color);
    }
}

// ================================================================
//  -  ()
// ================================================================
void drawIconBack(int16_t x, int16_t y, uint16_t color) {
    //  
    tft.fillTriangle(x, y + 8, x + 6, y + 2, x + 6, y + 14, color);
    
    //  
    tft.fillRect(x + 5, y + 6, 10, 4, color);
}

// ================================================================
//  -  ( + !)
// ================================================================
void drawIconWarning(int16_t x, int16_t y, uint16_t color) {
    // 
    tft.fillTriangle(x + 8, y, x, y + 16, x + 16, y + 16, color);
    
    //  (!)
    tft.fillRect(x + 7, y + 5, 2, 6, UITheme::COLOR_BG_DARK);
    tft.fillRect(x + 7, y + 13, 2, 2, UITheme::COLOR_BG_DARK);
}

// ================================================================
//  - 
// ================================================================
void drawIconCheck(int16_t x, int16_t y, uint16_t color) {
    //  
    tft.drawLine(x + 2, y + 8, x + 6, y + 12, color);
    tft.drawLine(x + 6, y + 12, x + 14, y + 2, color);
    
    // 
    tft.drawLine(x + 2, y + 9, x + 6, y + 13, color);
    tft.drawLine(x + 6, y + 13, x + 14, y + 3, color);
}

// ================================================================
// 
// ================================================================
void drawDivider(int16_t x, int16_t y, int16_t w) {
    tft.drawFastHLine(x, y, w, UITheme::COLOR_DIVIDER);
}

// ================================================================
// drawNavBar()  [U7]     
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
        drawButton(btn);  //   drawButton 
    }
}

} // namespace UIComponents
