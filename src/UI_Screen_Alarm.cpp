// ================================================================
// UI_Screen_Alarm.cpp -   
// [U6]   
// [U7] strlen()*6  tft.textWidth() 
// [U8] screenNeedsRedraw/currentScreen  uiManager 
// ================================================================
#include "UIComponents.h"
#include "UITheme.h"
#include "UIManager.h"
#include "Config.h"
#include "ErrorHandler.h"

using namespace UIComponents;
using namespace UITheme;

extern TFT_GFX tft;
extern UIManager        uiManager;
extern bool             errorActive;
extern ErrorInfo        currentError;
extern ErrorInfo        errorHistory[];
extern uint8_t          errorHistIdx;
extern uint8_t          errorHistCnt;

// ================================================================
// [U6]     
// ================================================================
struct ActionGuide {
    uint16_t codeMin;
    uint16_t codeMax;
    const char* step1;
    const char* step2;
    const char* step3;
};

static const ActionGuide ACTION_GUIDES[] = {
    //   (E100~E199)
    {100, 149, "  ",      "  ",     "  "},
    {150, 199, "  ",         "  ",     "   "},
    //   (E200~E299)
    {200, 249, "  ",        "  ",   "  "},
    {250, 299, "  ",        "   ",  "  "},
    //   (E300~E399)
    {300, 399, "  ",    "MQTT  ",   "ESP  "},
    //   (E400~E499)
    {400, 499, "  ",        "  ",     " "},
    // 
    {0,   999, "  ",      " ",    " "},
};

static const ActionGuide* findGuide(uint16_t code) {
    for (auto& g : ACTION_GUIDES) {
        if (code >= g.codeMin && code <= g.codeMax) return &g;
    }
    return &ACTION_GUIDES[sizeof(ACTION_GUIDES) / sizeof(ACTION_GUIDES[0]) - 1];
}

// ================================================================
//  
// ================================================================
namespace AlarmLayout {
    constexpr int16_t STATUS_CARD_Y  = HEADER_HEIGHT + SPACING_SM;
    constexpr int16_t STATUS_CARD_H  = 68;

    constexpr int16_t ACTION_CARD_Y  = STATUS_CARD_Y + STATUS_CARD_H + SPACING_SM;
    constexpr int16_t ACTION_CARD_H  = 76;  // [U6]   

    constexpr int16_t HIST_LABEL_Y   = ACTION_CARD_Y + ACTION_CARD_H + SPACING_XS;
    constexpr int16_t HIST_ITEM_H    = 38;
    constexpr int16_t HIST_ITEM_GAP  = 4;
    constexpr uint8_t HIST_MAX_SHOW  = 3;   // 3    
}

// ================================================================
//   
// ================================================================
void drawAlarmScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    drawHeader(" / ");

    //     
    {
        CardConfig card = {
            .x = SPACING_SM,
            .y = AlarmLayout::STATUS_CARD_Y,
            .w = SCREEN_WIDTH - SPACING_SM * 2,
            .h = AlarmLayout::STATUS_CARD_H,
            .bgColor  = errorActive ? COLOR_DANGER : COLOR_SUCCESS,
            .elevated = true
        };
        drawCard(card);

        if (errorActive) {
            drawIconWarning(card.x + CARD_PADDING, card.y + 14, COLOR_TEXT_PRIMARY);

            //  
            tft.setTextSize(TEXT_SIZE_MEDIUM);
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setCursor(card.x + CARD_PADDING + 28, card.y + CARD_PADDING);
            tft.print(" !");

            //   (   )
            tft.setTextSize(TEXT_SIZE_SMALL);
            tft.setCursor(card.x + CARD_PADDING + 28, card.y + CARD_PADDING + 22);
            char shortMsg[40];
            strncpy(shortMsg, currentError.message, sizeof(shortMsg) - 1);
            shortMsg[sizeof(shortMsg) - 1] = '\0';
            tft.print(shortMsg);

            //   +  
            char codeBuf[8];
            snprintf(codeBuf, sizeof(codeBuf), "E%03d", currentError.code);
            drawBadge(card.x + card.w - CARD_PADDING - 40,
                      card.y + CARD_PADDING,
                      codeBuf,
                      (currentError.severity >= SEVERITY_CRITICAL)
                          ? BADGE_DANGER : BADGE_WARNING);

            //  
            SystemPermissions perms = systemController.getPermissions();
            if (perms.canReset) {
                ButtonConfig clearBtn = {
                    .x = (int16_t)(card.x + card.w - 76),
                    .y = (int16_t)(card.y + CARD_PADDING + 26),
                    .w = 66,
                    .h = 26,
                    .label = "",
                    .style = BTN_OUTLINE,
                    .enabled = true
                };
                drawButton(clearBtn);
            }
        } else {
            drawIconCheck(card.x + CARD_PADDING, card.y + 14, COLOR_TEXT_PRIMARY);
            tft.setTextSize(TEXT_SIZE_MEDIUM);
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setCursor(card.x + CARD_PADDING + 28, card.y + CARD_PADDING);
            tft.print("  ");
            tft.setTextSize(TEXT_SIZE_SMALL);
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setCursor(card.x + CARD_PADDING + 28, card.y + CARD_PADDING + 22);
            tft.print("  ");
        }
    }

    //  [U6]    (  ) 
    if (errorActive) {
        const ActionGuide* guide = findGuide(currentError.code);

        CardConfig card = {
            .x = SPACING_SM,
            .y = AlarmLayout::ACTION_CARD_Y,
            .w = SCREEN_WIDTH - SPACING_SM * 2,
            .h = AlarmLayout::ACTION_CARD_H,
            .bgColor = COLOR_BG_CARD,
            .borderColor = COLOR_WARNING
        };
        drawCard(card);

        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_WARNING);
        tft.setCursor(card.x + CARD_PADDING, card.y + CARD_PADDING);
        tft.print("  ");

        const char* steps[] = {guide->step1, guide->step2, guide->step3};
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        for (uint8_t i = 0; i < 3; i++) {
            tft.setCursor(card.x + CARD_PADDING + 8,
                          card.y + CARD_PADDING + 16 + i * 18);
            char stepBuf[48];
            snprintf(stepBuf, sizeof(stepBuf), "%d. %s", i + 1, steps[i]);
            tft.print(stepBuf);
        }
    }

    //    
    {
        int16_t labelY = AlarmLayout::HIST_LABEL_Y;
        if (!errorActive) {
            //       
            labelY = AlarmLayout::ACTION_CARD_Y;
        }

        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(SPACING_SM, labelY);
        char histTitle[24];
        snprintf(histTitle, sizeof(histTitle), " (%u)", errorHistCnt);
        tft.print(histTitle);

        int16_t listY = labelY + 18;
        uint8_t showCnt = (errorHistCnt < AlarmLayout::HIST_MAX_SHOW)
                           ? errorHistCnt
                           : AlarmLayout::HIST_MAX_SHOW;

        for (uint8_t i = 0; i < showCnt; i++) {
            int16_t iy = listY + i * (AlarmLayout::HIST_ITEM_H
                                      + AlarmLayout::HIST_ITEM_GAP);

            ErrorInfo* err = &errorHistory[
                (errorHistIdx - 1 - i + ERROR_HIST_MAX) % ERROR_HIST_MAX];

            CardConfig hCard = {
                .x = SPACING_SM,
                .y = iy,
                .w = SCREEN_WIDTH - SPACING_SM * 2,
                .h = AlarmLayout::HIST_ITEM_H,
                .bgColor = COLOR_BG_CARD
            };
            drawCard(hCard);

            //   + 
            uint16_t ec = (err->severity >= SEVERITY_CRITICAL)
                          ? COLOR_DANGER
                          : (err->severity >= SEVERITY_RECOVERABLE)
                            ? COLOR_WARNING : COLOR_INFO;
            tft.setTextSize(TEXT_SIZE_SMALL);
            tft.setTextColor(ec);
            tft.setCursor(hCard.x + CARD_PADDING, hCard.y + CARD_PADDING);
            char codeBuf[8];
            snprintf(codeBuf, sizeof(codeBuf), "E%03d", err->code);
            tft.print(codeBuf);

            //  ( 28)
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setCursor(hCard.x + CARD_PADDING + 44, hCard.y + CARD_PADDING);
            char msgBuf[29];
            strncpy(msgBuf, err->message, 28);
            msgBuf[28] = '\0';
            tft.print(msgBuf);

            //    [U7]   textWidth() 
            uint32_t elapsed = (millis() - err->timestamp) / 1000;
            char timeBuf[16];
            if (elapsed < 60)
                snprintf(timeBuf, sizeof(timeBuf), "%lu ", elapsed);
            else if (elapsed < 3600)
                snprintf(timeBuf, sizeof(timeBuf), "%lu ", elapsed / 60);
            else
                snprintf(timeBuf, sizeof(timeBuf), "%lu ", elapsed / 3600);

            tft.setTextSize(1);
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            int16_t tw = tft.textWidth(timeBuf);           // [U7]
            tft.setCursor(hCard.x + hCard.w - CARD_PADDING - tw,
                          hCard.y + CARD_PADDING + 16);
            tft.print(timeBuf);
        }

        if (errorHistCnt == 0) {
            tft.setTextSize(TEXT_SIZE_SMALL);
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            const char* noHist = " ";
            // [U7] textWidth() 
            int16_t nx = (SCREEN_WIDTH - tft.textWidth(noHist)) / 2;
            tft.setCursor(nx, listY + 24);
            tft.print(noHist);
        }
    }

    //    
    SystemPermissions perms = systemController.getPermissions();
    bool canClearAll = perms.canChangeSettings && errorHistCnt > 0;

    if (canClearAll) {
        NavButton nav[] = {
            {"",     BTN_OUTLINE, true},
            {"", BTN_DANGER,  true}
        };
        drawNavBar(nav, 2);
    } else {
        NavButton nav[] = {{"", BTN_OUTLINE, true}};
        drawNavBar(nav, 1);
    }
}

// ================================================================
//    [U8]
// ================================================================
void handleAlarmTouch(uint16_t x, uint16_t y) {
    uiManager.updateActivity();

    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;

    //   (  +   )
    if (errorActive && systemController.getPermissions().canReset) {
        int16_t cx = SPACING_SM + (SCREEN_WIDTH - SPACING_SM * 2) - 76;
        int16_t cy = AlarmLayout::STATUS_CARD_Y + CARD_PADDING + 26;
        if (x >= cx && x <= cx + 66 && y >= cy && y <= cy + 26) {
            clearError();
            uiManager.requestRedraw();
            return;
        }
    }

    // 
    if (y >= navY) {
        SystemPermissions perms = systemController.getPermissions();
        int16_t bw = (SCREEN_WIDTH - SPACING_SM * 3) / 2;

        // 
        if (x < SPACING_SM + bw) {
            uiManager.setScreen(SCREEN_MAIN);   // [U8]
            return;
        }

        //  
        if (perms.canChangeSettings && errorHistCnt > 0 &&
            x >= SPACING_SM + bw + SPACING_SM) {
            errorHistCnt = 0;
            errorHistIdx = 0;
            uiManager.showToast(" ", COLOR_INFO);
            uiManager.requestRedraw();
            return;
        }
    }
}
