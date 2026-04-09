// ================================================================
// UI_Screen_Help.cpp -   
// ================================================================
#include "UIComponents.h"
#include "Config.h"

using namespace UIComponents;using namespace UITheme;

//  
extern uint8_t helpPageIndex;

void drawHelpScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    //   
    drawHeader("");
    
    //    
    int16_t startY = HEADER_HEIGHT + SPACING_SM;
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    int16_t pageTextX = (SCREEN_WIDTH - 60) / 2;
    tft.setCursor(pageTextX, startY);
    tft.printf(" %d / 5", helpPageIndex + 1);
    
    //    
    int16_t contentY = startY + 25;
    
    CardConfig contentCard = {
        .x = SPACING_SM,
        .y = contentY,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = 195,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(contentCard);
    
    int16_t textY = contentCard.y + CARD_PADDING;
    int16_t lineH = 18;
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    
    switch (helpPageIndex) {
        case 0: //  
            tft.setTextColor(COLOR_PRIMARY);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print(" ");
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            textY += lineH + 4;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("1.    ");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("2.     ");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("3.   ");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("4.     ");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("5.   ");
            textY += lineH * 2;
            
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("  ");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("  ");
            break;
            
        case 1: //  
            tft.setTextColor(COLOR_DANGER);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print(" ");
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            textY += lineH + 4;
            
            drawIconWarning(contentCard.x + CARD_PADDING, textY, COLOR_WARNING);
            tft.setCursor(contentCard.x + CARD_PADDING + 20, textY);
            tft.print(" 70C    ");
            textY += lineH;
            
            drawIconWarning(contentCard.x + CARD_PADDING, textY, COLOR_WARNING);
            tft.setCursor(contentCard.x + CARD_PADDING + 20, textY);
            tft.print(" 6A   ");
            textY += lineH;
            
            drawIconWarning(contentCard.x + CARD_PADDING, textY, COLOR_WARNING);
            tft.setCursor(contentCard.x + CARD_PADDING + 20, textY);
            tft.print("   ");
            textY += lineH * 2;
            
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("  ");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print(" ");
            break;
            
        case 2: //  
            tft.setTextColor(COLOR_MANAGER);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print(" ");
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            textY += lineH + 4;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print(" :");
            textY += lineH;
            
            tft.setTextColor(COLOR_ACCENT);
            tft.setCursor(contentCard.x + CARD_PADDING + 10, textY);
            tft.print("> manager");
            textY += lineH;
            
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("   ");
            textY += lineH * 2;
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print(" :");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("-  ");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("-  ");
            break;
            
        case 3: //  
            tft.setTextColor(COLOR_INFO);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print(" ");
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            textY += lineH + 4;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("  :");
            textY += lineH;
            
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentCard.x + CARD_PADDING + 10, textY);
            tft.print("-   ");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING + 10, textY);
            tft.print("-   ");
            textY += lineH * 2;
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("  :");
            textY += lineH;
            
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentCard.x + CARD_PADDING + 10, textY);
            tft.print("-   ");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING + 10, textY);
            tft.print("-   ");
            break;
            
        case 4: //  
            tft.setTextColor(COLOR_ACCENT);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print(" ");
            
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            textY += lineH + 4;
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("USB     ");
            textY += lineH + 4;
            
            //  
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            int16_t col1X = contentCard.x + CARD_PADDING;
            
            tft.setCursor(col1X, textY);
            tft.print("0:  ");
            textY += lineH;
            
            tft.setCursor(col1X, textY);
            tft.print("1: ");
            textY += lineH;
            
            tft.setCursor(col1X, textY);
            tft.print("2: ");
            textY += lineH;
            
            tft.setCursor(col1X, textY);
            tft.print("3:  ");
            textY += lineH;
            
            tft.setCursor(col1X, textY);
            tft.print("4:  ");
            textY += lineH;
            
            //  
            int16_t col2X = contentCard.x + CARD_PADDING + 140;
            textY = contentCard.y + CARD_PADDING + lineH * 2 + 4;
            
            tft.setCursor(col2X, textY);
            tft.print("5: ");
            textY += lineH;
            
            tft.setCursor(col2X, textY);
            tft.print("*: ");
            textY += lineH;
            
            tft.setCursor(col2X, textY);
            tft.print("ESC: ");
            textY += lineH;
            
            tft.setCursor(col2X, textY);
            tft.print(": ");
            textY += lineH;
            
            tft.setCursor(col2X, textY);
            tft.print("+/-: ");
            break;
    }
    
    //    
    NavButton navButtons[3];
    int navCount = 0;
    
    if (helpPageIndex > 0) {
        navButtons[navCount++] = {"", BTN_SECONDARY, true};
    } else {
        navButtons[navCount++] = {"", BTN_OUTLINE, true};
    }
    
    if (helpPageIndex < 4) {
        navButtons[navCount++] = {"", BTN_PRIMARY, true};
    }
    
    drawNavBar(navButtons, navCount);
}

void handleHelpTouch(uint16_t x, uint16_t y) {
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    
    if (y >= navY) {
        int buttonCount = (helpPageIndex > 0 && helpPageIndex < 4) ? 2 : 1;
        int16_t buttonW = buttonCount == 2 
                          ? (SCREEN_WIDTH - SPACING_SM * 3) / 2
                          : (SCREEN_WIDTH - SPACING_SM * 2);
        
        //    (  )
        ButtonConfig firstBtn = {
            .x = SPACING_SM,
            .y = (int16_t)(navY + 2),
            .w = buttonW,
            .h = (int16_t)(FOOTER_HEIGHT - 4),
            .label = (helpPageIndex > 0) ? "" : "",
            .style = (helpPageIndex > 0) ? BTN_SECONDARY : BTN_OUTLINE,
            .enabled = true
        };
        
        if (isButtonPressed(firstBtn, x, y)) {
            if (helpPageIndex > 0) {
                helpPageIndex--;
                screenNeedsRedraw = true;
            } else {
                currentScreen = SCREEN_SETTINGS;
                screenNeedsRedraw = true;
            }
            return;
        }
        
        //    ()
        if (helpPageIndex < 3 && buttonCount == 2) {
            ButtonConfig nextBtn = {
                .x = (int16_t)(SPACING_SM + buttonW + SPACING_SM),
                .y = (int16_t)(navY + 2),
                .w = buttonW,
                .h = (int16_t)(FOOTER_HEIGHT - 4),
                .label = "",
                .style = BTN_PRIMARY,
                .enabled = true
            };
            
            if (isButtonPressed(nextBtn, x, y)) {
                helpPageIndex++;
                screenNeedsRedraw = true;
                return;
            }
        }
    }
}