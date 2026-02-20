// ================================================================
// UI_Screen_Help.cpp - 재설계 도움말 화면
// ================================================================
#include "../include/UIComponents.h"
#include "../include/Config.h"

using namespace UIComponents;using namespace UITheme;

// 페이지 인덱스
extern int helpPageIndex;

void drawHelpScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    // ── 헤더 ──
    drawHeader("도움말");
    
    // ── 페이지 인디케이터 ──
    int16_t startY = HEADER_HEIGHT + SPACING_SM;
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    int16_t pageTextX = (SCREEN_WIDTH - 60) / 2;
    tft.setCursor(pageTextX, startY);
    tft.printf("페이지 %d / 5", helpPageIndex + 1);
    
    // ── 콘텐츠 카드 ──
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
        case 0: // 기본 사용법
            tft.setTextColor(COLOR_PRIMARY);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("기본 사용법");
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            textY += lineH + 4;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("1. 시작 버튼으로 진공 시작");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("2. 박스 감지 시 자동 흡착");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("3. 설정된 시간 유지");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("4. 진공 해제 후 박스 제거");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("5. 다음 사이클 대기");
            textY += lineH * 2;
            
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("비상정지 버튼으로 언제든");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("즉시 중단 가능합니다");
            break;
            
        case 1: // 안전 수칙
            tft.setTextColor(COLOR_DANGER);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("안전 수칙");
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            textY += lineH + 4;
            
            drawIconWarning(contentCard.x + CARD_PADDING, textY, COLOR_WARNING);
            tft.setCursor(contentCard.x + CARD_PADDING + 20, textY);
            tft.print("온도 70°C 초과 시 자동 정지");
            textY += lineH;
            
            drawIconWarning(contentCard.x + CARD_PADDING, textY, COLOR_WARNING);
            tft.setCursor(contentCard.x + CARD_PADDING + 20, textY);
            tft.print("전류 6A 초과 시 경고");
            textY += lineH;
            
            drawIconWarning(contentCard.x + CARD_PADDING, textY, COLOR_WARNING);
            tft.setCursor(contentCard.x + CARD_PADDING + 20, textY);
            tft.print("비상정지 항상 접근 가능");
            textY += lineH * 2;
            
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("정기적인 센서 캘리브레이션과");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("유지보수를 권장합니다");
            break;
            
        case 2: // 관리자 기능
            tft.setTextColor(COLOR_MANAGER);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("관리자 기능");
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            textY += lineH + 4;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("시리얼 모니터에서:");
            textY += lineH;
            
            tft.setTextColor(COLOR_ACCENT);
            tft.setCursor(contentCard.x + CARD_PADDING + 10, textY);
            tft.print("> manager");
            textY += lineH;
            
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("비밀번호 입력 후 진입");
            textY += lineH * 2;
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("관리자 권한으로:");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("- 캘리브레이션 실행");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("- 설정 변경");
            break;
            
        case 3: // 문제 해결
            tft.setTextColor(COLOR_INFO);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("문제 해결");
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            textY += lineH + 4;
            
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("압력이 낮을 때:");
            textY += lineH;
            
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentCard.x + CARD_PADDING + 10, textY);
            tft.print("- 호스 연결 확인");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING + 10, textY);
            tft.print("- 밸브 상태 확인");
            textY += lineH * 2;
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("센서 오류 시:");
            textY += lineH;
            
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentCard.x + CARD_PADDING + 10, textY);
            tft.print("- 센서 연결 확인");
            textY += lineH;
            
            tft.setCursor(contentCard.x + CARD_PADDING + 10, textY);
            tft.print("- 재부팅 후 재시도");
            break;
            
        case 4: // 키보드 단축키
            tft.setTextColor(COLOR_ACCENT);
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("키보드 단축키");
            
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            textY += lineH + 4;
            tft.setCursor(contentCard.x + CARD_PADDING, textY);
            tft.print("USB 키보드 연결 시 사용 가능");
            textY += lineH + 4;
            
            // 좌측 컬럼
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            int16_t col1X = contentCard.x + CARD_PADDING;
            
            tft.setCursor(col1X, textY);
            tft.print("0: 메인 화면");
            textY += lineH;
            
            tft.setCursor(col1X, textY);
            tft.print("1: 시작");
            textY += lineH;
            
            tft.setCursor(col1X, textY);
            tft.print("2: 정지");
            textY += lineH;
            
            tft.setCursor(col1X, textY);
            tft.print("3: 모드 전환");
            textY += lineH;
            
            tft.setCursor(col1X, textY);
            tft.print("4: 알람 리셋");
            textY += lineH;
            
            // 우측 컬럼
            int16_t col2X = contentCard.x + CARD_PADDING + 140;
            textY = contentCard.y + CARD_PADDING + lineH * 2 + 4;
            
            tft.setCursor(col2X, textY);
            tft.print("5: 통계");
            textY += lineH;
            
            tft.setCursor(col2X, textY);
            tft.print("*: 설정");
            textY += lineH;
            
            tft.setCursor(col2X, textY);
            tft.print("ESC: 메인");
            textY += lineH;
            
            tft.setCursor(col2X, textY);
            tft.print("←: 뒤로");
            textY += lineH;
            
            tft.setCursor(col2X, textY);
            tft.print("+/-: 페이지");
            break;
    }
    
    // ── 하단 네비게이션 ──
    NavButton navButtons[3];
    int navCount = 0;
    
    if (helpPageIndex > 0) {
        navButtons[navCount++] = {"이전", BTN_SECONDARY, true};
    } else {
        navButtons[navCount++] = {"뒤로", BTN_OUTLINE, true};
    }
    
    if (helpPageIndex < 4) {
        navButtons[navCount++] = {"다음", BTN_PRIMARY, true};
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
        
        // 첫 번째 버튼 (이전 또는 뒤로)
        ButtonConfig firstBtn = {
            .x = SPACING_SM,
            .y = (int16_t)(navY + 2),
            .w = buttonW,
            .h = (int16_t)(FOOTER_HEIGHT - 4),
            .label = (helpPageIndex > 0) ? "이전" : "뒤로",
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
        
        // 두 번째 버튼 (다음)
        if (helpPageIndex < 3 && buttonCount == 2) {
            ButtonConfig nextBtn = {
                .x = (int16_t)(SPACING_SM + buttonW + SPACING_SM),
                .y = (int16_t)(navY + 2),
                .w = buttonW,
                .h = (int16_t)(FOOTER_HEIGHT - 4),
                .label = "다음",
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