// ================================================================
// CommandHandler.cpp - String   (v3.9.3)
// String  char[]    
// ================================================================

#include "CommandHandler.h"
#include "CommandHistory.h"
#include "ExceptionHandler.h"
#include "SystemController.h"
#include "SensorManager.h"
#include "ControlManager.h"
#include "NetworkManager.h"
#include "UIManager.h"
#include "ConfigManager.h"
#include "SystemTest.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstring>
#include <cctype>

CommandHandler commandHandler;
extern CommandHistory commandHistory;
extern SensorManager sensorManager;

// ================================================================
//   
// ================================================================

static void trimString(char* str) {
    if (!str || *str == '\0') return;
    
    //   
    char* start = str;
    while (*start && isspace(static_cast<unsigned char>(*start))) start++;
    
    //   
    char* end = str + strlen(str) - 1;
    while (end > start && isspace(static_cast<unsigned char>(*end))) end--;
    *(end + 1) = '\0';
    
    //  
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

static void toLowerString(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(static_cast<unsigned char>(str[i]));
    }
}

static bool streq(const char* a, const char* b) {
    return strcmp(a, b) == 0;
}

// ================================================================
// CommandHandler 
// ================================================================

void CommandHandler::begin() {
    memset(cmdBuffer, 0, sizeof(cmdBuffer));
    memset(passwordBuffer, 0, sizeof(passwordBuffer));
    Serial.println("[CMD] CommandHandler   (String )");
}

bool CommandHandler::readCommand(char* buffer, size_t bufferSize) {
    if (!Serial.available() || !buffer || bufferSize == 0) {
        return false;
    }
    
    int len = Serial.readBytesUntil('\n', buffer, bufferSize - 1);
    if (len == 0) {
        return false;
    }
    
    buffer[len] = '\0';
    trimString(buffer);
    
    return strlen(buffer) > 0;
}

bool CommandHandler::waitForPassword(char* buffer, size_t bufferSize, uint32_t timeout) {
    if (!buffer || bufferSize == 0) return false;
    
    uint32_t startWait = millis();
    while (!Serial.available() && (millis() - startWait < timeout)) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    if (Serial.available()) {
        int len = Serial.readBytesUntil('\n', buffer, bufferSize - 1);
        if (len > 0) {
            buffer[len] = '\0';
            trimString(buffer);
            return strlen(buffer) > 0;
        }
    }
    
    buffer[0] = '\0';
    return false;
}

void CommandHandler::handleModeSwitch(const char* cmd) {
    //  
    if (streq(cmd, "operator") || streq(cmd, "logout")) {
        if (systemController.enterOperatorMode()) {
            Serial.println("  ");
            uiManager.showToast(" ", TFT_BLUE);
        }
        return;
    }
    
    //  
    if (streq(cmd, "manager")) {
        if (systemController.isLockedOut()) {
            uint32_t remaining = systemController.getLockoutRemainingTime();
            Serial.printf(" : %lu  \n", remaining / 1000);
            return;
        }
        
        Serial.println(" :");
        Serial.print("> ");
        
        if (waitForPassword(passwordBuffer, sizeof(passwordBuffer), 30000)) {
            if (systemController.enterManagerMode(passwordBuffer)) {
                Serial.println("  ");
                uiManager.showToast(" ", TFT_GREEN);
            } else {
                Serial.println("  ");
                systemController.recordFailedLogin();
                
                if (systemController.isLockedOut()) {
                    Serial.println("  - 1");
                }
            }
        } else {
            Serial.println(" ");
        }
        
        // :   
        memset(passwordBuffer, 0, sizeof(passwordBuffer));
        return;
    }
    
    //  
    if (streq(cmd, "developer") || streq(cmd, "dev")) {
        if (systemController.isLockedOut()) {
            uint32_t remaining = systemController.getLockoutRemainingTime();
            Serial.printf(" : %lu  \n", remaining / 1000);
            return;
        }
        
        Serial.println(" :");
        Serial.print("> ");
        
        if (waitForPassword(passwordBuffer, sizeof(passwordBuffer), 30000)) {
            if (systemController.enterDeveloperMode(passwordBuffer)) {
                Serial.println("  ");
                uiManager.showToast(" ", TFT_YELLOW);
            } else {
                Serial.println("  ");
                systemController.recordFailedLogin();
                
                if (systemController.isLockedOut()) {
                    Serial.println("  - 1");
                }
            }
        } else {
            Serial.println(" ");
        }
        
        // :   
        memset(passwordBuffer, 0, sizeof(passwordBuffer));
        return;
    }
}

void CommandHandler::handleSerialCommands() {
    if (!Serial.available()) return;

    // char    (String   )
    if (!readCommand(cmdBuffer, sizeof(cmdBuffer))) {
        return;
    }
    
    toLowerString(cmdBuffer);
    
    Serial.printf("\n[CMD] '%s'\n", cmdBuffer);

    //   
    commandHistory.add(cmdBuffer);

    //    
    if (streq(cmdBuffer, "operator") || streq(cmdBuffer, "logout") || 
        streq(cmdBuffer, "manager") || streq(cmdBuffer, "developer") || 
        streq(cmdBuffer, "dev")) {
        handleModeSwitch(cmdBuffer);
        return;
    }

    //  
    if (streq(cmdBuffer, "history")) {
        commandHistory.print();
        return;
    }

    //    
    if (systemController.isOperatorMode()) {
        handleOperatorCommands(cmdBuffer);
    }
    else if (systemController.isManagerMode()) {
        if (!handleManagerCommands(cmdBuffer)) {
            handleOperatorCommands(cmdBuffer);
        }
    }
    else if (systemController.isDeveloperMode()) {
        if (!handleDeveloperCommands(cmdBuffer)) {
            if (!handleManagerCommands(cmdBuffer)) {
                handleOperatorCommands(cmdBuffer);
            }
        }
    }
}

void CommandHandler::handleOperatorCommands(const char* cmd) {
    if (streq(cmd, "start")) {
        controlManager.setPumpState(true);
        Serial.println("  ");
    }
    else if (streq(cmd, "stop")) {
        controlManager.setPumpState(false);
        Serial.println("  ");
    }
    else if (streq(cmd, "pause")) {
        controlManager.setPumpState(false);  // pause
        Serial.println(" ");
    }
    else if (streq(cmd, "status")) {
        Serial.println("\n===   ===");
        sensorManager.printStatus();
        controlManager.printStatus();
        Serial.println("==================\n");
    }
    else if (streq(cmd, "sensor")) {
        sensorManager.printStatus();
    }
    else if (streq(cmd, "help") || streq(cmd, "?")) {
        Serial.println("\n");
        Serial.println("            ");
        Serial.println("");
        Serial.println("  start    - ");
        Serial.println("  stop     - ");
        Serial.println("  pause    - ");
        Serial.println("  status   - ");
        Serial.println("  sensor   - ");
        Serial.println("  history  -  ");
        Serial.println("  manager  -  ");
        Serial.println();
    }
    else {
        Serial.printf("    : '%s'\n", cmd);
        Serial.println(" 'help' ");
    }
}

bool CommandHandler::handleManagerCommands(const char* cmd) {
    if (streq(cmd, "calibrate")) {
        Serial.println(" ...");
        sensorManager.calibratePressure();
        sensorManager.calibrateCurrent();
        Serial.println(" ");
        return true;
    }
    else if (streq(cmd, "config_save")) {
        // configManager.saveConfig();  //  
        Serial.println("  ");
        return true;
    }
    else if (streq(cmd, "network_status")) {
        // networkManager.printStatus();
        return true;
    }
    else if (streq(cmd, "help_manager")) {
        Serial.println("\n");
        Serial.println("            ");
        Serial.println("");
        Serial.println("  calibrate     - ");
        Serial.println("  config_save   -  ");
        Serial.println("  network_status- ");
        Serial.println("  developer     -  ");
        Serial.println();
        return true;
    }
    
    return false;
}

bool CommandHandler::handleDeveloperCommands(const char* cmd) {
    if (streq(cmd, "test_all")) {
        Serial.println("\n ...\n");
        systemTest.runAllTests();
        return true;
    }
    else if (streq(cmd, "version")) {
        Serial.println("\n");
        Serial.println("  ESP32-S3     ");
        Serial.println("");
        Serial.println("  : v3.9.3 String ");
        Serial.println("  : 2024.12 (Optimized) ");
        Serial.println("\n");
        return true;
    }
    else if (streq(cmd, "help_dev")) {
        Serial.println("\n");
        Serial.println("            ");
        Serial.println("");
        Serial.println("  test_all -  ");
        Serial.println("  version  - ");
        Serial.println("  history  -  ");
        Serial.println();
        return true;
    }
    
    return false;
}
