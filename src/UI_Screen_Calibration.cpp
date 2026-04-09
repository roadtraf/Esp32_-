// ================================================================
// UI_Screen_Calibration.cpp -   
// ================================================================
#include "UIComponents.h"
#include "Config.h"
#include "SystemController.h"

// FreeRTOS (delay )
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

using namespace UIComponents;
using namespace UITheme;

// ================================================================
//  
// ================================================================
void drawCalibrationScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    //   
    drawHeader("");
    
    //  
    if (!systemController.getPermissions().canCalibrate) {
        //   
        int16_t msgY = SCREEN_HEIGHT / 2 - 40;
        
        drawIconWarning(SCREEN_WIDTH / 2 - 8, msgY, COLOR_WARNING);
        
        tft.setTextSize(TEXT_SIZE_MEDIUM);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.setCursor(120, msgY + 30);
        tft.print(" ");
        
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(80, msgY + 55);
        tft.print("  ");
        
        //  
        NavButton navButtons[] = {
            {"", BTN_OUTLINE, true}
        };
        drawNavBar(navButtons, 1);
        return;
    }
    
    //    
    int16_t startY = HEADER_HEIGHT + SPACING_MD;
    int16_t cardH = 70;
    
    //  
    CardConfig pressureCard = {
        .x = SPACING_SM,
        .y = startY,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = cardH,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(pressureCard);
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(pressureCard.x + CARD_PADDING, pressureCard.y + CARD_PADDING);
    tft.print("1.  ");
    
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(pressureCard.x + CARD_PADDING, pressureCard.y + CARD_PADDING + 18);
    tft.print(" :");
    
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(COLOR_PRIMARY);
    tft.setCursor(pressureCard.x + CARD_PADDING + 90, pressureCard.y + CARD_PADDING + 15);
    tft.printf("%.2f kPa", 0.0f  /* pressureOffset */);
    
    //  
    ButtonConfig pressureBtn = {
        .x = (int16_t)(pressureCard.x + pressureCard.w - 90),
        .y = (int16_t)(pressureCard.y + CARD_PADDING + 10),
        .w = 80,
        .h = 28,
        .label = "",
        .style = BTN_PRIMARY,
        .enabled = true
    };
    drawButton(pressureBtn);
    
    //  
    CardConfig currentCard = {
        .x = SPACING_SM,
        .y = (int16_t)(startY + cardH + SPACING_SM),
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = cardH,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(currentCard);
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(currentCard.x + CARD_PADDING, currentCard.y + CARD_PADDING);
    tft.print("2.  ");
    
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(currentCard.x + CARD_PADDING, currentCard.y + CARD_PADDING + 18);
    tft.print(" :");
    
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(COLOR_ACCENT);
    tft.setCursor(currentCard.x + CARD_PADDING + 90, currentCard.y + CARD_PADDING + 15);
    tft.printf("%.3f A", 0.0f  /* currentOffset */);
    
    ButtonConfig currentBtn = {
        .x = (int16_t)(currentCard.x + currentCard.w - 90),
        .y = (int16_t)(currentCard.y + CARD_PADDING + 10),
        .w = 80,
        .h = 28,
        .label = "",
        .style = BTN_PRIMARY,
        .enabled = true
    };
    drawButton(currentBtn);
    
    //  
    CardConfig tempCard = {
        .x = SPACING_SM,
        .y = (int16_t)(startY + (cardH + SPACING_SM) * 2),
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = cardH,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(tempCard);
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(tempCard.x + CARD_PADDING, tempCard.y + CARD_PADDING);
    tft.print("3.   (DS18B20)");
    
    if (isTemperatureSensorConnected()) {
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(tempCard.x + CARD_PADDING, tempCard.y + CARD_PADDING + 18);
        tft.print("  ");
        
        ButtonConfig tempBtn = {
            .x = (int16_t)(tempCard.x + tempCard.w - 90),
            .y = (int16_t)(tempCard.y + CARD_PADDING + 10),
            .w = 80,
            .h = 28,
            .label = "",
            .style = BTN_SECONDARY,
            .enabled = true
        };
        drawButton(tempBtn);
    } else {
        tft.setTextColor(COLOR_DANGER);
        tft.setCursor(tempCard.x + CARD_PADDING, tempCard.y + CARD_PADDING + 18);
        tft.print("   ");
    }
    
    //  
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(SPACING_SM, startY + (cardH + SPACING_SM) * 3 + 10);
    tft.print("    ");
    
    //    
    NavButton navButtons[] = {
        {"", BTN_OUTLINE, true}
    };
    drawNavBar(navButtons, 1);
}

// ================================================================
//   
// ================================================================
void handleCalibrationTouch(uint16_t x, uint16_t y) {
    //  
    if (!systemController.getPermissions().canCalibrate) {
        int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
        
        if (y >= navY) {
            currentScreen = SCREEN_SETTINGS;
            screenNeedsRedraw = true;
        }
        return;
    }
    
    //    
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
            currentScreen = SCREEN_SETTINGS;
            screenNeedsRedraw = true;
            return;
        }
    }
    
    //    
    int16_t startY = HEADER_HEIGHT + SPACING_MD;
    int16_t cardH = 70;
    
    //   
    ButtonConfig pressureBtn = {
        .x = (int16_t)(SCREEN_WIDTH - SPACING_SM - 90),
        .y = (int16_t)(startY + CARD_PADDING + 10),
        .w = 80,
        .h = 28,
        .label = "",
        .style = BTN_PRIMARY,
        .enabled = true
    };
    
    if (isButtonPressed(pressureBtn, x, y)) {
        calibratePressure();
        screenNeedsRedraw = true;
        return;
    }
    
    //   
    ButtonConfig currentBtn = {
        .x = (int16_t)(SCREEN_WIDTH - SPACING_SM - 90),
        .y = (int16_t)(startY + (cardH + SPACING_SM) + CARD_PADDING + 10),
        .w = 80,
        .h = 28,
        .label = "",
        .style = BTN_PRIMARY,
        .enabled = true
    };
    
    if (isButtonPressed(currentBtn, x, y)) {
        calibrateCurrent();
        screenNeedsRedraw = true;
        return;
    }
    
    //    
    if (isTemperatureSensorConnected()) {
        ButtonConfig tempBtn = {
            .x = (int16_t)(SCREEN_WIDTH - SPACING_SM - 90),
            .y = (int16_t)(startY + (cardH + SPACING_SM) * 2 + CARD_PADDING + 10),
            .w = 80,
            .h = 28,
            .label = "",
            .style = BTN_SECONDARY,
            .enabled = true
        };
        
        if (isButtonPressed(tempBtn, x, y)) {
            //    
            // showTemperatureSensorInfo();  // 
            return;
        }
    }
}

// ================================================================
//    
// ================================================================
void showTemperatureSensorInfo() {
    int16_t popupW = 300;
    int16_t popupH = 160;
    int16_t popupX = (SCREEN_WIDTH - popupW) / 2;
    int16_t popupY = (SCREEN_HEIGHT - popupH) / 2;
    
    // 
    tft.fillScreen(0x18E3);
    
    CardConfig popup = {
        .x = popupX,
        .y = popupY,
        .w = popupW,
        .h = popupH,
        .bgColor = COLOR_BG_CARD,
        .borderColor = COLOR_PRIMARY,
        .elevated = true
    };
    drawCard(popup);
    
    // 
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(COLOR_PRIMARY);
    tft.setCursor(popupX + CARD_PADDING, popupY + CARD_PADDING);
    tft.print("DS18B20  ");
    
    // 
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    
    int16_t infoY = popupY + CARD_PADDING + 30;
    tft.setCursor(popupX + CARD_PADDING, infoY);
    tft.printf(" : %d", getTemperatureSensorCount());
    
    tft.setCursor(popupX + CARD_PADDING, infoY + 20);
    tft.print(": 12 (0.0625C)");
    
    tft.setCursor(popupX + CARD_PADDING, infoY + 40);
    tft.print(": 0.5C");
    
    tft.setCursor(popupX + CARD_PADDING, infoY + 60);
    tft.print("  ");
    
    //  
    ButtonConfig closeBtn = {
        .x = (int16_t)(popupX + (popupW - 100) / 2),
        .y = (int16_t)(popupY + popupH - 35),
        .w = 100,
        .h = 28,
        .label = "",
        .style = BTN_PRIMARY,
        .enabled = true
    };
    drawButton(closeBtn);
    
    // [R4] : 3     
    Serial.println(" ");
}