// ================================================================
// UI_Screen_StateDiagram.cpp -    
// UIComponents  
// ================================================================
#include "UIComponents.h"
#include "Config.h"
#include "StateMachine.h"

using namespace UIComponents;
using namespace UITheme;

// ================================================================
//  
// ================================================================
static uint8_t stateDiagramPage = 0;     // 0:  , 1:  
static int8_t selectedState = -1;        //   (-1: )

// ================================================================
//   
// ================================================================
struct StateNode {
    int16_t x, y;           // 
    int16_t w, h;           // 
    const char* label;      // 
    SystemState state;      //  enum
    uint16_t color;         // 
};

// ================================================================
// Page 0:   (IDLE  COMPLETE)
// ================================================================
static const StateNode PAGE0_NODES[] = {
    // Row 1: IDLE  VACUUM_ON  VACUUM_HOLD
    {60,  65, 90, 40, "IDLE",        STATE_IDLE,            COLOR_INFO},
    {180, 65, 90, 40, "VAC_ON",      STATE_IDLE,  COLOR_SUCCESS},
    {300, 65, 90, 40, "VAC_HOLD",    STATE_VACUUM_ON,       COLOR_SUCCESS},
    
    // Row 2: VACUUM_BREAK
    {420, 65, 90, 40, "HOLD",        STATE_VACUUM_HOLD,     COLOR_SUCCESS},
    
    // Row 3: WAIT_REMOVAL
    {420, 135, 90, 40, "BREAK",      STATE_VACUUM_BREAK,    COLOR_WARNING},
    {300, 135, 90, 40, "WAIT_REM",   STATE_WAIT_REMOVAL,    COLOR_SECONDARY},
    {180, 135, 90, 40, "COMPLETE",   STATE_COMPLETE,        COLOR_SUCCESS},
};

static const uint8_t PAGE0_NODE_COUNT = 7;

// ================================================================
// Page 1: / 
// ================================================================
static const StateNode PAGE1_NODES[] = {
    {100, 70,  110, 40, "WAIT_REM",   STATE_WAIT_REMOVAL,    COLOR_SECONDARY},
    {340, 70,  110, 40, "ERROR",      STATE_ERROR,           COLOR_DANGER},
    
    {220, 140, 120, 40, "EMERGENCY",  STATE_EMERGENCY_STOP,  COLOR_DANGER},
    
    {100, 210, 100, 40, "IDLE",       STATE_IDLE,            COLOR_INFO},
};

static const uint8_t PAGE1_NODE_COUNT = 4;

// ================================================================
//   
// ================================================================
void drawArrow(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color, bool dashed = false) {
    //  
    if (dashed) {
        // 
        int16_t dx = x2 - x1;
        int16_t dy = y2 - y1;
        int steps = 8;
        for (int i = 0; i < steps; i += 2) {
            int16_t sx = x1 + (dx * i) / steps;
            int16_t sy = y1 + (dy * i) / steps;
            int16_t ex = x1 + (dx * (i + 1)) / steps;
            int16_t ey = y1 + (dy * (i + 1)) / steps;
            tft.drawLine(sx, sy, ex, ey, color);
        }
    } else {
        // 
        tft.drawLine(x1, y1, x2, y2, color);
    }
    
    //  
    int16_t dx = x2 - x1;
    int16_t dy = y2 - y1;
    float angle = atan2(dy, dx);
    int16_t headLen = 6;
    
    int16_t ax1 = x2 - headLen * cos(angle - 0.4);
    int16_t ay1 = y2 - headLen * sin(angle - 0.4);
    int16_t ax2 = x2 - headLen * cos(angle + 0.4);
    int16_t ay2 = y2 - headLen * sin(angle + 0.4);
    
    tft.drawLine(x2, y2, ax1, ay1, color);
    tft.drawLine(x2, y2, ax2, ay2, color);
}

// ================================================================
//   
// ================================================================
void drawStateNode(const StateNode& node, bool isActive, bool isSelected) {
    CardConfig card = {
        .x = node.x,
        .y = node.y,
        .w = node.w,
        .h = node.h,
        .bgColor = COLOR_BG_CARD,
        .borderColor = isActive ? node.color : COLOR_BORDER,
        .elevated = isActive
    };
    
    //    
    if (isSelected) {
        tft.drawRoundRect(node.x - 2, node.y - 2, node.w + 4, node.h + 4, 
                          CARD_RADIUS, node.color);
    }
    
    drawCard(card);
    
    // 
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(isActive ? node.color : COLOR_TEXT_PRIMARY);
    
    int16_t textW = strlen(node.label) * 6;
    int16_t textX = node.x + (node.w - textW) / 2;
    int16_t textY = node.y + (node.h - 8) / 2;
    
    tft.setCursor(textX, textY);
    tft.print(node.label);
    
    //   
    if (isActive) {
        tft.fillCircle(node.x + node.w - 8, node.y + 8, 3, node.color);
    }
}

// ================================================================
//   
// ================================================================
void drawStateDiagramScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    //   
    drawHeader(" ");
    
    //     
    int16_t statusY = HEADER_HEIGHT + SPACING_SM;
    
    CardConfig statusCard = {
        .x = SPACING_SM,
        .y = statusY,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = 35,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(statusCard);
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(statusCard.x + CARD_PADDING, statusCard.y + CARD_PADDING);
    tft.print(" :");
    
    //  
    const char* stateName = getStateName(currentState);
    BadgeType badgeType;
    
    if (currentState == STATE_ERROR || currentState == STATE_EMERGENCY_STOP) {
        badgeType = BADGE_DANGER;
    } else if (currentState == STATE_IDLE) {
        badgeType = BADGE_INFO;
    } else {
        badgeType = BADGE_SUCCESS;
    }
    
    drawBadge(statusCard.x + 100, statusCard.y + CARD_PADDING, stateName, badgeType);
    
    //  
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(statusCard.x + statusCard.w - 50, statusCard.y + CARD_PADDING);
    tft.printf("Page %d/2", stateDiagramPage + 1);
    
    //    
    int16_t diagramY = statusCard.y + statusCard.h + SPACING_SM;
    int16_t diagramH = 170;
    
    // 
    tft.fillRect(0, diagramY, SCREEN_WIDTH, diagramH, COLOR_BG_DARK);
    
    //    
    const StateNode* nodes;
    uint8_t nodeCount;
    
    if (stateDiagramPage == 0) {
        nodes = PAGE0_NODES;
        nodeCount = PAGE0_NODE_COUNT;
        
        // Page 0 
        drawArrow(110, 85, 170, 85, COLOR_SUCCESS);          // IDLE  VAC_ON
        drawArrow(230, 85, 290, 85, COLOR_SUCCESS);          // VAC_ON  VAC_HOLD
        drawArrow(350, 85, 410, 85, COLOR_SUCCESS);          // VAC_HOLD  HOLD
        drawArrow(465, 105, 465, 125, COLOR_WARNING);        // HOLD  BREAK ()
        drawArrow(410, 155, 350, 155, COLOR_WARNING);        // BREAK  WAIT ()
        drawArrow(290, 155, 230, 155, COLOR_SECONDARY);      // WAIT  COMPLETE ()
        drawArrow(170, 155, 110, 155, COLOR_SUCCESS);        // COMPLETE  ()
        drawArrow(105, 105, 105, 125, COLOR_SUCCESS);        //  IDLE ()
        
    } else {
        nodes = PAGE1_NODES;
        nodeCount = PAGE1_NODE_COUNT;
        
        // Page 1  ()
        drawArrow(160, 90, 330, 90, COLOR_DANGER, true);     // WAIT  ERROR
        drawArrow(340, 110, 150, 200, COLOR_DANGER, true);   // ERROR  IDLE
        drawArrow(220, 180, 150, 210, COLOR_DANGER, true);   // EMERGENCY  IDLE
    }
    
    //  
    for (uint8_t i = 0; i < nodeCount; i++) {
        bool isActive = (nodes[i].state == currentState);
        bool isSelected = (nodes[i].state == (SystemState)selectedState);
        drawStateNode(nodes[i], isActive, isSelected);
    }
    
    //    
    int16_t infoY = diagramY + diagramH + SPACING_SM;
    
    CardConfig infoCard = {
        .x = SPACING_SM,
        .y = infoY,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = 50,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(infoCard);
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    
    if (selectedState >= 0) {
        //   
        tft.setTextColor(COLOR_PRIMARY);
        tft.setCursor(infoCard.x + CARD_PADDING, infoCard.y + CARD_PADDING);
        tft.print(getStateName((SystemState)selectedState));
        
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(infoCard.x + CARD_PADDING, infoCard.y + CARD_PADDING + 16);
        
        if (stateDiagramPage == 0) {
            tft.print("  ");
        } else {
            tft.print("/  ");
        }
        
    } else {
        //  
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(infoCard.x + CARD_PADDING, infoCard.y + CARD_PADDING);
        
        if (stateDiagramPage == 0) {
            tft.print("   ");
            tft.setCursor(infoCard.x + CARD_PADDING, infoCard.y + CARD_PADDING + 16);
            tft.print("     ");
        } else {
            tft.print("    ");
            tft.setCursor(infoCard.x + CARD_PADDING, infoCard.y + CARD_PADDING + 16);
            tft.print("    ");
        }
    }
    
    //    
    NavButton navButtons[] = {
        {"", BTN_SECONDARY, stateDiagramPage > 0},
        {"", BTN_SECONDARY, stateDiagramPage < 1},
        {"", BTN_OUTLINE, true}
    };
    drawNavBar(navButtons, 3);
}

// ================================================================
//  
// ================================================================
void handleStateDiagramTouch(uint16_t x, uint16_t y) {
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    
    //  
    if (y >= navY) {
        int16_t buttonW = (SCREEN_WIDTH - SPACING_SM * 4) / 3;
        
        //  
        if (stateDiagramPage > 0) {
            ButtonConfig prevBtn = {
                .x = SPACING_SM,
                .y = (int16_t)(navY + 2),
                .w = buttonW,
                .h = (int16_t)(FOOTER_HEIGHT - 4),
                .label = "",
                .style = BTN_SECONDARY,
                .enabled = true
            };
            
            if (isButtonPressed(prevBtn, x, y)) {
                stateDiagramPage--;
                selectedState = -1;
                screenNeedsRedraw = true;
                return;
            }
        }
        
        //  
        if (stateDiagramPage < 1) {
            ButtonConfig nextBtn = {
                .x = (int16_t)(SPACING_SM + buttonW + SPACING_SM),
                .y = (int16_t)(navY + 2),
                .w = buttonW,
                .h = (int16_t)(FOOTER_HEIGHT - 4),
                .label = "",
                .style = BTN_SECONDARY,
                .enabled = true
            };
            
            if (isButtonPressed(nextBtn, x, y)) {
                stateDiagramPage++;
                selectedState = -1;
                screenNeedsRedraw = true;
                return;
            }
        }
        
        // 
        ButtonConfig backBtn = {
            .x = (int16_t)(SPACING_SM + (buttonW + SPACING_SM) * 2),
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
    }
    
    //    
    const StateNode* nodes;
    uint8_t nodeCount;
    
    if (stateDiagramPage == 0) {
        nodes = PAGE0_NODES;
        nodeCount = PAGE0_NODE_COUNT;
    } else {
        nodes = PAGE1_NODES;
        nodeCount = PAGE1_NODE_COUNT;
    }
    
    for (uint8_t i = 0; i < nodeCount; i++) {
        if (x >= nodes[i].x && x <= nodes[i].x + nodes[i].w &&
            y >= nodes[i].y && y <= nodes[i].y + nodes[i].h) {
            
            //     
            if (selectedState == (int8_t)nodes[i].state) {
                selectedState = -1;
            } else {
                selectedState = (int8_t)nodes[i].state;
            }
            
            screenNeedsRedraw = true;
            return;
        }
    }
    
    //      
    if (selectedState >= 0) {
        selectedState = -1;
        screenNeedsRedraw = true;
    }
}

// ================================================================
//   (  )
// ================================================================
void drawStateDiagram() {
    drawStateDiagramScreen();
}

void handleStateDiagramTouch(uint16_t x, uint16_t y);  //  
