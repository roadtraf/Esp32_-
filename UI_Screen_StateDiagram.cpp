// ================================================================
// UI_Screen_StateDiagram.cpp - 재설계 상태 다이어그램 화면
// UIComponents 기반으로 재작성
// ================================================================
#include "../include/UIComponents.h"
#include "../include/Config.h"
#include "../include/StateMachine.h"

using namespace UIComponents;
using namespace UITheme;

// ================================================================
// 페이지 상태
// ================================================================
static uint8_t stateDiagramPage = 0;     // 0: 정상 사이클, 1: 에러 경로
static int8_t selectedState = -1;        // 선택된 상태 (-1: 없음)

// ================================================================
// 상태 블록 구조체
// ================================================================
struct StateNode {
    int16_t x, y;           // 위치
    int16_t w, h;           // 크기
    const char* label;      // 라벨
    SystemState state;      // 상태 enum
    uint16_t color;         // 색상
};

// ================================================================
// Page 0: 정상 사이클 (IDLE → COMPLETE)
// ================================================================
static const StateNode PAGE0_NODES[] = {
    // Row 1: IDLE → VACUUM_ON → VACUUM_HOLD
    {60,  65, 90, 40, "IDLE",        STATE_IDLE,            COLOR_INFO},
    {180, 65, 90, 40, "VAC_ON",      STATE_IDLE_TO_VACUUM,  COLOR_SUCCESS},
    {300, 65, 90, 40, "VAC_HOLD",    STATE_VACUUMING,       COLOR_SUCCESS},
    
    // Row 2: VACUUM_BREAK
    {420, 65, 90, 40, "HOLD",        STATE_VACUUM_HOLD,     COLOR_SUCCESS},
    
    // Row 3: WAIT_REMOVAL
    {420, 135, 90, 40, "BREAK",      STATE_VACUUM_BREAK,    COLOR_WARNING},
    {300, 135, 90, 40, "WAIT_REM",   STATE_WAIT_REMOVAL,    COLOR_SECONDARY},
    {180, 135, 90, 40, "COMPLETE",   STATE_COMPLETE,        COLOR_SUCCESS},
};

static const uint8_t PAGE0_NODE_COUNT = 7;

// ================================================================
// Page 1: 에러/비상 경로
// ================================================================
static const StateNode PAGE1_NODES[] = {
    {100, 70,  110, 40, "WAIT_REM",   STATE_WAIT_REMOVAL,    COLOR_SECONDARY},
    {340, 70,  110, 40, "ERROR",      STATE_ERROR,           COLOR_DANGER},
    
    {220, 140, 120, 40, "EMERGENCY",  STATE_EMERGENCY_STOP,  COLOR_DANGER},
    
    {100, 210, 100, 40, "IDLE",       STATE_IDLE,            COLOR_INFO},
};

static const uint8_t PAGE1_NODE_COUNT = 4;

// ================================================================
// 화살표 그리기 헬퍼
// ================================================================
void drawArrow(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color, bool dashed = false) {
    // 화살표 선
    if (dashed) {
        // 점선
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
        // 실선
        tft.drawLine(x1, y1, x2, y2, color);
    }
    
    // 화살표 머리
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
// 상태 노드 그리기
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
    
    // 선택된 노드는 이중 테두리
    if (isSelected) {
        tft.drawRoundRect(node.x - 2, node.y - 2, node.w + 4, node.h + 4, 
                          CARD_RADIUS, node.color);
    }
    
    drawCard(card);
    
    // 라벨
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(isActive ? node.color : COLOR_TEXT_PRIMARY);
    
    int16_t textW = strlen(node.label) * 6;
    int16_t textX = node.x + (node.w - textW) / 2;
    int16_t textY = node.y + (node.h - 8) / 2;
    
    tft.setCursor(textX, textY);
    tft.print(node.label);
    
    // 활성 상태 표시
    if (isActive) {
        tft.fillCircle(node.x + node.w - 8, node.y + 8, 3, node.color);
    }
}

// ================================================================
// 메인 화면 그리기
// ================================================================
void drawStateDiagramScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    // ── 헤더 ──
    drawHeader("상태 다이어그램");
    
    // ── 현재 상태 표시 ──
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
    tft.print("현재 상태:");
    
    // 상태 배지
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
    
    // 페이지 표시
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(statusCard.x + statusCard.w - 50, statusCard.y + CARD_PADDING);
    tft.printf("Page %d/2", stateDiagramPage + 1);
    
    // ── 다이어그램 영역 ──
    int16_t diagramY = statusCard.y + statusCard.h + SPACING_SM;
    int16_t diagramH = 170;
    
    // 배경
    tft.fillRect(0, diagramY, SCREEN_WIDTH, diagramH, COLOR_BG_DARK);
    
    // 노드 및 화살표 그리기
    const StateNode* nodes;
    uint8_t nodeCount;
    
    if (stateDiagramPage == 0) {
        nodes = PAGE0_NODES;
        nodeCount = PAGE0_NODE_COUNT;
        
        // Page 0 화살표
        drawArrow(110, 85, 170, 85, COLOR_SUCCESS);          // IDLE → VAC_ON
        drawArrow(230, 85, 290, 85, COLOR_SUCCESS);          // VAC_ON → VAC_HOLD
        drawArrow(350, 85, 410, 85, COLOR_SUCCESS);          // VAC_HOLD → HOLD
        drawArrow(465, 105, 465, 125, COLOR_WARNING);        // HOLD → BREAK (하)
        drawArrow(410, 155, 350, 155, COLOR_WARNING);        // BREAK → WAIT (좌)
        drawArrow(290, 155, 230, 155, COLOR_SECONDARY);      // WAIT → COMPLETE (좌)
        drawArrow(170, 155, 110, 155, COLOR_SUCCESS);        // COMPLETE → (좌)
        drawArrow(105, 105, 105, 125, COLOR_SUCCESS);        // → IDLE (상)
        
    } else {
        nodes = PAGE1_NODES;
        nodeCount = PAGE1_NODE_COUNT;
        
        // Page 1 화살표 (점선)
        drawArrow(160, 90, 330, 90, COLOR_DANGER, true);     // WAIT → ERROR
        drawArrow(340, 110, 150, 200, COLOR_DANGER, true);   // ERROR → IDLE
        drawArrow(220, 180, 150, 210, COLOR_DANGER, true);   // EMERGENCY → IDLE
    }
    
    // 노드 그리기
    for (uint8_t i = 0; i < nodeCount; i++) {
        bool isActive = (nodes[i].state == currentState);
        bool isSelected = (nodes[i].state == (SystemState)selectedState);
        drawStateNode(nodes[i], isActive, isSelected);
    }
    
    // ── 정보 패널 ──
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
        // 선택된 상태 정보
        tft.setTextColor(COLOR_PRIMARY);
        tft.setCursor(infoCard.x + CARD_PADDING, infoCard.y + CARD_PADDING);
        tft.print(getStateName((SystemState)selectedState));
        
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(infoCard.x + CARD_PADDING, infoCard.y + CARD_PADDING + 16);
        
        if (stateDiagramPage == 0) {
            tft.print("정상 사이클 경로");
        } else {
            tft.print("에러/비상 정지 경로");
        }
        
    } else {
        // 기본 안내
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(infoCard.x + CARD_PADDING, infoCard.y + CARD_PADDING);
        
        if (stateDiagramPage == 0) {
            tft.print("정상 작동 사이클 다이어그램");
            tft.setCursor(infoCard.x + CARD_PADDING, infoCard.y + CARD_PADDING + 16);
            tft.print("상태 박스를 터치하면 상세 정보 표시");
        } else {
            tft.print("에러 및 비상 정지 경로");
            tft.setCursor(infoCard.x + CARD_PADDING, infoCard.y + CARD_PADDING + 16);
            tft.print("점선 화살표는 비정상 전환 경로");
        }
    }
    
    // ── 하단 네비게이션 ──
    NavButton navButtons[] = {
        {"이전", BTN_SECONDARY, stateDiagramPage > 0},
        {"다음", BTN_SECONDARY, stateDiagramPage < 1},
        {"뒤로", BTN_OUTLINE, true}
    };
    drawNavBar(navButtons, 3);
}

// ================================================================
// 터치 핸들러
// ================================================================
void handleStateDiagramTouch(uint16_t x, uint16_t y) {
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    
    // 네비게이션 버튼
    if (y >= navY) {
        int16_t buttonW = (SCREEN_WIDTH - SPACING_SM * 4) / 3;
        
        // 이전 페이지
        if (stateDiagramPage > 0) {
            ButtonConfig prevBtn = {
                .x = SPACING_SM,
                .y = (int16_t)(navY + 2),
                .w = buttonW,
                .h = (int16_t)(FOOTER_HEIGHT - 4),
                .label = "이전",
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
        
        // 다음 페이지
        if (stateDiagramPage < 1) {
            ButtonConfig nextBtn = {
                .x = (int16_t)(SPACING_SM + buttonW + SPACING_SM),
                .y = (int16_t)(navY + 2),
                .w = buttonW,
                .h = (int16_t)(FOOTER_HEIGHT - 4),
                .label = "다음",
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
        
        // 뒤로
        ButtonConfig backBtn = {
            .x = (int16_t)(SPACING_SM + (buttonW + SPACING_SM) * 2),
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
    
    // 상태 노드 클릭 감지
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
            
            // 같은 노드 재클릭하면 선택 해제
            if (selectedState == (int8_t)nodes[i].state) {
                selectedState = -1;
            } else {
                selectedState = (int8_t)nodes[i].state;
            }
            
            screenNeedsRedraw = true;
            return;
        }
    }
    
    // 빈 공간 클릭 시 선택 해제
    if (selectedState >= 0) {
        selectedState = -1;
        screenNeedsRedraw = true;
    }
}

// ================================================================
// 호환성 함수 (기존 코드와의 호환)
// ================================================================
void drawStateDiagram() {
    drawStateDiagramScreen();
}

void handleStateDiagramTouch(uint16_t x, uint16_t y);  // 위에 정의됨
