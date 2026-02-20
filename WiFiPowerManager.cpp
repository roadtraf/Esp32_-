// WiFiPowerManager.cpp
#include "WiFiPowerManager.h"

WiFiPowerManager wifiPowerManager;

WiFiPowerManager::WiFiPowerManager()
    : currentMode(WiFiPowerMode::BALANCED),
      activityLevel(WiFiActivityLevel::IDLE),
      lastActivityTime(0),
      lastModeChange(0),
      idleStartTime(0),
      txPackets(0),
      rxPackets(0),
      lastTxPackets(0),
      lastRxPackets(0),
      isConnected(false),
      powerSaveEnabled(false),
      currentTxPower(20),
      modemSleepCount(0),
      lightSleepCount(0),
      totalSleepTime(0)
{}

void WiFiPowerManager::begin(const WiFiPowerConfig& cfg) {
    config = cfg;
    currentMode = config.mode;
    
    Serial.println("[WiFiPowerManager] 초기화 시작");
    
    // Apply initial power mode
    applyPowerMode(currentMode);
    
    // Configure initial TX power
    setTxPower(config.maxTxPower);
    
    Serial.printf("[WiFiPowerManager] 모드: %d, TX Power: %d dBm\n", 
                  (int)currentMode, currentTxPower);
}

void WiFiPowerManager::update() {
    uint32_t now = millis();
    
    // Update activity level
    updateActivityLevel();
    
    // Check connection state
    if (WiFi.status() == WL_CONNECTED) {
        if (!isConnected) {
            setConnected(true);
        }
    } else {
        if (isConnected) {
            setConnected(false);
        }
        return; // Don't process power management when disconnected
    }
    
    // Auto power mode adjustment based on activity
    if (currentMode == WiFiPowerMode::BALANCED) {
        uint32_t idleDuration = now - lastActivityTime;
        
        switch (activityLevel) {
            case WiFiActivityLevel::IDLE:
                if (idleDuration > config.idleTimeout) {
                    // Enter power save after idle timeout
                    if (!powerSaveEnabled) {
                        configurePowerSave();
                        powerSaveEnabled = true;
                        Serial.println("[WiFiPowerManager] 절전 모드 진입");
                    }
                }
                break;
                
            case WiFiActivityLevel::LOW:
                // Keep modem sleep enabled
                if (!powerSaveEnabled) {
                    enableModemSleep(true);
                }
                break;
                
            case WiFiActivityLevel::MEDIUM:
            case WiFiActivityLevel::HIGH:
                // Disable power save for better performance
                if (powerSaveEnabled) {
                    enableModemSleep(false);
                    powerSaveEnabled = false;
                    Serial.println("[WiFiPowerManager] 성능 모드 진입");
                }
                break;
        }
    }
    
    // Adjust TX power based on RSSI (every 30 seconds)
    static uint32_t lastTxPowerAdjust = 0;
    if (now - lastTxPowerAdjust > 30000) {
        adjustTxPowerByRSSI();
        lastTxPowerAdjust = now;
    }
}

void WiFiPowerManager::applyPowerMode(WiFiPowerMode mode) {
    Serial.printf("[WiFiPowerManager] 전력 모드 변경: %d -> %d\n", 
                  (int)currentMode, (int)mode);
    
    switch (mode) {
        case WiFiPowerMode::ALWAYS_ON:
            esp_wifi_set_ps(WIFI_PS_NONE);
            enableModemSleep(false);
            enableLightSleep(false);
            setTxPower(config.maxTxPower);
            Serial.println("[WiFiPowerManager] ALWAYS_ON 모드");
            break;
            
        case WiFiPowerMode::BALANCED:
            esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
            enableModemSleep(true);
            enableLightSleep(false);
            setTxPower((config.minTxPower + config.maxTxPower) / 2);
            Serial.println("[WiFiPowerManager] BALANCED 모드");
            break;
            
        case WiFiPowerMode::POWER_SAVE:
            esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
            enableModemSleep(true);
            enableLightSleep(true);
            setTxPower(config.minTxPower);
            Serial.println("[WiFiPowerManager] POWER_SAVE 모드");
            break;
            
        case WiFiPowerMode::DEEP_SLEEP_READY:
            // Prepare for deep sleep
            esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
            WiFi.disconnect(true);
            Serial.println("[WiFiPowerManager] DEEP_SLEEP_READY 모드");
            break;
    }
    
    currentMode = mode;
    lastModeChange = millis();
}

void WiFiPowerManager::updateActivityLevel() {
    uint32_t now = millis();
    static uint32_t lastUpdate = 0;
    
    if (now - lastUpdate < 1000) return; // Update every second
    
    // Calculate packet rate
    uint32_t txRate = txPackets - lastTxPackets;
    uint32_t rxRate = rxPackets - lastRxPackets;
    uint32_t totalRate = txRate + rxRate;
    
    lastTxPackets = txPackets;
    lastRxPackets = rxPackets;
    
    // Determine activity level
    WiFiActivityLevel newLevel;
    if (totalRate == 0) {
        newLevel = WiFiActivityLevel::IDLE;
    } else if (totalRate < 5) {
        newLevel = WiFiActivityLevel::LOW;
    } else if (totalRate < 20) {
        newLevel = WiFiActivityLevel::MEDIUM;
    } else {
        newLevel = WiFiActivityLevel::HIGH;
    }
    
    if (newLevel != activityLevel) {
        activityLevel = newLevel;
        Serial.printf("[WiFiPowerManager] 활동 레벨: %d (패킷/초: %lu)\n", 
                      (int)activityLevel, totalRate);
    }
    
    lastUpdate = now;
}

void WiFiPowerManager::configurePowerSave() {
    // Enable modem sleep
    enableModemSleep(true);
    
    // Reduce TX power if not already minimum
    if (currentTxPower > config.minTxPower + 2) {
        setTxPower(currentTxPower - 2);
    }
}

void WiFiPowerManager::enableModemSleep(bool enable) {
    if (enable) {
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        modemSleepCount++;
        Serial.println("[WiFiPowerManager] Modem Sleep 활성화");
    } else {
        esp_wifi_set_ps(WIFI_PS_NONE);
        Serial.println("[WiFiPowerManager] Modem Sleep 비활성화");
    }
}

void WiFiPowerManager::enableLightSleep(bool enable) {
    if (enable) {
        // Configure PM for light sleep
        esp_pm_config_esp32s3_t pm_config = {
            .max_freq_mhz = 240,
            .min_freq_mhz = 80,
            .light_sleep_enable = true
        };
        esp_pm_configure(&pm_config);
        lightSleepCount++;
        Serial.println("[WiFiPowerManager] Light Sleep 활성화");
    } else {
        esp_pm_config_esp32s3_t pm_config = {
            .max_freq_mhz = 240,
            .min_freq_mhz = 240,
            .light_sleep_enable = false
        };
        esp_pm_configure(&pm_config);
        Serial.println("[WiFiPowerManager] Light Sleep 비활성화");
    }
}

void WiFiPowerManager::enterLightSleep(uint32_t durationMs) {
    Serial.printf("[WiFiPowerManager] Light Sleep 진입: %lu ms\n", durationMs);
    
    uint32_t sleepStart = millis();
    esp_sleep_enable_timer_wakeup(durationMs * 1000ULL); // Convert to microseconds
    esp_light_sleep_start();
    
    uint32_t actualSleep = millis() - sleepStart;
    totalSleepTime += actualSleep;
    lightSleepCount++;
    
    Serial.printf("[WiFiPowerManager] Light Sleep 복귀: %lu ms\n", actualSleep);
}

void WiFiPowerManager::setTxPower(int8_t dbm) {
    // Clamp to configured range
    dbm = constrain(dbm, config.minTxPower, config.maxTxPower);
    
    esp_err_t err = esp_wifi_set_max_tx_power(dbm * 4); // WiFi API uses 0.25dBm units
    if (err == ESP_OK) {
        currentTxPower = dbm;
        Serial.printf("[WiFiPowerManager] TX Power 설정: %d dBm\n", dbm);
    } else {
        Serial.printf("[WiFiPowerManager] TX Power 설정 실패: %d\n", err);
    }
}

void WiFiPowerManager::adjustTxPowerByRSSI() {
    if (!isConnected) return;
    
    int32_t rssi = WiFi.RSSI();
    int8_t newTxPower = currentTxPower;
    
    // Adjust TX power based on RSSI
    if (rssi > -50) {
        // Excellent signal - reduce power
        newTxPower = config.minTxPower;
    } else if (rssi > -60) {
        // Good signal - low power
        newTxPower = config.minTxPower + 2;
    } else if (rssi > -70) {
        // Fair signal - medium power
        newTxPower = (config.minTxPower + config.maxTxPower) / 2;
    } else {
        // Poor signal - max power
        newTxPower = config.maxTxPower;
    }
    
    if (newTxPower != currentTxPower) {
        Serial.printf("[WiFiPowerManager] RSSI: %ld dBm, TX Power 조정: %d -> %d dBm\n", 
                      rssi, currentTxPower, newTxPower);
        setTxPower(newTxPower);
    }
}

void WiFiPowerManager::setPowerMode(WiFiPowerMode mode) {
    if (mode != currentMode) {
        applyPowerMode(mode);
    }
}

void WiFiPowerManager::notifyActivity() {
    lastActivityTime = millis();
    if (activityLevel == WiFiActivityLevel::IDLE) {
        idleStartTime = 0;
    }
}

void WiFiPowerManager::notifyPacketTx() {
    txPackets++;
    notifyActivity();
}

void WiFiPowerManager::notifyPacketRx() {
    rxPackets++;
    notifyActivity();
}

void WiFiPowerManager::setConnected(bool connected) {
    if (connected != isConnected) {
        isConnected = connected;
        if (connected) {
            Serial.println("[WiFiPowerManager] WiFi 연결됨");
            lastActivityTime = millis();
        } else {
            Serial.println("[WiFiPowerManager] WiFi 연결 끊김");
            powerSaveEnabled = false;
        }
    }
}

uint32_t WiFiPowerManager::getIdleTime() const {
    if (activityLevel == WiFiActivityLevel::IDLE) {
        return millis() - lastActivityTime;
    }
    return 0;
}

float WiFiPowerManager::getPowerSavingRatio() const {
    uint32_t uptime = millis();
    if (uptime == 0) return 0.0f;
    return (float)totalSleepTime / uptime * 100.0f;
}

void WiFiPowerManager::printStatus() const {
    Serial.println("\n========== WiFi Power Manager 상태 ==========");
    Serial.printf("전력 모드: ");
    switch (currentMode) {
        case WiFiPowerMode::ALWAYS_ON: Serial.println("ALWAYS_ON"); break;
        case WiFiPowerMode::BALANCED: Serial.println("BALANCED"); break;
        case WiFiPowerMode::POWER_SAVE: Serial.println("POWER_SAVE"); break;
        case WiFiPowerMode::DEEP_SLEEP_READY: Serial.println("DEEP_SLEEP_READY"); break;
    }
    
    Serial.printf("활동 레벨: ");
    switch (activityLevel) {
        case WiFiActivityLevel::IDLE: Serial.println("IDLE"); break;
        case WiFiActivityLevel::LOW: Serial.println("LOW"); break;
        case WiFiActivityLevel::MEDIUM: Serial.println("MEDIUM"); break;
        case WiFiActivityLevel::HIGH: Serial.println("HIGH"); break;
    }
    
    Serial.printf("연결 상태: %s\n", isConnected ? "연결됨" : "끊김");
    Serial.printf("TX Power: %d dBm\n", currentTxPower);
    Serial.printf("RSSI: %ld dBm\n", WiFi.RSSI());
    Serial.printf("절전 활성화: %s\n", powerSaveEnabled ? "예" : "아니오");
    Serial.printf("유휴 시간: %lu ms\n", getIdleTime());
    
    Serial.println("\n통계:");
    Serial.printf("  TX 패킷: %lu\n", txPackets);
    Serial.printf("  RX 패킷: %lu\n", rxPackets);
    Serial.printf("  Modem Sleep 횟수: %lu\n", modemSleepCount);
    Serial.printf("  Light Sleep 횟수: %lu\n", lightSleepCount);
    Serial.printf("  총 Sleep 시간: %lu ms\n", totalSleepTime);
    Serial.printf("  절전 비율: %.2f%%\n", getPowerSavingRatio());
    Serial.println("============================================\n");
}

void WiFiPowerManager::resetStatistics() {
    txPackets = 0;
    rxPackets = 0;
    lastTxPackets = 0;
    lastRxPackets = 0;
    modemSleepCount = 0;
    lightSleepCount = 0;
    totalSleepTime = 0;
    Serial.println("[WiFiPowerManager] 통계 초기화");
}