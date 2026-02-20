// WiFiPowerManager.h
#ifndef WIFI_POWER_MANAGER_H
#define WIFI_POWER_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_pm.h>

// WiFi Power Modes
enum class WiFiPowerMode {
    ALWAYS_ON,          // 항상 최대 성능 (개발/디버깅)
    BALANCED,           // 균형 모드 (기본)
    POWER_SAVE,         // 전력 절약 모드
    DEEP_SLEEP_READY    // Deep Sleep 준비 모드
};

// WiFi Activity Level
enum class WiFiActivityLevel {
    IDLE,               // 유휴 상태
    LOW,                // 낮은 활동
    MEDIUM,             // 중간 활동
    HIGH                // 높은 활동
};

// WiFi Power Configuration
struct WiFiPowerConfig {
    WiFiPowerMode mode;
    uint32_t idleTimeout;           // 유휴 후 절전 모드 진입 시간 (ms)
    uint32_t sleepInterval;         // Light Sleep 간격 (ms)
    uint32_t wakeInterval;          // Wake 간격 (ms)
    bool enableModemSleep;          // Modem Sleep 활성화
    bool enableLightSleep;          // Light Sleep 활성화
    uint8_t minTxPower;             // 최소 송신 전력 (dBm)
    uint8_t maxTxPower;             // 최대 송신 전력 (dBm)
    
    WiFiPowerConfig() 
        : mode(WiFiPowerMode::BALANCED),
          idleTimeout(30000),       // 30초
          sleepInterval(100),       // 100ms
          wakeInterval(3),          // 3ms
          enableModemSleep(true),
          enableLightSleep(true),
          minTxPower(8),            // 8 dBm (2mW)
          maxTxPower(20)            // 20 dBm (100mW)
    {}
};

class WiFiPowerManager {
private:
    WiFiPowerConfig config;
    WiFiPowerMode currentMode;
    WiFiActivityLevel activityLevel;
    
    uint32_t lastActivityTime;
    uint32_t lastModeChange;
    uint32_t idleStartTime;
    
    uint32_t txPackets;
    uint32_t rxPackets;
    uint32_t lastTxPackets;
    uint32_t lastRxPackets;
    
    bool isConnected;
    bool powerSaveEnabled;
    int8_t currentTxPower;
    
    // Statistics
    uint32_t modemSleepCount;
    uint32_t lightSleepCount;
    uint32_t totalSleepTime;
    
    // Internal methods
    void applyPowerMode(WiFiPowerMode mode);
    void updateActivityLevel();
    void configurePowerSave();
    void configureTxPower(int8_t dbm);
    
public:
    WiFiPowerManager();
    
    // Initialization
    void begin(const WiFiPowerConfig& cfg = WiFiPowerConfig());
    
    // Main update loop
    void update();
    
    // Power mode control
    void setPowerMode(WiFiPowerMode mode);
    WiFiPowerMode getPowerMode() const { return currentMode; }
    
    // Activity tracking
    void notifyActivity();
    void notifyPacketTx();
    void notifyPacketRx();
    WiFiActivityLevel getActivityLevel() const { return activityLevel; }
    
    // Connection state
    void setConnected(bool connected);
    bool getConnected() const { return isConnected; }
    
    // Power saving features
    void enableModemSleep(bool enable);
    void enableLightSleep(bool enable);
    void enterLightSleep(uint32_t durationMs);
    
    // Tx Power control
    void setTxPower(int8_t dbm);
    int8_t getTxPower() const { return currentTxPower; }
    void adjustTxPowerByRSSI();
    
    // Statistics
    uint32_t getModemSleepCount() const { return modemSleepCount; }
    uint32_t getLightSleepCount() const { return lightSleepCount; }
    uint32_t getTotalSleepTime() const { return totalSleepTime; }
    uint32_t getIdleTime() const;
    float getPowerSavingRatio() const;
    
    // Diagnostic
    void printStatus() const;
    void resetStatistics();
};

extern WiFiPowerManager wifiPowerManager;

#endif // WIFI_POWER_MANAGER_H