// ================================================================
// WiFiResilience.h - WiFi    (Phase 3-1)
// ================================================================
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <vector>

// ================================================================
// WiFi 
// ================================================================
#define WIFI_MAX_RETRY_ATTEMPTS        5       //   
#define WIFI_RETRY_DELAY_MS            2000    //   (ms)
#define WIFI_CONNECTION_TIMEOUT_MS     10000   //   (ms)
#define WIFI_MONITOR_INTERVAL_MS       5000    //   (ms)
#define WIFI_MAX_STORED_APS            3       //  AP 
#define WIFI_RSSI_THRESHOLD            -80     //    (dBm)

// ================================================================
// WiFi 
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
//  
// ================================================================
enum ReconnectStrategy {
    STRATEGY_FAST,          //   ( AP)
    STRATEGY_ALTERNATE,     //  AP 
    STRATEGY_SCAN,          //     AP
    STRATEGY_RESET          // WiFi   
};

// ================================================================
// AP 
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
// WiFi 
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
// WiFi Resilience 
// ================================================================
class WiFiResilience {
public:
    //   
    void begin();
    
    //  AP  
    bool addAP(const char* ssid, const char* password);
    bool removeAP(const char* ssid);
    void clearAPs();
    uint8_t getAPCount();
    
    //    
    bool connect(uint32_t timeout = WIFI_CONNECTION_TIMEOUT_MS);
    bool reconnect(ReconnectStrategy strategy = STRATEGY_FAST);
    void disconnect();
    
    //   
    void update();  // loop()  
    bool isConnected();
    WiFiState getState();
    int8_t getRSSI();
    
    //    
    void enableAutoReconnect(bool enable = true);
    void setReconnectInterval(uint32_t intervalMs);
    
    //    
    bool isSignalWeak();
    bool shouldSwitchAP();
    bool scanAndConnectBest();
    
    //   
    WiFiStats getStats();
    void resetStats();
    void printStats();
    void printAPList();
    
    //   
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
    
    //    
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
//  
// ================================================================
extern WiFiResilience wifiResilience;

// ================================================================
//  
// ================================================================
#define WIFI_CHECK() wifiResilience.update()
#define WIFI_CONNECTED() wifiResilience.isConnected()
