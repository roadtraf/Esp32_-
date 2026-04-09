// ================================================================
// SerialCommands.cpp -    
// ESP32-S3 v3.9.3 - String 
// ================================================================
#include "SerialCommands.h"
#include "Config.h"
#include "EnhancedWatchdog.h"
#include "ConfigManager.h"
#include <cstring>
#include <cctype>

// FreeRTOS (delay )
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

//  
extern SystemConfig config;
extern void saveConfig();
extern void connectWiFi();
extern void connectMQTT();

// ================================================================
//    
// ================================================================
//  :   
static void sc_trim(char* str) {
    if (!str || !*str) return;
    char* s = str;
    while (*s && isspace((unsigned char)*s)) s++;
    char* e = str + strlen(str) - 1;
    while (e > s && isspace((unsigned char)*e)) e--;
    *(e + 1) = '\0';
    if (s != str) memmove(str, s, strlen(s) + 1);
}

//  :  
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

    //   
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
        Serial.printf("    : %s\n", cmd);
        Serial.println("   'help'     ");
    }
}

// ================================================================
//  
// ================================================================
void handleSystemCommands(const char* cmd) {
    if (strcmp(cmd, "status") == 0 || strcmp(cmd, "info") == 0 || strcmp(cmd, "sys_info") == 0) {
        Serial.println("\n");
        Serial.println("                                         ");
        Serial.println("");
        Serial.printf(" : v3.9.2 Phase 3-1                            \n");
        Serial.printf(" Chip: %s                                          \n", ESP.getChipModel());
        Serial.printf(" CPU: %d MHz                                       \n", ESP.getCpuFreqMHz());
        Serial.printf(" Free Heap: %d bytes                               \n", ESP.getFreeHeap());
        Serial.printf(" Flash: %d bytes                                   \n", ESP.getFlashChipSize());
        Serial.printf(" Uptime: %lu sec                                   \n", millis() / 1000);
        Serial.println("\n");
    }
    else if (strcmp(cmd, "sys_restart") == 0 || strcmp(cmd, "restart") == 0 || strcmp(cmd, "reboot") == 0) {
        Serial.println("  ...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP.restart();
    }
    else if (strcmp(cmd, "sys_memory") == 0 || strcmp(cmd, "memory") == 0 || strcmp(cmd, "mem") == 0) {
        Serial.println("\n");
        Serial.println("                             ");
        Serial.println("");
        Serial.printf(" Total Heap: %d bytes                  \n", ESP.getHeapSize());
        Serial.printf(" Free Heap: %d bytes                   \n", ESP.getFreeHeap());
        Serial.printf(" Min Free Heap: %d bytes               \n", ESP.getMinFreeHeap());
        Serial.printf(" Max Alloc: %d bytes                   \n", ESP.getMaxAllocHeap());
        Serial.println("\n");
    }
}

// ================================================================
// Enhanced Watchdog 
// ================================================================
void handleWatchdogCommands(const char* cmd) {
    if (strcmp(cmd, "wdt") == 0 || strcmp(cmd, "wdt_status") == 0) {
        enhancedWatchdog.printStatus();
    }
    else if (strncmp(cmd, "wdt_task ", 9) == 0) {
        // "wdt_task "    
        const char* taskName = cmd + 9;
        enhancedWatchdog.printTaskDetails(taskName);
    }
    else if (strcmp(cmd, "wdt_history") == 0 || strcmp(cmd, "wdt_restart") == 0) {
        enhancedWatchdog.printRestartHistory();
    }
    else if (strcmp(cmd, "wdt_enable") == 0) {
        enhancedWatchdog.enable();
        Serial.println(" Enhanced Watchdog ");
    }
    else if (strcmp(cmd, "wdt_disable") == 0) {
        enhancedWatchdog.disable();
        Serial.println("  Enhanced Watchdog ");
    }
    else if (strcmp(cmd, "wdt_help") == 0 || strcmp(cmd, "?wdt") == 0) {
        Serial.println("\n");
        Serial.println("   Enhanced Watchdog             ");
        Serial.println("");
        Serial.println(" wdt_status       - Watchdog       ");
        Serial.println(" wdt_task <>  -          ");
        Serial.println(" wdt_history      -      ");
        Serial.println(" wdt_enable       -              ");
        Serial.println(" wdt_disable      -            ");
        Serial.println("");
        Serial.println(" : wdt_task VacuumCtrl              ");
        Serial.println("\n");
    }
    else {
        Serial.println("    Watchdog ");
        Serial.println("   'wdt_help'   ");
    }
}

// ================================================================
// ConfigManager 
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
            Serial.println("   ");
        } else {
            Serial.println("   ");
        }
    }
    else if (strcmp(cmd, "config_restore") == 0 || strcmp(cmd, "cfg_restore") == 0 || strcmp(cmd, "restore") == 0) {
        Serial.println("\n   ?");
        Serial.println("    'yes'  (10 )");
        
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
                    Serial.println("");
                    return;
                }
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        if (!confirmed) {
            Serial.println("  - ");
            return;
        }
        
        //  
        SystemConfig tempConfig;
        if (configManager.restoreFromBackup(&tempConfig, sizeof(tempConfig)) == CONFIG_OK) {
            config = tempConfig;
            configManager.saveConfig(&config, sizeof(config), false);
            Serial.println("   . ...");
            vTaskDelay(pdMS_TO_TICKS(2000));
            ESP.restart();
        } else {
            Serial.println("   ");
        }
    }
    else if (strcmp(cmd, "config_factory") == 0 || strcmp(cmd, "cfg_factory") == 0 || strcmp(cmd, "factory") == 0) {
        Serial.println("\n    ?");
        Serial.println("     !");
        Serial.println("    'yes'  (10 )");
        
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
                    Serial.println("");
                    return;
                }
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        if (!confirmed) {
            Serial.println("  - ");
            return;
        }
        
        //  
        SystemConfig tempConfig;
        if (configManager.restoreFromFactory(&tempConfig, sizeof(tempConfig)) == CONFIG_OK) {
            config = tempConfig;
            configManager.saveConfig(&config, sizeof(config), false);
            Serial.println("   . ...");
            vTaskDelay(pdMS_TO_TICKS(2000));
            ESP.restart();
        } else {
            Serial.println("   ");
        }
    }
    else if (strcmp(cmd, "config_save") == 0 || strcmp(cmd, "cfg_save") == 0 || strcmp(cmd, "save") == 0) {
        saveConfig();
        Serial.println("   ");
    }
    else if (strcmp(cmd, "config_help") == 0 || strcmp(cmd, "?cfg") == 0) {
        Serial.println("\n");
        Serial.println("   ConfigManager                 ");
        Serial.println("");
        Serial.println(" config_status    -            ");
        Serial.println(" config_stats     -            ");
        Serial.println(" config_backup    -            ");
        Serial.println(" config_restore   -            ");
        Serial.println(" config_factory   -          ");
        Serial.println(" config_save      -        ");
        Serial.println("\n");
    }
    else {
        Serial.println("    Config ");
        Serial.println("   'config_help'   ");
    }
}

// ================================================================
//  
// ================================================================
void handleNetworkCommands(const char* cmd) {
    if (strcmp(cmd, "wifi_status") == 0 || strcmp(cmd, "wifi") == 0) {
        Serial.println("\n");
        Serial.println("       WiFi                        ");
        Serial.println("");
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println(" :                        ");
            Serial.printf(" SSID: %-31s \n", WiFi.SSID().c_str());
            char ipBuf[16];
            WiFi.localIP().toString().toCharArray(ipBuf, sizeof(ipBuf));
            Serial.printf(" IP: %-33s \n", ipBuf);
            Serial.printf(" RSSI: %d dBm                          \n", WiFi.RSSI());
        } else {
            Serial.println(" :                     ");
        }
        Serial.println("\n");
    }
    else if (strcmp(cmd, "wifi_connect") == 0 || strcmp(cmd, "wifi_reconnect") == 0) {
        Serial.println("WiFi  ...");
        connectWiFi();
    }
    else if (strcmp(cmd, "wifi_disconnect") == 0) {
        WiFi.disconnect();
        Serial.println(" WiFi  ");
    }
    else if (strcmp(cmd, "mqtt_status") == 0 || strcmp(cmd, "mqtt") == 0) {
        extern bool mqttConnected;
        Serial.println("\n");
        Serial.println("       MQTT                        ");
        Serial.println("");
        Serial.printf(" : %s                              \n",
                      mqttConnected ? " " : "  ");
        Serial.println("\n");
    }
    else if (strcmp(cmd, "mqtt_connect") == 0 || strcmp(cmd, "mqtt_reconnect") == 0) {
        Serial.println("MQTT  ...");
        connectMQTT();
    }
    else if (strcmp(cmd, "net_scan") == 0 || strcmp(cmd, "wifi_scan") == 0) {
        Serial.println("WiFi  ...");
        int n = WiFi.scanNetworks();
        Serial.printf("\n : %d\n\n", n);
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
        Serial.println("     ");
    }
}

// ================================================================
//  
// ================================================================
void handleSensorCommands(const char* cmd) {
    extern SensorData sensorData;
    
    if (strcmp(cmd, "sensor_read") == 0 || strcmp(cmd, "read") == 0 || strcmp(cmd, "sensor") == 0) {
        Serial.println("\n");
        Serial.println("                                 ");
        Serial.println("");
        Serial.printf(" : %.1f kPa                        \n", sensorData.pressure);
        Serial.printf(" : %.1f C                         \n", sensorData.temperature);
        Serial.printf(" : %.2f A                          \n", sensorData.current);
        Serial.println("\n");
    }
    else {
        Serial.println("     ");
    }
}

// ================================================================
//  
// ================================================================
void handleControlCommands(const char* cmd) {
    extern bool pumpActive;
    extern bool valveActive;
    
    if (strcmp(cmd, "control_status") == 0 || strcmp(cmd, "control") == 0) {
        Serial.println("\n");
        Serial.println("                               ");
        Serial.println("");
        Serial.printf(" : %s                              \n", pumpActive ? " ON" : " OFF");
        Serial.printf(" : %s                              \n", valveActive ? " ON" : " OFF");
        Serial.println("\n");
    }
    else {
        Serial.println("     ");
    }
}

// ================================================================
//  
// ================================================================
void handleDebugCommands(const char* cmd) {
    if (strcmp(cmd, "debug_heap") == 0 || strcmp(cmd, "heap") == 0) {
        Serial.println("\n");
        Serial.println("                         ");
        Serial.println("");
        Serial.printf(" Free: %d bytes                        \n", ESP.getFreeHeap());
        Serial.printf(" Min Free: %d bytes                    \n", ESP.getMinFreeHeap());
        Serial.printf(" Max Alloc: %d bytes                   \n", ESP.getMaxAllocHeap());
        void* ptr = malloc(1024);
        if (ptr) {
            Serial.println(" 1KB  :              ");
            free(ptr);
        } else {
            Serial.println(" 1KB  :              ");
        }
        Serial.println("\n");
    }
    else if (strcmp(cmd, "debug_tasks") == 0 || strcmp(cmd, "tasks") == 0) {
        Serial.println("\nFreeRTOS  :");
        Serial.println("(TaskConfig.h  )");
        Serial.println("'wdt_status'    \n");
    }
    else {
        Serial.println("     ");
    }
}

// ================================================================
// 
// ================================================================
void showHelp() {
    Serial.println("\n");
    Serial.println("     ESP32-S3    v3.9.2 Phase 3-1   ");
    Serial.println("                                 ");
    Serial.println("");
    Serial.println("                                                   ");
    Serial.println("                                            ");
    Serial.println("   status         -                      ");
    Serial.println("   memory         -                      ");
    Serial.println("   restart        -                          ");
    Serial.println("                                                   ");
    Serial.println("  Enhanced Watchdog                               ");
    Serial.println("   wdt_status     - Watchdog                   ");
    Serial.println("   wdt_task <>-                  ");
    Serial.println("   wdt_history    -                  ");
    Serial.println("   wdt_help       - Watchdog                 ");
    Serial.println("                                                   ");
    Serial.println("                                          ");
    Serial.println("   config_status  -                        ");
    Serial.println("   config_backup  -                        ");
    Serial.println("   config_restore -                        ");
    Serial.println("   config_factory -                      ");
    Serial.println("   config_help    - Config                   ");
    Serial.println("                                                   ");
    Serial.println("                                          ");
    Serial.println("   wifi_status    - WiFi                       ");
    Serial.println("   wifi_scan      - WiFi                       ");
    Serial.println("   mqtt_status    - MQTT                       ");
    Serial.println("                                                   ");
    Serial.println("  /                                       ");
    Serial.println("   sensor_read    -                      ");
    Serial.println("   control_status -                        ");
    Serial.println("                                                   ");
    Serial.println("                                            ");
    Serial.println("   debug_heap     -                  ");
    Serial.println("   debug_tasks    -                      ");
    Serial.println("                                                   ");
    Serial.println("\n");
}
