// ================================================================
// SerialCommands.cpp - 시리얼 명령어 처리 구현
// ESP32-S3 v3.9.3 - String 최적화
// ================================================================
#include "SerialCommands.h"
#include "../include/Config.h"
#include "../include/EnhancedWatchdog.h"
#include "../include/ConfigManager.h"
#include <cstring>
#include <cctype>

// FreeRTOS (delay 개선)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 외부 참조
extern SystemConfig config;
extern void saveConfig();
extern void connectWiFi();
extern void connectMQTT();

// ================================================================
// 메인 명령어 처리 함수
// ================================================================
// 내부 유틸리티: 앞뒤 공백 제거
static void sc_trim(char* str) {
    if (!str || !*str) return;
    char* s = str;
    while (*s && isspace((unsigned char)*s)) s++;
    char* e = str + strlen(str) - 1;
    while (e > s && isspace((unsigned char)*e)) e--;
    *(e + 1) = '\0';
    if (s != str) memmove(str, s, strlen(s) + 1);
}

// 내부 유틸리티: 소문자 변환
static void sc_lower(char* str) {
    for (int i = 0; str[i]; i++)
        str[i] = tolower((unsigned char)str[i]);
}

void processSerialCommands() {
    if (!Serial.available()) return;

    char cmd[SERIAL_CMD_BUFFER_SIZE];
    int len = Serial.readBytesUntil('\n', cmd, sizeof(cmd) - 1);
    if (len == 0) return;
    cmd[len] = '\0';
    sc_trim(cmd);
    sc_lower(cmd);

    if (strlen(cmd) == 0) return;

    // 명령어 그룹별 분기
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        showHelp();
    }
    else if (strncmp(cmd, "wdt", 3) == 0) {
        handleWatchdogCommands(cmd);
    }
    else if (strncmp(cmd, "config", 6) == 0 || strncmp(cmd, "cfg", 3) == 0) {
        handleConfigCommands(cmd);
    }
    else if (strncmp(cmd, "wifi", 4) == 0 || strncmp(cmd, "mqtt", 4) == 0 || strncmp(cmd, "net", 3) == 0) {
        handleNetworkCommands(cmd);
    }
    else if (strncmp(cmd, "sensor", 6) == 0 || strncmp(cmd, "read", 4) == 0) {
        handleSensorCommands(cmd);
    }
    else if (strncmp(cmd, "control", 7) == 0 || strncmp(cmd, "vacuum", 6) == 0 || strncmp(cmd, "pump", 4) == 0) {
        handleControlCommands(cmd);
    }
    else if (strncmp(cmd, "debug", 5) == 0 || strncmp(cmd, "test", 4) == 0) {
        handleDebugCommands(cmd);
    }
    else if (strncmp(cmd, "sys", 3) == 0 || strcmp(cmd, "status") == 0 || strcmp(cmd, "info") == 0) {
        handleSystemCommands(cmd);
    }
    else {
        Serial.printf("❓ 알 수 없는 명령어: %s\n", cmd);
        Serial.println("   'help' 입력하여 사용 가능한 명령어 확인");
    }
}

// ================================================================
// 시스템 명령어
// ================================================================
void handleSystemCommands(const char* cmd) {
    if (strcmp(cmd, "status") == 0 || strcmp(cmd, "info") == 0 || strcmp(cmd, "sys_info") == 0) {
        Serial.println("\n╔═══════════════════════════════════════════════════╗");
        Serial.println("║         시스템 정보                               ║");
        Serial.println("╠═══════════════════════════════════════════════════╣");
        Serial.printf("║ 버전: v3.9.2 Phase 3-1                            ║\n");
        Serial.printf("║ Chip: %s                                          ║\n", ESP.getChipModel());
        Serial.printf("║ CPU: %d MHz                                       ║\n", ESP.getCpuFreqMHz());
        Serial.printf("║ Free Heap: %d bytes                               ║\n", ESP.getFreeHeap());
        Serial.printf("║ Flash: %d bytes                                   ║\n", ESP.getFlashChipSize());
        Serial.printf("║ Uptime: %lu sec                                   ║\n", millis() / 1000);
        Serial.println("╚═══════════════════════════════════════════════════╝\n");
    }
    else if (strcmp(cmd, "sys_restart") == 0 || strcmp(cmd, "restart") == 0 || strcmp(cmd, "reboot") == 0) {
        Serial.println("⚠️  재시작합니다...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP.restart();
    }
    else if (strcmp(cmd, "sys_memory") == 0 || strcmp(cmd, "memory") == 0 || strcmp(cmd, "mem") == 0) {
        Serial.println("\n╔═══════════════════════════════════════╗");
        Serial.println("║       메모리 정보                     ║");
        Serial.println("╠═══════════════════════════════════════╣");
        Serial.printf("║ Total Heap: %d bytes                  ║\n", ESP.getHeapSize());
        Serial.printf("║ Free Heap: %d bytes                   ║\n", ESP.getFreeHeap());
        Serial.printf("║ Min Free Heap: %d bytes               ║\n", ESP.getMinFreeHeap());
        Serial.printf("║ Max Alloc: %d bytes                   ║\n", ESP.getMaxAllocHeap());
        Serial.println("╚═══════════════════════════════════════╝\n");
    }
}

// ================================================================
// Enhanced Watchdog 명령어
// ================================================================
void handleWatchdogCommands(const char* cmd) {
    if (strcmp(cmd, "wdt") == 0 || strcmp(cmd, "wdt_status") == 0) {
        enhancedWatchdog.printStatus();
    }
    else if (strncmp(cmd, "wdt_task ", 9) == 0) {
        // "wdt_task " 이후 태스크 이름 추출
        const char* taskName = cmd + 9;
        enhancedWatchdog.printTaskDetails(taskName);
    }
    else if (strcmp(cmd, "wdt_history") == 0 || strcmp(cmd, "wdt_restart") == 0) {
        enhancedWatchdog.printRestartHistory();
    }
    else if (strcmp(cmd, "wdt_enable") == 0) {
        enhancedWatchdog.enable();
        Serial.println("✅ Enhanced Watchdog 활성화");
    }
    else if (strcmp(cmd, "wdt_disable") == 0) {
        enhancedWatchdog.disable();
        Serial.println("⚠️  Enhanced Watchdog 비활성화");
    }
    else if (strcmp(cmd, "wdt_help") == 0 || strcmp(cmd, "?wdt") == 0) {
        Serial.println("\n╔═══════════════════════════════════════╗");
        Serial.println("║   Enhanced Watchdog 명령어            ║");
        Serial.println("╠═══════════════════════════════════════╣");
        Serial.println("║ wdt_status       - Watchdog 상태      ║");
        Serial.println("║ wdt_task <이름>  - 태스크 상세        ║");
        Serial.println("║ wdt_history      - 재시작 히스토리    ║");
        Serial.println("║ wdt_enable       - 활성화             ║");
        Serial.println("║ wdt_disable      - 비활성화           ║");
        Serial.println("╠═══════════════════════════════════════╣");
        Serial.println("║ 예: wdt_task VacuumCtrl              ║");
        Serial.println("╚═══════════════════════════════════════╝\n");
    }
    else {
        Serial.println("❓ 알 수 없는 Watchdog 명령어");
        Serial.println("   'wdt_help' 입력하여 도움말 확인");
    }
}

// ================================================================
// ConfigManager 명령어
// ================================================================
void handleConfigCommands(const char* cmd) {
    if (strcmp(cmd, "config_status") == 0 || strcmp(cmd, "cfg_status") == 0 || strcmp(cmd, "cfg") == 0) {
        configManager.printStatus();
    }
    else if (strcmp(cmd, "config_stats") == 0 || strcmp(cmd, "cfg_stats") == 0) {
        configManager.printStats();
    }
    else if (strcmp(cmd, "config_backup") == 0 || strcmp(cmd, "cfg_backup") == 0 || strcmp(cmd, "backup") == 0) {
        if (configManager.createBackup()) {
            Serial.println("✅ 백업 생성 완료");
        } else {
            Serial.println("❌ 백업 생성 실패");
        }
    }
    else if (strcmp(cmd, "config_restore") == 0 || strcmp(cmd, "cfg_restore") == 0 || strcmp(cmd, "restore") == 0) {
        Serial.println("\n⚠️  백업에서 복원하시겠습니까?");
        Serial.println("   복원하려면 'yes' 입력 (10초 대기)");
        
        uint32_t startTime = millis();
        bool confirmed = false;
        
        while (millis() - startTime < 10000) {
            if (Serial.available()) {
                char confirm[8];
                int clen = Serial.readBytesUntil('\n', confirm, sizeof(confirm) - 1);
                confirm[clen] = '\0';
                sc_trim(confirm);
                sc_lower(confirm);
                
                if (strcmp(confirm, "yes") == 0 || strcmp(confirm, "y") == 0) {
                    confirmed = true;
                    break;
                } else {
                    Serial.println("취소됨");
                    return;
                }
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        if (!confirmed) {
            Serial.println("시간 초과 - 취소됨");
            return;
        }
        
        // 백업 복원
        SystemConfig tempConfig;
        if (configManager.restoreFromBackup(&tempConfig, sizeof(tempConfig)) == CONFIG_OK) {
            config = tempConfig;
            configManager.saveConfig(&config, sizeof(config), false);
            Serial.println("✅ 백업 복원 완료. 재시작합니다...");
            vTaskDelay(pdMS_TO_TICKS(2000));
            ESP.restart();
        } else {
            Serial.println("❌ 백업 복원 실패");
        }
    }
    else if (strcmp(cmd, "config_factory") == 0 || strcmp(cmd, "cfg_factory") == 0 || strcmp(cmd, "factory") == 0) {
        Serial.println("\n⚠️  공장 초기화를 진행하시겠습니까?");
        Serial.println("   모든 설정이 초기화됩니다!");
        Serial.println("   진행하려면 'yes' 입력 (10초 대기)");
        
        uint32_t startTime = millis();
        bool confirmed = false;
        
        while (millis() - startTime < 10000) {
            if (Serial.available()) {
                char confirm[8];
                int clen = Serial.readBytesUntil('\n', confirm, sizeof(confirm) - 1);
                confirm[clen] = '\0';
                sc_trim(confirm);
                sc_lower(confirm);
                
                if (strcmp(confirm, "yes") == 0 || strcmp(confirm, "y") == 0) {
                    confirmed = true;
                    break;
                } else {
                    Serial.println("취소됨");
                    return;
                }
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        if (!confirmed) {
            Serial.println("시간 초과 - 취소됨");
            return;
        }
        
        // 공장 초기화
        SystemConfig tempConfig;
        if (configManager.restoreFromFactory(&tempConfig, sizeof(tempConfig)) == CONFIG_OK) {
            config = tempConfig;
            configManager.saveConfig(&config, sizeof(config), false);
            Serial.println("✅ 공장 초기화 완료. 재시작합니다...");
            vTaskDelay(pdMS_TO_TICKS(2000));
            ESP.restart();
        } else {
            Serial.println("❌ 공장 초기화 실패");
        }
    }
    else if (strcmp(cmd, "config_save") == 0 || strcmp(cmd, "cfg_save") == 0 || strcmp(cmd, "save") == 0) {
        saveConfig();
        Serial.println("✅ 설정 저장 완료");
    }
    else if (strcmp(cmd, "config_help") == 0 || strcmp(cmd, "?cfg") == 0) {
        Serial.println("\n╔═══════════════════════════════════════╗");
        Serial.println("║   ConfigManager 명령어                ║");
        Serial.println("╠═══════════════════════════════════════╣");
        Serial.println("║ config_status    - 설정 상태          ║");
        Serial.println("║ config_stats     - 설정 통계          ║");
        Serial.println("║ config_backup    - 백업 생성          ║");
        Serial.println("║ config_restore   - 백업 복원          ║");
        Serial.println("║ config_factory   - 공장 초기화        ║");
        Serial.println("║ config_save      - 현재 설정 저장     ║");
        Serial.println("╚═══════════════════════════════════════╝\n");
    }
    else {
        Serial.println("❓ 알 수 없는 Config 명령어");
        Serial.println("   'config_help' 입력하여 도움말 확인");
    }
}

// ================================================================
// 네트워크 명령어
// ================================================================
void handleNetworkCommands(const char* cmd) {
    if (strcmp(cmd, "wifi_status") == 0 || strcmp(cmd, "wifi") == 0) {
        Serial.println("\n╔═══════════════════════════════════════╗");
        Serial.println("║       WiFi 상태                       ║");
        Serial.println("╠═══════════════════════════════════════╣");
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("║ 상태: ✅ 연결됨                      ║");
            Serial.printf("║ SSID: %-31s ║\n", WiFi.SSID().c_str());
            char ipBuf[16];
            WiFi.localIP().toString().toCharArray(ipBuf, sizeof(ipBuf));
            Serial.printf("║ IP: %-33s ║\n", ipBuf);
            Serial.printf("║ RSSI: %d dBm                          ║\n", WiFi.RSSI());
        } else {
            Serial.println("║ 상태: ❌ 연결 끊김                  ║");
        }
        Serial.println("╚═══════════════════════════════════════╝\n");
    }
    else if (strcmp(cmd, "wifi_connect") == 0 || strcmp(cmd, "wifi_reconnect") == 0) {
        Serial.println("WiFi 재연결 시도...");
        connectWiFi();
    }
    else if (strcmp(cmd, "wifi_disconnect") == 0) {
        WiFi.disconnect();
        Serial.println("✅ WiFi 연결 해제");
    }
    else if (strcmp(cmd, "mqtt_status") == 0 || strcmp(cmd, "mqtt") == 0) {
        extern bool mqttConnected;
        Serial.println("\n╔═══════════════════════════════════════╗");
        Serial.println("║       MQTT 상태                       ║");
        Serial.println("╠═══════════════════════════════════════╣");
        Serial.printf("║ 상태: %s                              ║\n",
                      mqttConnected ? "✅ 연결됨" : "❌ 연결 끊김");
        Serial.println("╚═══════════════════════════════════════╝\n");
    }
    else if (strcmp(cmd, "mqtt_connect") == 0 || strcmp(cmd, "mqtt_reconnect") == 0) {
        Serial.println("MQTT 재연결 시도...");
        connectMQTT();
    }
    else if (strcmp(cmd, "net_scan") == 0 || strcmp(cmd, "wifi_scan") == 0) {
        Serial.println("WiFi 스캔 중...");
        int n = WiFi.scanNetworks();
        Serial.printf("\n발견된 네트워크: %d개\n\n", n);
        for (int i = 0; i < n; i++) {
            Serial.printf("%d: %s (%d dBm) %s\n",
                          i + 1,
                          WiFi.SSID(i).c_str(),
                          WiFi.RSSI(i),
                          WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Encrypted");
        }
        Serial.println();
    }
    else {
        Serial.println("❓ 알 수 없는 네트워크 명령어");
    }
}

// ================================================================
// 센서 명령어
// ================================================================
void handleSensorCommands(const char* cmd) {
    extern SensorData sensorData;
    
    if (strcmp(cmd, "sensor_read") == 0 || strcmp(cmd, "read") == 0 || strcmp(cmd, "sensor") == 0) {
        Serial.println("\n╔═══════════════════════════════════════╗");
        Serial.println("║       센서 값                         ║");
        Serial.println("╠═══════════════════════════════════════╣");
        Serial.printf("║ 압력: %.1f kPa                        ║\n", sensorData.pressure);
        Serial.printf("║ 온도: %.1f °C                         ║\n", sensorData.temperature);
        Serial.printf("║ 전류: %.2f A                          ║\n", sensorData.current);
        Serial.println("╚═══════════════════════════════════════╝\n");
    }
    else {
        Serial.println("❓ 알 수 없는 센서 명령어");
    }
}

// ================================================================
// 제어 명령어
// ================================================================
void handleControlCommands(const char* cmd) {
    extern bool pumpActive;
    extern bool valveActive;
    
    if (strcmp(cmd, "control_status") == 0 || strcmp(cmd, "control") == 0) {
        Serial.println("\n╔═══════════════════════════════════════╗");
        Serial.println("║       제어 상태                       ║");
        Serial.println("╠═══════════════════════════════════════╣");
        Serial.printf("║ 펌프: %s                              ║\n", pumpActive ? "✅ ON" : "❌ OFF");
        Serial.printf("║ 밸브: %s                              ║\n", valveActive ? "✅ ON" : "❌ OFF");
        Serial.println("╚═══════════════════════════════════════╝\n");
    }
    else {
        Serial.println("❓ 알 수 없는 제어 명령어");
    }
}

// ================================================================
// 디버그 명령어
// ================================================================
void handleDebugCommands(const char* cmd) {
    if (strcmp(cmd, "debug_heap") == 0 || strcmp(cmd, "heap") == 0) {
        Serial.println("\n╔═══════════════════════════════════════╗");
        Serial.println("║       힙 메모리 디버그                ║");
        Serial.println("╠═══════════════════════════════════════╣");
        Serial.printf("║ Free: %d bytes                        ║\n", ESP.getFreeHeap());
        Serial.printf("║ Min Free: %d bytes                    ║\n", ESP.getMinFreeHeap());
        Serial.printf("║ Max Alloc: %d bytes                   ║\n", ESP.getMaxAllocHeap());
        void* ptr = malloc(1024);
        if (ptr) {
            Serial.println("║ 1KB 할당 테스트: ✅ 성공            ║");
            free(ptr);
        } else {
            Serial.println("║ 1KB 할당 테스트: ❌ 실패            ║");
        }
        Serial.println("╚═══════════════════════════════════════╝\n");
    }
    else if (strcmp(cmd, "debug_tasks") == 0 || strcmp(cmd, "tasks") == 0) {
        Serial.println("\nFreeRTOS 태스크 목록:");
        Serial.println("(TaskConfig.h에서 설정된 태스크들)");
        Serial.println("'wdt_status' 명령어로 태스크 상태 확인\n");
    }
    else {
        Serial.println("❓ 알 수 없는 디버그 명령어");
    }
}

// ================================================================
// 도움말
// ================================================================
void showHelp() {
    Serial.println("\n╔═══════════════════════════════════════════════════╗");
    Serial.println("║     ESP32-S3 진공 제어 시스템 v3.9.2 Phase 3-1   ║");
    Serial.println("║              시리얼 명령어 도움말                 ║");
    Serial.println("╠═══════════════════════════════════════════════════╣");
    Serial.println("║                                                   ║");
    Serial.println("║ ▶ 시스템                                          ║");
    Serial.println("║   status         - 시스템 정보                    ║");
    Serial.println("║   memory         - 메모리 정보                    ║");
    Serial.println("║   restart        - 재시작                         ║");
    Serial.println("║                                                   ║");
    Serial.println("║ ▶ Enhanced Watchdog                               ║");
    Serial.println("║   wdt_status     - Watchdog 상태                  ║");
    Serial.println("║   wdt_task <이름>- 태스크 상세 정보               ║");
    Serial.println("║   wdt_history    - 재시작 히스토리                ║");
    Serial.println("║   wdt_help       - Watchdog 도움말                ║");
    Serial.println("║                                                   ║");
    Serial.println("║ ▶ 설정 관리                                       ║");
    Serial.println("║   config_status  - 설정 상태                      ║");
    Serial.println("║   config_backup  - 백업 생성                      ║");
    Serial.println("║   config_restore - 백업 복원                      ║");
    Serial.println("║   config_factory - 공장 초기화                    ║");
    Serial.println("║   config_help    - Config 도움말                  ║");
    Serial.println("║                                                   ║");
    Serial.println("║ ▶ 네트워크                                        ║");
    Serial.println("║   wifi_status    - WiFi 상태                      ║");
    Serial.println("║   wifi_scan      - WiFi 스캔                      ║");
    Serial.println("║   mqtt_status    - MQTT 상태                      ║");
    Serial.println("║                                                   ║");
    Serial.println("║ ▶ 센서/제어                                       ║");
    Serial.println("║   sensor_read    - 센서 값 읽기                   ║");
    Serial.println("║   control_status - 제어 상태                      ║");
    Serial.println("║                                                   ║");
    Serial.println("║ ▶ 디버그                                          ║");
    Serial.println("║   debug_heap     - 힙 메모리 디버그               ║");
    Serial.println("║   debug_tasks    - 태스크 목록                    ║");
    Serial.println("║                                                   ║");
    Serial.println("╚═══════════════════════════════════════════════════╝\n");
}
