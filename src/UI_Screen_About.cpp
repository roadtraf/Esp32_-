// ================================================================
// UI_Screen_About.cpp - 재설계 정보 화면 (Bug Fixed)
// ================================================================
#include "../include/UIComponents.h"
#include "../include/Config.h"

using namespace UIComponents;
using namespace UITheme;

void drawAboutScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    // ── 헤더 ──
    drawHeader("시스템 정보");
    
    // ── 로고/제목 카드 ──
    int16_t startY = HEADER_HEIGHT + SPACING_MD;
    
    CardConfig titleCard = {
        .x = SPACING_SM,
        .y = startY,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = 70,
        .bgColor = COLOR_PRIMARY_DARK,
        .elevated = true
    };
    drawCard(titleCard);
    
    // 시스템 이름
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    int16_t titleX = titleCard.x + (titleCard.w - strlen("ESP32-S3 진공 제어") * 12) / 2;
    tft.setCursor(titleX, titleCard.y + CARD_PADDING);
    tft.print("ESP32-S3 진공 제어");
    
    // 버전
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_ACCENT);
    int16_t versionX = titleCard.x + (titleCard.w - strlen(FIRMWARE_VERSION) * 6) / 2;
    tft.setCursor(versionX, titleCard.y + CARD_PADDING + 22);
    tft.print(FIRMWARE_VERSION);
    
    // 빌드 날짜
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    int16_t dateX = titleCard.x + (titleCard.w - strlen(BUILD_DATE) * 6) / 2;
    tft.setCursor(dateX, titleCard.y + CARD_PADDING + 38);
    tft.print(BUILD_DATE);
    
    // ── 시스템 정보 그리드 ──
    int16_t gridY = titleCard.y + titleCard.h + SPACING_SM;
    int16_t itemW = (SCREEN_WIDTH - SPACING_SM * 3) / 2;
    int16_t itemH = 55;
    
    // ✅ FIX: 구조체 정의 수정 - const char*는 직접 할당
    struct InfoItem {
        const char* label;
        char value[32];
        uint16_t color;
    };
    
    InfoItem items[6];
    
    // ✅ FIX: label은 포인터 할당, value만 strcpy 사용
    // CPU 정보
    items[0].label = "MCU";
    strcpy(items[0].value, "ESP32-S3");
    items[0].color = COLOR_PRIMARY;
    
    // 메모리
    items[1].label = "Free Heap";
    snprintf(items[1].value, sizeof(items[1].value), "%lu KB", ESP.getFreeHeap() / 1024);
    items[1].color = COLOR_SUCCESS;
    
    // 가동 시간
    items[2].label = "Uptime";
    uint32_t uptime = millis() / 1000;
    snprintf(items[2].value, sizeof(items[2].value), "%luh %lum", uptime / 3600, (uptime % 3600) / 60);
    items[2].color = COLOR_ACCENT;
    
    // WiFi 상태
    items[3].label = "WiFi";
    if (WiFi.status() == WL_CONNECTED) {
        snprintf(items[3].value, sizeof(items[3].value), "연결됨");
        items[3].color = COLOR_SUCCESS;
    } else {
        strcpy(items[3].value, "연결 안 됨");
        items[3].color = COLOR_DANGER;
    }
    
    // MQTT 상태
    items[4].label = "MQTT";
    strcpy(items[4].value, mqttConnected ? "연결됨" : "연결 안 됨");
    items[4].color = mqttConnected ? COLOR_SUCCESS : COLOR_DANGER;
    
    // 센서 개수
    items[5].label = "센서";
    snprintf(items[5].value, sizeof(items[5].value), "%d개", getTemperatureSensorCount() + 2); // 온도 + 압력 + 전류
    items[5].color = COLOR_INFO;
    
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 2; col++) {
            int idx = row * 2 + col;
            
            int16_t x = SPACING_SM + col * (itemW + SPACING_SM);
            int16_t y = gridY + row * (itemH + 4);
            
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
            tft.setCursor(x + 6, y + 20);
            tft.print(items[idx].value);
        }
    }
    
    // ── 저작권 ──
    int16_t copyrightY = gridY + 3 * (itemH + 4) + SPACING_SM;
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    
    const char* copyright1 = "Developed with Claude";
    int16_t cr1X = (SCREEN_WIDTH - strlen(copyright1) * 6) / 2;
    tft.setCursor(cr1X, copyrightY);
    tft.print(copyright1);
    
    const char* copyright2 = "Phase 1-2 Complete";
    int16_t cr2X = (SCREEN_WIDTH - strlen(copyright2) * 6) / 2;
    tft.setCursor(cr2X, copyrightY + 12);
    tft.print(copyright2);
    
    // ── 하단 네비게이션 ──
    NavButton navButtons[] = {
        {"뒤로", BTN_OUTLINE, true}
    };
    drawNavBar(navButtons, 1);
}

void handleAboutTouch(uint16_t x, uint16_t y) {
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    
    if (y >= navY) {
        ButtonConfig backBtn = {
            .x = SPACING_SM,
            .y = (int16_t)(navY + 2),
            .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
            .h = (int16_t)(FOOTER_HEIGHT - 4),
            .label = "뒤로",
            .style = BTN_OUTLINE,
            .enabled = true
        };
        
        if (isButtonPressed(backBtn, x, y)) {
            currentScreen = SCREEN_SETTINGS;
            screenNeedsRedraw = true;
        }
    }
}
