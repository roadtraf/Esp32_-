// ================================================================
// CommandHandler.cpp - String 최적화 버전 (v3.9.3)
// String → char[] 변환으로 힙 단편화 방지
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
// 문자열 유틸리티 함수
// ================================================================

static void trimString(char* str) {
    if (!str || *str == '\0') return;
    
    // 앞쪽 공백 제거
    char* start = str;
    while (*start && isspace(static_cast<unsigned char>(*start))) start++;
    
    // 뒤쪽 공백 제거
    char* end = str + strlen(str) - 1;
    while (end > start && isspace(static_cast<unsigned char>(*end))) end--;
    *(end + 1) = '\0';
    
    // 문자열 앞당기기
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
// CommandHandler 구현
// ================================================================

void CommandHandler::begin() {
    memset(cmdBuffer, 0, sizeof(cmdBuffer));
    memset(passwordBuffer, 0, sizeof(passwordBuffer));
    Serial.println("[CMD] CommandHandler 초기화 완료 (String 최적화)");
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
    // 작업자 모드
    if (streq(cmd, "operator") || streq(cmd, "logout")) {
        if (systemController.enterOperatorMode()) {
            Serial.println("✅ 작업자 모드");
            uiManager.showToast("작업자 모드", TFT_BLUE);
        }
        return;
    }
    
    // 관리자 모드
    if (streq(cmd, "manager")) {
        if (systemController.isLockedOut()) {
            uint32_t remaining = systemController.getLockoutRemainingTime();
            Serial.printf("🔒 잠금: %lu초 후 재시도\n", remaining / 1000);
            return;
        }
        
        Serial.println("관리자 비밀번호:");
        Serial.print("> ");
        
        if (waitForPassword(passwordBuffer, sizeof(passwordBuffer), 30000)) {
            if (systemController.enterManagerMode(passwordBuffer)) {
                Serial.println("✅ 관리자 모드");
                uiManager.showToast("관리자 모드", TFT_GREEN);
            } else {
                Serial.println("❌ 비밀번호 오류");
                systemController.recordFailedLogin();
                
                if (systemController.isLockedOut()) {
                    Serial.println("🔒 잠금 - 1분");
                }
            }
        } else {
            Serial.println("⏱️ 타임아웃");
        }
        
        // 보안: 비밀번호 버퍼 초기화
        memset(passwordBuffer, 0, sizeof(passwordBuffer));
        return;
    }
    
    // 개발자 모드
    if (streq(cmd, "developer") || streq(cmd, "dev")) {
        if (systemController.isLockedOut()) {
            uint32_t remaining = systemController.getLockoutRemainingTime();
            Serial.printf("🔒 잠금: %lu초 후 재시도\n", remaining / 1000);
            return;
        }
        
        Serial.println("개발자 비밀번호:");
        Serial.print("> ");
        
        if (waitForPassword(passwordBuffer, sizeof(passwordBuffer), 30000)) {
            if (systemController.enterDeveloperMode(passwordBuffer)) {
                Serial.println("✅ 개발자 모드");
                uiManager.showToast("개발자 모드", TFT_YELLOW);
            } else {
                Serial.println("❌ 비밀번호 오류");
                systemController.recordFailedLogin();
                
                if (systemController.isLockedOut()) {
                    Serial.println("🔒 잠금 - 1분");
                }
            }
        } else {
            Serial.println("⏱️ 타임아웃");
        }
        
        // 보안: 비밀번호 버퍼 초기화
        memset(passwordBuffer, 0, sizeof(passwordBuffer));
        return;
    }
}

void CommandHandler::handleSerialCommands() {
    if (!Serial.available()) return;

    // char 배열로 명령어 읽기 (String 사용 안 함)
    if (!readCommand(cmdBuffer, sizeof(cmdBuffer))) {
        return;
    }
    
    toLowerString(cmdBuffer);
    
    Serial.printf("\n[CMD] '%s'\n", cmdBuffer);

    // 명령어 히스토리에 추가
    commandHistory.add(cmdBuffer);

    // 모드 전환 명령어 체크
    if (streq(cmdBuffer, "operator") || streq(cmdBuffer, "logout") || 
        streq(cmdBuffer, "manager") || streq(cmdBuffer, "developer") || 
        streq(cmdBuffer, "dev")) {
        handleModeSwitch(cmdBuffer);
        return;
    }

    // 히스토리 명령어
    if (streq(cmdBuffer, "history")) {
        commandHistory.print();
        return;
    }

    // 권한 기반 명령어 처리
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
        controlManager.start();
        Serial.println("✅ 시스템 시작");
    }
    else if (streq(cmd, "stop")) {
        controlManager.stop();
        Serial.println("✅ 시스템 정지");
    }
    else if (streq(cmd, "pause")) {
        controlManager.pause();
        Serial.println("✅ 일시정지");
    }
    else if (streq(cmd, "status")) {
        Serial.println("\n=== 시스템 상태 ===");
        sensorManager.printStatus();
        controlManager.printStatus();
        Serial.println("==================\n");
    }
    else if (streq(cmd, "sensor")) {
        sensorManager.printStatus();
    }
    else if (streq(cmd, "help") || streq(cmd, "?")) {
        Serial.println("\n╔════════════════════════════╗");
        Serial.println("║     작업자 모드 명령어     ║");
        Serial.println("╚════════════════════════════╝");
        Serial.println("  start    - 시작");
        Serial.println("  stop     - 정지");
        Serial.println("  pause    - 일시정지");
        Serial.println("  status   - 상태");
        Serial.println("  sensor   - 센서");
        Serial.println("  history  - 명령 히스토리");
        Serial.println("  manager  - 관리자 모드");
        Serial.println();
    }
    else {
        Serial.printf("❌ 알 수 없는 명령어: '%s'\n", cmd);
        Serial.println("💡 'help' 입력");
    }
}

bool CommandHandler::handleManagerCommands(const char* cmd) {
    if (streq(cmd, "calibrate")) {
        Serial.println("센서 캘리브레이션...");
        sensorManager.calibratePressure();
        sensorManager.calibrateCurrent();
        Serial.println("✅ 완료");
        return true;
    }
    else if (streq(cmd, "config_save")) {
        configManager.saveConfig();
        Serial.println("✅ 설정 저장");
        return true;
    }
    else if (streq(cmd, "network_status")) {
        networkManager.printStatus();
        return true;
    }
    else if (streq(cmd, "help_manager")) {
        Serial.println("\n╔════════════════════════════╗");
        Serial.println("║     관리자 모드 명령어     ║");
        Serial.println("╚════════════════════════════╝");
        Serial.println("  calibrate     - 캘리브레이션");
        Serial.println("  config_save   - 설정 저장");
        Serial.println("  network_status- 네트워크");
        Serial.println("  developer     - 개발자 모드");
        Serial.println();
        return true;
    }
    
    return false;
}

bool CommandHandler::handleDeveloperCommands(const char* cmd) {
    if (streq(cmd, "test_all")) {
        Serial.println("\n전체 테스트...\n");
        systemTest.runAllTests();
        return true;
    }
    else if (streq(cmd, "version")) {
        Serial.println("\n╔════════════════════════════╗");
        Serial.println("║  ESP32-S3 진공 제어 시스템  ║");
        Serial.println("╠════════════════════════════╣");
        Serial.println("║  버전: v3.9.3 String최적화 ║");
        Serial.println("║  빌드: 2024.12 (Optimized) ║");
        Serial.println("╚════════════════════════════╝\n");
        return true;
    }
    else if (streq(cmd, "help_dev")) {
        Serial.println("\n╔════════════════════════════╗");
        Serial.println("║     개발자 모드 명령어     ║");
        Serial.println("╚════════════════════════════╝");
        Serial.println("  test_all - 전체 테스트");
        Serial.println("  version  - 버전");
        Serial.println("  history  - 명령 히스토리");
        Serial.println();
        return true;
    }
    
    return false;
}
