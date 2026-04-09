// ================================================================
// UI_Screen_PID.cpp -  PID  
// ================================================================
#include "UIComponents.h"
#include "Config.h"
#include "PID_Control.h"
extern float pidOutput;
extern float pidError;

using namespace UIComponents;
using namespace UITheme;

//  PID 
static int8_t selectedPIDParam = -1;

void drawPIDScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    //   
    drawHeader("PID  ");
    
    //  PID   
    int16_t startY = HEADER_HEIGHT + SPACING_SM;
    int16_t cardH = 70;
    
    struct PIDParam {
        const char* label;
        const char* description;
        float* value;
        float minVal;
        float maxVal;
        float step;
        uint16_t color;
    };
    
    PIDParam params[] = {
        {"Kp ()", " ", &config.pidKp, 0.0f, 10.0f, 0.1f, COLOR_PRIMARY},
        {"Ki ()", " ", &config.pidKi, 0.0f, 5.0f, 0.05f, COLOR_ACCENT},
        {"Kd ()", " ", &config.pidKd, 0.0f, 5.0f, 0.1f, COLOR_INFO}
    };
    
    for (int i = 0; i < 3; i++) {
        int16_t y = startY + i * (cardH + SPACING_SM);
        bool isSelected = (selectedPIDParam == i);
        
        CardConfig paramCard = {
            .x = SPACING_SM,
            .y = y,
            .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
            .h = cardH,
            .bgColor = isSelected ? COLOR_BG_ELEVATED : COLOR_BG_CARD,
            .borderColor = isSelected ? params[i].color : COLOR_BORDER
        };
        drawCard(paramCard);
        
        //  
        tft.setTextSize(TEXT_SIZE_MEDIUM);
        tft.setTextColor(params[i].color);
        tft.setCursor(paramCard.x + CARD_PADDING, paramCard.y + CARD_PADDING);
        tft.print(params[i].label);
        
        // 
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(paramCard.x + CARD_PADDING, paramCard.y + CARD_PADDING + 20);
        tft.print(params[i].description);
        
        //  
        tft.setTextSize(3);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.setCursor(paramCard.x + CARD_PADDING, paramCard.y + CARD_PADDING + 35);
        tft.printf("%.2f", *params[i].value);
        
        //  
        if (!isSelected) {
            ButtonConfig editBtn = {
                .x = (int16_t)(paramCard.x + paramCard.w - 70),
                .y = (int16_t)(paramCard.y + CARD_PADDING + 20),
                .w = 60,
                .h = 28,
                .label = "",
                .style = BTN_SECONDARY,
                .enabled = true
            };
            drawButton(editBtn);
        }
    }
    
    //     
    if (selectedPIDParam >= 0) {
        int16_t panelY = startY + 3 * (cardH + SPACING_SM) + SPACING_SM;
        
        CardConfig editPanel = {
            .x = SPACING_SM,
            .y = panelY,
            .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
            .h = 55,
            .bgColor = COLOR_PRIMARY_DARK,
            .elevated = true
        };
        drawCard(editPanel);
        
        //   
        ButtonConfig minus10Btn = {
            .x = (int16_t)(editPanel.x + SPACING_SM),
            .y = (int16_t)(panelY + 8),
            .w = 50,
            .h = 38,
            .label = "--",
            .style = BTN_DANGER,
            .enabled = true
        };
        drawButton(minus10Btn);
        
        //   
        ButtonConfig minus1Btn = {
            .x = (int16_t)(editPanel.x + SPACING_SM + 55),
            .y = (int16_t)(panelY + 8),
            .w = 50,
            .h = 38,
            .label = "-",
            .style = BTN_SECONDARY,
            .enabled = true
        };
        drawButton(minus1Btn);
        
        //   
        tft.setTextSize(TEXT_SIZE_MEDIUM);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        char valueStr[16];
        snprintf(valueStr, sizeof(valueStr), "%.2f", *params[selectedPIDParam].value);
        int16_t textW = strlen(valueStr) * 12;
        tft.setCursor(editPanel.x + (editPanel.w - textW) / 2, panelY + 20);
        tft.print(valueStr);
        
        //   
        ButtonConfig plus1Btn = {
            .x = (int16_t)(editPanel.x + editPanel.w - 110),
            .y = (int16_t)(panelY + 8),
            .w = 50,
            .h = 38,
            .label = "+",
            .style = BTN_SECONDARY,
            .enabled = true
        };
        drawButton(plus1Btn);
        
        //   
        ButtonConfig plus10Btn = {
            .x = (int16_t)(editPanel.x + editPanel.w - 55),
            .y = (int16_t)(panelY + 8),
            .w = 50,
            .h = 38,
            .label = "++",
            .style = BTN_SUCCESS,
            .enabled = true
        };
        drawButton(plus10Btn);
    }
    
    //  PID  
    if (selectedPIDParam < 0) {
        int16_t previewY = startY + 3 * (cardH + SPACING_SM) + SPACING_SM;
        
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(SPACING_SM + 4, previewY);
        tft.print(" PID :");
        
        tft.setTextSize(TEXT_SIZE_MEDIUM);
        tft.setTextColor(COLOR_ACCENT);
        tft.setCursor(SPACING_SM + 4, previewY + 16);
        tft.printf("%.2f", pidOutput);
        
        // PID 
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(SCREEN_WIDTH / 2, previewY);
        tft.print(":");
        
        tft.setTextSize(TEXT_SIZE_MEDIUM);
        tft.setTextColor(pidError > 0 ? COLOR_WARNING : COLOR_SUCCESS);
        tft.setCursor(SCREEN_WIDTH / 2, previewY + 16);
        tft.printf("%.1f kPa", pidError);
    }
    
    //    
    if (selectedPIDParam >= 0) {
        NavButton navButtons[] = {
            {"", BTN_DANGER, true},
            {"", BTN_SUCCESS, true}
        };
        drawNavBar(navButtons, 2);
    } else {
        NavButton navButtons[] = {
            {"", BTN_OUTLINE, true},
            {"", BTN_SECONDARY, systemController.getPermissions().canChangeSettings},
            {"Auto", BTN_PRIMARY, false} //    
        };
        drawNavBar(navButtons, 3);
    }
}

void handlePIDTouch(uint16_t x, uint16_t y) {
    int16_t startY = HEADER_HEIGHT + SPACING_SM;
    int16_t cardH = 70;
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    
    struct PIDParam {
        float* value;
        float minVal;
        float maxVal;
        float step;
    };
    
    PIDParam params[] = {
        {&config.pidKp, 0.0f, 10.0f, 0.1f},
        {&config.pidKi, 0.0f, 5.0f, 0.05f},
        {&config.pidKd, 0.0f, 5.0f, 0.1f}
    };
    
    //   
    if (selectedPIDParam >= 0) {
        int16_t panelY = startY + 3 * (cardH + SPACING_SM) + SPACING_SM;
        
        // --  ( )
        ButtonConfig minus10Btn = {
            .x = (int16_t)(SPACING_SM + SPACING_SM),
            .y = (int16_t)(panelY + 8),
            .w = 50,
            .h = 38,
            .label = "--",
            .style = BTN_DANGER,
            .enabled = true
        };
        
        if (isButtonPressed(minus10Btn, x, y)) {
            float newVal = *params[selectedPIDParam].value - params[selectedPIDParam].step * 10;
            if (newVal >= params[selectedPIDParam].minVal) {
                *params[selectedPIDParam].value = newVal;
            }
            screenNeedsRedraw = true;
            return;
        }
        
        // -  ( )
        ButtonConfig minus1Btn = {
            .x = (int16_t)(SPACING_SM + SPACING_SM + 55),
            .y = (int16_t)(panelY + 8),
            .w = 50,
            .h = 38,
            .label = "-",
            .style = BTN_SECONDARY,
            .enabled = true
        };
        
        if (isButtonPressed(minus1Btn, x, y)) {
            float newVal = *params[selectedPIDParam].value - params[selectedPIDParam].step;
            if (newVal >= params[selectedPIDParam].minVal) {
                *params[selectedPIDParam].value = newVal;
            }
            screenNeedsRedraw = true;
            return;
        }
        
        // +  ( )
        ButtonConfig plus1Btn = {
            .x = (int16_t)(SCREEN_WIDTH - SPACING_SM - 110),
            .y = (int16_t)(panelY + 8),
            .w = 50,
            .h = 38,
            .label = "+",
            .style = BTN_SECONDARY,
            .enabled = true
        };
        
        if (isButtonPressed(plus1Btn, x, y)) {
            float newVal = *params[selectedPIDParam].value + params[selectedPIDParam].step;
            if (newVal <= params[selectedPIDParam].maxVal) {
                *params[selectedPIDParam].value = newVal;
            }
            screenNeedsRedraw = true;
            return;
        }
        
        // ++  ( )
        ButtonConfig plus10Btn = {
            .x = (int16_t)(SCREEN_WIDTH - SPACING_SM - 55),
            .y = (int16_t)(panelY + 8),
            .w = 50,
            .h = 38,
            .label = "++",
            .style = BTN_SUCCESS,
            .enabled = true
        };
        
        if (isButtonPressed(plus10Btn, x, y)) {
            float newVal = *params[selectedPIDParam].value + params[selectedPIDParam].step * 10;
            if (newVal <= params[selectedPIDParam].maxVal) {
                *params[selectedPIDParam].value = newVal;
            }
            screenNeedsRedraw = true;
            return;
        }
        
        //  (/)
        if (y >= navY) {
            int16_t buttonW = (SCREEN_WIDTH - SPACING_SM * 3) / 2;
            
            // 
            ButtonConfig cancelBtn = {
                .x = SPACING_SM,
                .y = (int16_t)(navY + 2),
                .w = buttonW,
                .h = (int16_t)(FOOTER_HEIGHT - 4),
                .label = "",
                .style = BTN_DANGER,
                .enabled = true
            };
            
            if (isButtonPressed(cancelBtn, x, y)) {
                // loadConfig();  // 
                selectedPIDParam = -1;
                screenNeedsRedraw = true;
                return;
            }
            
            // 
            ButtonConfig saveBtn = {
                .x = (int16_t)(SPACING_SM + buttonW + SPACING_SM),
                .y = (int16_t)(navY + 2),
                .w = buttonW,
                .h = (int16_t)(FOOTER_HEIGHT - 4),
                .label = "",
                .style = BTN_SUCCESS,
                .enabled = true
            };
            
            if (isButtonPressed(saveBtn, x, y)) {
                // saveConfig();  // 
                selectedPIDParam = -1;
                screenNeedsRedraw = true;
                return;
            }
        }
    }
    //   
    else {
        //     
        for (int i = 0; i < 3; i++) {
            int16_t cardY = startY + i * (cardH + SPACING_SM);
            
            ButtonConfig editBtn = {
                .x = (int16_t)(SCREEN_WIDTH - SPACING_SM - 70),
                .y = (int16_t)(cardY + CARD_PADDING + 20),
                .w = 60,
                .h = 28,
                .label = "",
                .style = BTN_SECONDARY,
                .enabled = true
            };
            
            if (isButtonPressed(editBtn, x, y)) {
                selectedPIDParam = i;
                screenNeedsRedraw = true;
                return;
            }
        }
        
        // 
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
            if (systemController.getPermissions().canChangeSettings) {
                ButtonConfig defaultBtn = {
                    .x = (int16_t)(SPACING_SM + buttonW + SPACING_SM),
                    .y = (int16_t)(navY + 2),
                    .w = buttonW,
                    .h = (int16_t)(FOOTER_HEIGHT - 4),
                    .label = "",
                    .style = BTN_SECONDARY,
                    .enabled = true
                };
                
                if (isButtonPressed(defaultBtn, x, y)) {
                    config.pidKp = PID_KP;
                    config.pidKi = PID_KI;
                    config.pidKd = PID_KD;
                    // saveConfig();  // 
                    screenNeedsRedraw = true;
                    return;
                }
            }
        }
    }
}