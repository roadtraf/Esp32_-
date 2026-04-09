// ================================================================
// SafeMode.cpp -    
// ================================================================
#include "SafeMode.h"
#include "GFX_Wrapper.hpp"
//  
SafeMode safeMode;

//    ( )
extern TFT_GFX tft;

// ================================================================
// 
// ================================================================
void SafeMode::begin() {
    Serial.println("[SafeMode]  ...");
    
    inSafeMode = false;
    bootSuccessMarked = false;
    bootStartTime = millis();
    
    //   
    loadBootInfo();
    
    //   
    markBootStart();
    
    Serial.printf("[SafeMode]  : %lu (: %lu, : %lu)\n",
                  bootInfo.bootCount, bootInfo.successfulBoots, bootInfo.failedBoots);
    Serial.printf("[SafeMode]  : %lu\n", bootInfo.consecutiveFailures);
    
    //     
    if (shouldEnterSafeMode()) {
        Serial.println("[SafeMode]       ");
        enterSafeMode(bootInfo.lastFailure);
    } else {
        Serial.println("[SafeMode]    ");
    }
}

// ================================================================
//   
// ================================================================
bool SafeMode::checkBootStatus() {
    //         
    if (!bootSuccessMarked && (millis() - bootStartTime) > (SAFE_MODE_BOOT_TIMEOUT * 1000)) {
        Serial.println("[SafeMode]   ");
        markBootFailure(BOOT_UNKNOWN_ERROR);
        return false;
    }
    
    return true;
}

// ================================================================
//   
// ================================================================
void SafeMode::markBootStart() {
    bootStartTime = millis();
    bootSuccessMarked = false;
    
    incrementBootCount();
    
    Serial.println("[SafeMode]   ");
}

// ================================================================
//   
// ================================================================
void SafeMode::markBootSuccess() {
    if (bootSuccessMarked) return;  //   
    
    bootSuccessMarked = true;
    bootInfo.successfulBoots++;
    resetFailureCount();
    
    saveBootInfo();
    
    uint32_t bootTime = (millis() - bootStartTime) / 1000;
    Serial.printf("[SafeMode]   ! (: %lu)\n", bootTime);
}

// ================================================================
//   
// ================================================================
void SafeMode::markBootFailure(BootFailureReason reason) {
    bootInfo.failedBoots++;
    bootInfo.consecutiveFailures++;
    bootInfo.lastFailure = reason;
    bootInfo.lastBootTime = millis() / 1000;
    
    saveBootInfo();
    
    Serial.printf("[SafeMode]   : %s\n", getFailureReasonString(reason));
    Serial.printf("[SafeMode]   : %lu\n", bootInfo.consecutiveFailures);
}

// ================================================================
//   
// ================================================================
bool SafeMode::isInSafeMode() {
    return inSafeMode;
}

bool SafeMode::shouldEnterSafeMode() {
    return (bootInfo.consecutiveFailures >= SAFE_MODE_MAX_BOOT_FAILURES);
}

// ================================================================
//   
// ================================================================
void SafeMode::enterSafeMode(BootFailureReason reason) {
    inSafeMode = true;
    
    Serial.println("\n");
    Serial.println("");
    Serial.println("                                 ");
    Serial.println("");
    Serial.printf(" : %-43s \n", getFailureReasonString(reason));
    Serial.printf("  : %lu                                  \n", 
                  bootInfo.consecutiveFailures);
    Serial.println("                                                   ");
    Serial.println("    .           ");
    Serial.println("   .                          ");
    Serial.println("");
    Serial.println();
    
    // UI      
    // drawSafeModeScreen();
}

// ================================================================
//   
// ================================================================
void SafeMode::exitSafeMode() {
    inSafeMode = false;
    resetFailureCount();
    saveBootInfo();
    
    Serial.println("[SafeMode]   ");
}

// ================================================================
//  
// ================================================================
bool SafeMode::restoreFromBackup() {
    Serial.println("[SafeMode]   ...");
    
    // ConfigManager   
    // (  ConfigManager  )
    
    Serial.println("[SafeMode]    ");
    return true;
}

bool SafeMode::factoryReset() {
    Serial.println("[SafeMode]   ...");
    
    // 1.   
    // 2.  
    // 3.   
    resetBootStats();
    
    Serial.println("[SafeMode]    ");
    Serial.println("[SafeMode]  .");
    
    return true;
}

bool SafeMode::diagnosticMode() {
    Serial.println("[SafeMode]   ...");
    
    Serial.println("\n===   ===");
    
    // 1.  
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Heap Size: %d bytes\n", ESP.getHeapSize());
    Serial.printf("Min Free Heap: %d bytes\n", ESP.getMinFreeHeap());
    
    // 2. Flash 
    Serial.printf("Flash Size: %d bytes\n", ESP.getFlashChipSize());
    Serial.printf("Flash Speed: %d Hz\n", ESP.getFlashChipSpeed());
    
    // 3. CPU 
    Serial.printf("CPU Freq: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("Chip Revision: %d\n", ESP.getChipRevision());
    
    // 4.  
    Serial.printf("Reset Reason: %d\n", esp_reset_reason());
    
    Serial.println("==================\n");
    
    return true;
}

// ================================================================
//  
// ================================================================
BootInfo SafeMode::getBootInfo() {
    return bootInfo;
}

uint32_t SafeMode::getConsecutiveFailures() {
    return bootInfo.consecutiveFailures;
}

BootFailureReason SafeMode::getLastFailureReason() {
    return bootInfo.lastFailure;
}

// ================================================================
// 
// ================================================================
void SafeMode::resetBootStats() {
    bootInfo.bootCount = 0;
    bootInfo.successfulBoots = 0;
    bootInfo.failedBoots = 0;
    bootInfo.consecutiveFailures = 0;
    bootInfo.lastFailure = BOOT_SUCCESS;
    bootInfo.lastBootTime = 0;
    
    saveBootInfo();
    
    Serial.println("[SafeMode]    ");
}

void SafeMode::printBootInfo() {
    Serial.println("\n");
    Serial.println("                               ");
    Serial.println("");
    Serial.printf("  : %lu                        \n", bootInfo.bootCount);
    Serial.printf(" : %lu                           \n", bootInfo.successfulBoots);
    Serial.printf(" : %lu                           \n", bootInfo.failedBoots);
    Serial.printf("  : %lu                      \n", bootInfo.consecutiveFailures);
    Serial.println("");
    
    if (bootInfo.lastFailure != BOOT_SUCCESS) {
        Serial.printf("  : %-24s \n", 
                      getFailureReasonString(bootInfo.lastFailure));
    }
    
    Serial.println("\n");
}

void SafeMode::printSafeModeStatus() {
    Serial.println("\n");
    Serial.println("                           ");
    Serial.println("");
    Serial.printf("  : %-26s \n", 
                  inSafeMode ? "   " : "  ");
    Serial.printf("  : %-26s \n",
                  bootSuccessMarked ? "" : "");
    
    if (shouldEnterSafeMode()) {
        Serial.println("                                       ");
        Serial.println("                ");
    }
    
    Serial.println("\n");
}

// ================================================================
// UI 
// ================================================================
void SafeMode::drawSafeModeScreen() {
    // TFT   
    // tft.fillScreen(TFT_BLACK);
    
    // 
    // tft.setTextSize(2);
    // tft.setTextColor(TFT_RED);
    // tft.setCursor(50, 20);
    // tft.print(" ");
    
    // 
    // ...
    
    Serial.println("[SafeMode]   UI ");
}

SafeModeOption SafeMode::handleSafeModeTouch(uint16_t x, uint16_t y) {
    //   
    // ...
    
    return SAFE_CONTINUE_ANYWAY;
}

// ================================================================
//  
// ================================================================
void SafeMode::loadBootInfo() {
    prefs.begin(SAFE_MODE_PREFERENCE_KEY, true);  //  
    
    bootInfo.bootCount = prefs.getUInt("bootCnt", 0);
    bootInfo.successfulBoots = prefs.getUInt("successCnt", 0);
    bootInfo.failedBoots = prefs.getUInt("failCnt", 0);
    bootInfo.consecutiveFailures = prefs.getUInt("conseqFail", 0);
    bootInfo.lastFailure = (BootFailureReason)prefs.getUInt("lastFail", BOOT_SUCCESS);
    bootInfo.lastBootTime = prefs.getUInt("lastBootT", 0);
    
    prefs.end();
}

void SafeMode::saveBootInfo() {
    prefs.begin(SAFE_MODE_PREFERENCE_KEY, false);  //  
    
    prefs.putUInt("bootCnt", bootInfo.bootCount);
    prefs.putUInt("successCnt", bootInfo.successfulBoots);
    prefs.putUInt("failCnt", bootInfo.failedBoots);
    prefs.putUInt("conseqFail", bootInfo.consecutiveFailures);
    prefs.putUInt("lastFail", (uint32_t)bootInfo.lastFailure);
    prefs.putUInt("lastBootT", bootInfo.lastBootTime);
    
    prefs.end();
}

void SafeMode::incrementBootCount() {
    bootInfo.bootCount++;
    saveBootInfo();
}

void SafeMode::resetFailureCount() {
    bootInfo.consecutiveFailures = 0;
    bootInfo.lastFailure = BOOT_SUCCESS;
}

const char* SafeMode::getFailureReasonString(BootFailureReason reason) {
    switch (reason) {
        case BOOT_SUCCESS:           return "";
        case BOOT_WATCHDOG_TIMEOUT:  return "Watchdog ";
        case BOOT_CONFIG_CORRUPTED:  return " ";
        case BOOT_HARDWARE_FAILURE:  return " ";
        case BOOT_MEMORY_ERROR:      return " ";
        case BOOT_SENSOR_FAILURE:    return " ";
        case BOOT_NETWORK_TIMEOUT:   return " ";
        default:                     return "  ";
    }
}
