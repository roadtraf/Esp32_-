// ================================================================
// WiFiResilience.h - WiFi 재연결 강화 시스템 (Phase 3-1)
// ================================================================
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <vector>

// ================================================================
// WiFi 설정
// ================================================================
#define WIFI_MAX_RETRY_ATTEMPTS        5       // 최대 재시도 횟수
#define WIFI_RETRY_DELAY_MS            2000    // 재시도 간격 (ms)
#define WIFI_CONNECTION_TIMEOUT_MS     10000   // 연결 타임아웃 (ms)
#define WIFI_MONITOR_INTERVAL_MS       5000    // 모니터링 간격 (ms)
#define WIFI_MAX_STORED_APS            3       // 저장할 AP 개수
#define WIFI_RSSI_THRESHOLD            -80     // 최소 신호 강도 (dBm)

// ================================================================
// WiFi 상태
// ================================================================
enum WiFiState {
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_RECONNECTING,
    WIFI_STATE_FAILED,
    WIFI_STATE_SCANNING
};

// ================================================================
// 재연결 전략
// ================================================================
enum ReconnectStrategy {
    STRATEGY_FAST,          // 빠른 재연결 (같은 AP)
    STRATEGY_ALTERNATE,     // 대체 AP 시도
    STRATEGY_SCAN,          // 전체 스캔 후 최적 AP
    STRATEGY_RESET          // WiFi 리셋 후 재연결
};

// ================================================================
// AP 정보
// ================================================================
struct APInfo {
    char ssid[33];
    char password[64];
    int8_t rssi;
    uint32_t lastConnected;
    uint32_t connectionCount;
    bool enabled;
};

// ================================================================
// WiFi 통계
// ================================================================
struct WiFiStats {
    uint32_t totalConnections;
    uint32_t totalDisconnections;
    uint32_t totalReconnections;
    uint32_t failedAttempts;
    uint32_t longestUptime;
    uint32_t currentUptime;
    uint32_t lastDisconnectTime;
    int8_t averageRSSI;
};

// ================================================================
// WiFi Resilience 클래스
// ================================================================
class WiFiResilience {
public:
    // ── 초기화 ──
    void begin();
    
    // ── AP 관리 ──
    bool addAP(const char* ssid, const char* password);
    bool removeAP(const char* ssid);
    void clearAPs();
    uint8_t getAPCount();
    
    // ── 연결 관리 ──
    bool connect(uint32_t timeout = WIFI_CONNECTION_TIMEOUT_MS);
    bool reconnect(ReconnectStrategy strategy = STRATEGY_FAST);
    void disconnect();
    
    // ── 모니터링 ──
    void update();  // loop()에서 주기적 호출
    bool isConnected();
    WiFiState getState();
    int8_t getRSSI();
    
    // ── 자동 재연결 ──
    void enableAutoReconnect(bool enable = true);
    void setReconnectInterval(uint32_t intervalMs);
    
    // ── 신호 품질 ──
    bool isSignalWeak();
    bool shouldSwitchAP();
    bool scanAndConnectBest();
    
    // ── 통계 ──
    WiFiStats getStats();
    void resetStats();
    void printStats();
    void printAPList();
    
    // ── 진단 ──
    void printDiagnostics();
    bool testConnectivity();
    
private:
    std::vector<APInfo> apList;
    WiFiState state;
    WiFiStats stats;
    
    uint32_t lastMonitorTime;
    uint32_t connectionStartTime;
    uint32_t disconnectionTime;
    uint32_t lastReconnectAttempt;
    
    int8_t currentAPIndex;
    bool autoReconnectEnabled;
    uint32_t reconnectInterval;
    uint32_t retryCount;
    
    // ── 내부 메서드 ──
    bool connectToAP(int8_t index, uint32_t timeout);
    int8_t findBestAP();
    int8_t findAPBySSID(const char* ssid);
    void updateConnectionStats();
    void updateRSSI();
    void handleDisconnection();
    bool waitForConnection(uint32_t timeout);
    void saveAPList();
    void loadAPList();
};

// ================================================================
// 전역 인스턴스
// ================================================================
extern WiFiResilience wifiResilience;

// ================================================================
// 편의 매크로
// ================================================================
#define WIFI_CHECK() wifiResilience.update()
#define WIFI_CONNECTED() wifiResilience.isConnected()
