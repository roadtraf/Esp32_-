// ================================================================
// ManagerUI.cpp -   UI 
// ================================================================
#include "ManagerUI.h"
#include "Config.h"
#include "UI_Screens.h"
#include "UI_AccessControl.h"

// FreeRTOS (delay )
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern TFT_GFX tft;

// ================================================================
//     [U11] TFT_*   UITheme  
// ================================================================
void drawManagerBadge() {
    using namespace UITheme;

    SystemMode mode = systemController.getMode();
    if (mode == SystemMode::OPERATOR) return;

    int16_t  x = SCREEN_WIDTH - 100;
    int16_t  y = 5;
    int16_t  w = 95;
    int16_t  h = 25;

    // [U11] TFT_ORANGE/TFT_RED  UITheme 
    uint16_t bgColor = (mode == SystemMode::MANAGER)
                       ? COLOR_MANAGER : COLOR_DEVELOPER;

    tft.fillRoundRect(x, y, w, h, 5, bgColor);
    tft.drawRoundRect(x, y, w, h, 5, COLOR_TEXT_PRIMARY);  // [U11] TFT_WHITE 

    tft.setTextSize(1);
    tft.setTextColor(COLOR_BG_DARK);  // [U11] TFT_WHITE   

    const char* modeText = systemController.getModeString();
    // [U7] textWidth   
    int16_t tw = tft.textWidth(modeText);
    tft.setCursor(x + (w - tw) / 2, y + (h - 8) / 2);
    tft.print(modeText);
}

// ================================================================
//    
// ================================================================
void drawLogoutTimer() {
    if (!systemController.isAutoLogoutEnabled() || systemController.isOperatorMode()) {
        return;
    }
    
    uint32_t remaining = systemController.getRemainingTime();
    
    //    
    int16_t x = tft.width() - 100;
    int16_t y = 35;
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW);
    
    uint32_t minutes = remaining / 60000;
    uint32_t seconds = (remaining % 60000) / 1000;
    
    char timerBuf[16];
    snprintf(timerBuf, sizeof(timerBuf), "%lu:%02lu", minutes, seconds);
    
    tft.setCursor(x + 10, y);
    tft.print(timerBuf);
}

// ================================================================
//   
// ================================================================
void drawManagerMenu() {
    //   
    int16_t menuW = 280;
    int16_t menuH = 200;
    int16_t menuX = (tft.width() - menuW) / 2;
    int16_t menuY = (tft.height() - menuH) / 2;
    
    //  
    tft.fillRect(0, 0, tft.width(), tft.height(), TFT_BLACK);
    
    //  
    tft.fillRoundRect(menuX, menuY, menuW, menuH, 10, TFT_DARKGREY);
    tft.drawRoundRect(menuX, menuY, menuW, menuH, 10, TFT_ORANGE);
    
    // 
    tft.setTextSize(2);
    tft.setTextColor(TFT_ORANGE);
    tft.setCursor(menuX + 60, menuY + 10);
    tft.print(" ");
    
    //  
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    
    int16_t itemY = menuY + 50;
    int16_t lineHeight = 25;
    
    tft.setCursor(menuX + 20, itemY);
    tft.print("1.  ");
    
    tft.setCursor(menuX + 20, itemY + lineHeight);
    tft.print("2. ");
    
    tft.setCursor(menuX + 20, itemY + lineHeight * 2);
    tft.print("3.  ");
    
    tft.setCursor(menuX + 20, itemY + lineHeight * 3);
    tft.print("4.  ");
    
    tft.setCursor(menuX + 20, itemY + lineHeight * 4);
    tft.print("5. ");
    
    //  
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(menuX + 40, menuY + menuH - 20);
    tft.print(" ");
}

// ================================================================
//   
// ================================================================
bool showPermissionDialog(const char* action) {
    SystemPermissions perms = systemController.getPermissions();
    
    //     
    bool hasPermission = systemController.hasPermission(action);
    
    if (!hasPermission) {
        //   
        int16_t dialogW = 250;
        int16_t dialogH = 120;
        int16_t dialogX = (tft.width() - dialogW) / 2;
        int16_t dialogY = (tft.height() - dialogH) / 2;
        
        tft.fillRoundRect(dialogX, dialogY, dialogW, dialogH, 10, TFT_RED);
        tft.drawRoundRect(dialogX, dialogY, dialogW, dialogH, 10, TFT_WHITE);
        
        tft.setTextSize(2);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(dialogX + 50, dialogY + 20);
        tft.print(" ");
        
        tft.setTextSize(1);
        tft.setCursor(dialogX + 20, dialogY + 60);
        tft.print("  ");
        
        tft.setCursor(dialogX + 70, dialogY + 90);
        tft.print("(3  )");
        
        // [R4] : 3   
        Serial.println(" "); 
        return false;
    }
    
    return true;
}

// ================================================================
//   
// ================================================================
// showAccessDenied - UI_AccessControl.h 

// ================================================================
//   
// ================================================================
void drawAdvancedStatistics() {
    tft.fillScreen(TFT_BLACK);
    
    // 
    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(10, 10);
    tft.print(" ");
    
    //  
    drawManagerBadge();
    
    //   (Phase 2-3  )
    SensorStats stats;
    calculateSensorStats(stats);
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    
    int16_t y = 50;
    int16_t lineH = 20;
    
    // 
    tft.setCursor(10, y);
    tft.printf(": %.2fC", stats.avgTemperature);
    tft.setCursor(200, y);
    tft.printf("(%.2f ~ %.2f)", stats.minTemperature, stats.maxTemperature);
    y += lineH;
    
    tft.setCursor(10, y);
    tft.printf("  : %.2f", stats.tempStdDev);
    y += lineH * 2;
    
    // 
    tft.setCursor(10, y);
    tft.printf(": %.2f kPa", stats.avgPressure);
    tft.setCursor(200, y);
    tft.printf("(%.2f ~ %.2f)", stats.minPressure, stats.maxPressure);
    y += lineH;
    
    tft.setCursor(10, y);
    tft.printf("  : %.2f", stats.pressureStdDev);
    y += lineH * 2;
    
    // 
    tft.setCursor(10, y);
    tft.printf(": %.2f A", stats.avgCurrent);
    tft.setCursor(200, y);
    tft.printf("(%.2f ~ %.2f)", stats.minCurrent, stats.maxCurrent);
    y += lineH;
    
    tft.setCursor(10, y);
    tft.printf("  : %.2f", stats.currentStdDev);
    y += lineH * 2;
    
    //  
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, y);
    tft.printf("  : %lu", stats.sampleCount);
    y += lineH;
    
    tft.setCursor(10, y);
    tft.printf(" : %.1f%%", 
               (float)temperatureBuffer.size() / TEMP_BUFFER_SIZE * 100);
    
    //  
    tft.fillRoundRect(10, tft.height() - 40, 80, 30, 5, TFT_DARKGREY);
    tft.drawRoundRect(10, tft.height() - 40, 80, 30, 5, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(30, tft.height() - 32);
    tft.print("");
}

// ================================================================
//   
// ================================================================
void drawSystemDiagnostics() {
    tft.fillScreen(TFT_BLACK);
    
    // 
    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(10, 10);
    tft.print(" ");
    
    //  
    drawManagerBadge();
    
    tft.setTextSize(1);
    int16_t y = 50;
    int16_t lineH = 18;
    
    //  
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, y);
    tft.print(" :");
    y += lineH;
    
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(20, y);
    tft.printf("Free Heap: %lu KB", ESP.getFreeHeap() / 1024);
    y += lineH;
    
    tft.setCursor(20, y);
    tft.printf("Min Free:  %lu KB", ESP.getMinFreeHeap() / 1024);
    y += lineH * 2;
    
    //  
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, y);
    tft.print(" :");
    y += lineH;
    
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(20, y);
    tft.printf(" : %s", 
               isTemperatureSensorConnected() ? "" : "");
    y += lineH;
    
    tft.setCursor(20, y);
    tft.printf(" : ");
    y += lineH;
    
    tft.setCursor(20, y);
    tft.printf(" : ");
    y += lineH * 2;
    
    // WiFi 
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, y);
    tft.print(" :");
    y += lineH;
    
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(20, y);
    if (WiFi.status() == WL_CONNECTED) {
        tft.printf("WiFi:  (RSSI: %d)", WiFi.RSSI());
    } else {
        tft.print("WiFi:  ");
    }
    y += lineH;
    
    tft.setCursor(20, y);
    tft.printf("MQTT: %s", mqttConnected ? "" : "");
    y += lineH * 2;
    
    //  
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, y);
    tft.print(" :");
    y += lineH;
    
    tft.setTextColor(TFT_WHITE);
    uint32_t uptime = millis() / 1000;
    tft.setCursor(20, y);
    tft.printf(" : %luh %lum", uptime / 3600, (uptime % 3600) / 60);
    
    //  
    tft.fillRoundRect(10, tft.height() - 40, 80, 30, 5, TFT_DARKGREY);
    tft.drawRoundRect(10, tft.height() - 40, 80, 30, 5, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(30, tft.height() - 32);
    tft.print("");
}