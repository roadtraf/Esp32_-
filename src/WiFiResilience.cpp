// ================================================================
// WiFiResilience.cpp - WiFi    
// ================================================================
#include "WiFiResilience.h"
#include <Preferences.h>

// FreeRTOS (delay )
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

//  
WiFiResilience wifiResilience;

// Preferences 
static Preferences wifiPrefs;

// ================================================================
// 
// ================================================================
void WiFiResilience::begin() {
    Serial.println("[WiFiResilience]  ...");
    
    state = WIFI_STATE_DISCONNECTED;
    currentAPIndex = -1;
    autoReconnectEnabled = true;
    reconnectInterval = WIFI_MONITOR_INTERVAL_MS;
    retryCount = 0;
    
    lastMonitorTime = 0;
    connectionStartTime = 0;
    disconnectionTime = 0;
    lastReconnectAttempt = 0;
    
    memset(&stats, 0, sizeof(stats));
    
    // WiFi  
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);  //  
    
    // AP  
    loadAPList();
    
    Serial.printf("[WiFiResilience]   ( AP: %d)\n", apList.size());
}

// ================================================================
// AP 
// ================================================================
bool WiFiResilience::addAP(const char* ssid, const char* password) {
    if (apList.size() >= WIFI_MAX_STORED_APS) {
        Serial.println("[WiFiResilience]   AP   ");
        return false;
    }
    
    //  
    if (findAPBySSID(ssid) >= 0) {
        Serial.printf("[WiFiResilience]   AP  : %s\n", ssid);
        return false;
    }
    
    APInfo newAP;
    strncpy(newAP.ssid, ssid, sizeof(newAP.ssid) - 1);
    newAP.ssid[sizeof(newAP.ssid) - 1] = '\0';
    
    strncpy(newAP.password, password, sizeof(newAP.password) - 1);
    newAP.password[sizeof(newAP.password) - 1] = '\0';
    
    newAP.rssi = 0;
    newAP.lastConnected = 0;
    newAP.connectionCount = 0;
    newAP.enabled = true;
    
    apList.push_back(newAP);
    saveAPList();
    
    Serial.printf("[WiFiResilience]  AP : %s\n", ssid);
    return true;
}

bool WiFiResilience::removeAP(const char* ssid) {
    int8_t index = findAPBySSID(ssid);
    if (index < 0) {
        Serial.printf("[WiFiResilience]   AP : %s\n", ssid);
        return false;
    }
    
    apList.erase(apList.begin() + index);
    saveAPList();
    
    Serial.printf("[WiFiResilience] AP : %s\n", ssid);
    return true;
}

void WiFiResilience::clearAPs() {
    apList.clear();
    saveAPList();
    Serial.println("[WiFiResilience]  AP ");
}

uint8_t WiFiResilience::getAPCount() {
    return apList.size();
}

// ================================================================
//  
// ================================================================
bool WiFiResilience::connect(uint32_t timeout) {
    if (apList.empty()) {
        Serial.println("[WiFiResilience]   AP ");
        return false;
    }
    
    Serial.println("[WiFiResilience]  ...");
    state = WIFI_STATE_CONNECTING;
    connectionStartTime = millis();
    
    //   AP  
    int8_t bestIndex = findBestAP();
    
    if (connectToAP(bestIndex, timeout)) {
        return true;
    }
    
    //    AP 
    for (uint8_t i = 0; i < apList.size(); i++) {
        if (i == bestIndex) continue;  //  
        
        if (connectToAP(i, timeout)) {
            return true;
        }
    }
    
    state = WIFI_STATE_FAILED;
    stats.failedAttempts++;
    
    Serial.println("[WiFiResilience]   AP  ");
    return false;
}

bool WiFiResilience::reconnect(ReconnectStrategy strategy) {
    Serial.printf("[WiFiResilience]   (: %d)...\n", strategy);
    state = WIFI_STATE_RECONNECTING;
    
    switch (strategy) {
        case STRATEGY_FAST:
            //  AP  
            if (currentAPIndex >= 0) {
                return connectToAP(currentAPIndex, WIFI_CONNECTION_TIMEOUT_MS);
            }
            return connect();
            
        case STRATEGY_ALTERNATE:
            //  AP 
            for (uint8_t i = 0; i < apList.size(); i++) {
                if (i == currentAPIndex) continue;
                if (connectToAP(i, WIFI_CONNECTION_TIMEOUT_MS)) {
                    return true;
                }
            }
            return false;
            
        case STRATEGY_SCAN:
            //    AP 
            return scanAndConnectBest();
            
        case STRATEGY_RESET:
            // WiFi   
            WiFi.disconnect(true);
            vTaskDelay(pdMS_TO_TICKS(1000));
            WiFi.mode(WIFI_STA);
            return connect();
            
        default:
            return connect();
    }
}

void WiFiResilience::disconnect() {
    WiFi.disconnect();
    state = WIFI_STATE_DISCONNECTED;
    disconnectionTime = millis();
    
    Serial.println("[WiFiResilience]  ");
}

// ================================================================
// 
// ================================================================
void WiFiResilience::update() {
    uint32_t now = millis();
    
    //   
    if (now - lastMonitorTime < reconnectInterval) {
        return;
    }
    lastMonitorTime = now;
    
    //   
    if (WiFi.status() == WL_CONNECTED) {
        if (state != WIFI_STATE_CONNECTED) {
            state = WIFI_STATE_CONNECTED;
            updateConnectionStats();
        }
        
        // RSSI 
        updateRSSI();
        
        //     AP 
        if (shouldSwitchAP()) {
            Serial.println("[WiFiResilience]  , AP  ...");
            scanAndConnectBest();
        }
        
    } else {
        //   
        if (state == WIFI_STATE_CONNECTED) {
            handleDisconnection();
        }
        
        //  
        if (autoReconnectEnabled && 
            (now - lastReconnectAttempt) > WIFI_RETRY_DELAY_MS) {
            
            lastReconnectAttempt = now;
            retryCount++;
            
            if (retryCount <= WIFI_MAX_RETRY_ATTEMPTS) {
                Serial.printf("[WiFiResilience]    (%d/%d)...\n", 
                              retryCount, WIFI_MAX_RETRY_ATTEMPTS);
                
                ReconnectStrategy strategy = STRATEGY_FAST;
                if (retryCount > 2) strategy = STRATEGY_ALTERNATE;
                if (retryCount > 4) strategy = STRATEGY_SCAN;
                
                if (reconnect(strategy)) {
                    retryCount = 0;
                }
            } else {
                Serial.println("[WiFiResilience]     ");
                retryCount = 0;
                lastReconnectAttempt = now + 30000;  // 30  
            }
        }
    }
}

bool WiFiResilience::isConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

WiFiState WiFiResilience::getState() {
    return state;
}

int8_t WiFiResilience::getRSSI() {
    if (!isConnected()) return -100;
    return WiFi.RSSI();
}

// ================================================================
//  
// ================================================================
void WiFiResilience::enableAutoReconnect(bool enable) {
    autoReconnectEnabled = enable;
    Serial.printf("[WiFiResilience]  : %s\n", enable ? "" : "");
}

void WiFiResilience::setReconnectInterval(uint32_t intervalMs) {
    reconnectInterval = intervalMs;
    Serial.printf("[WiFiResilience]  : %lums\n", intervalMs);
}

// ================================================================
//  
// ================================================================
bool WiFiResilience::isSignalWeak() {
    if (!isConnected()) return true;
    return (getRSSI() < WIFI_RSSI_THRESHOLD);
}

bool WiFiResilience::shouldSwitchAP() {
    if (!isConnected() || apList.size() <= 1) return false;
    
    int8_t currentRSSI = getRSSI();
    
    //     AP   
    return (currentRSSI < WIFI_RSSI_THRESHOLD);
}

bool WiFiResilience::scanAndConnectBest() {
    Serial.println("[WiFiResilience] WiFi  ...");
    state = WIFI_STATE_SCANNING;
    
    int n = WiFi.scanNetworks();
    Serial.printf("[WiFiResilience] %d  \n", n);
    
    if (n == 0) {
        Serial.println("[WiFiResilience]   ");
        return false;
    }
    
    //  AP     
    int8_t bestAPIndex = -1;
    int8_t bestRSSI = -100;
    
    for (uint8_t i = 0; i < apList.size(); i++) {
        for (int j = 0; j < n; j++) {
            if (strcmp(WiFi.SSID(j).c_str(), apList[i].ssid) == 0) {
                int8_t rssi = WiFi.RSSI(j);
                if (rssi > bestRSSI) {
                    bestRSSI = rssi;
                    bestAPIndex = i;
                }
            }
        }
    }
    
    WiFi.scanDelete();
    
    if (bestAPIndex >= 0) {
        Serial.printf("[WiFiResilience]  AP: %s (RSSI: %d dBm)\n", 
                      apList[bestAPIndex].ssid, bestRSSI);
        return connectToAP(bestAPIndex, WIFI_CONNECTION_TIMEOUT_MS);
    }
    
    Serial.println("[WiFiResilience]  AP   ");
    return false;
}

// ================================================================
// 
// ================================================================
WiFiStats WiFiResilience::getStats() {
    if (isConnected()) {
        stats.currentUptime = (millis() - connectionStartTime) / 1000;
        if (stats.currentUptime > stats.longestUptime) {
            stats.longestUptime = stats.currentUptime;
        }
    }
    
    return stats;
}

void WiFiResilience::resetStats() {
    memset(&stats, 0, sizeof(stats));
    Serial.println("[WiFiResilience]  ");
}

void WiFiResilience::printStats() {
    WiFiStats s = getStats();
    
    Serial.println("\n");
    Serial.println("       WiFi                        ");
    Serial.println("");
    Serial.printf("  : %lu                        \n", s.totalConnections);
    Serial.printf("  : %lu                      \n", s.totalDisconnections);
    Serial.printf(" : %lu                         \n", s.totalReconnections);
    Serial.printf(" : %lu                           \n", s.failedAttempts);
    Serial.println("");
    Serial.printf("  : %lu                      \n", s.currentUptime);
    Serial.printf("  : %lu                      \n", s.longestUptime);
    
    if (isConnected()) {
        Serial.printf("  RSSI: %d dBm                    \n", getRSSI());
    }
    
    Serial.println("\n");
}

void WiFiResilience::printAPList() {
    Serial.println("\n");
    Serial.println("        AP                   ");
    Serial.println("");
    
    if (apList.empty()) {
        Serial.println(" ()                                ");
    } else {
        for (uint8_t i = 0; i < apList.size(); i++) {
            Serial.printf(" %d. %-33s \n", i + 1, apList[i].ssid);
            Serial.printf("    : %lu                        \n", apList[i].connectionCount);
        }
    }
    
    Serial.println("\n");
}

// ================================================================
// 
// ================================================================
void WiFiResilience::printDiagnostics() {
    Serial.println("\n");
    Serial.println("       WiFi                    ");
    Serial.println("");
    
    const char* stateStr;
    switch (state) {
        case WIFI_STATE_DISCONNECTED: stateStr = " "; break;
        case WIFI_STATE_CONNECTING:   stateStr = " "; break;
        case WIFI_STATE_CONNECTED:    stateStr = ""; break;
        case WIFI_STATE_RECONNECTING: stateStr = " "; break;
        case WIFI_STATE_FAILED:       stateStr = ""; break;
        case WIFI_STATE_SCANNING:     stateStr = " "; break;
        default:                      stateStr = "  "; break;
    }
    
    Serial.printf(" : %-31s \n", stateStr);
    
    if (isConnected()) {
        Serial.printf(" SSID: %-31s \n", WiFi.SSID().c_str());
        char ipBufW[16]; WiFi.localIP().toString().toCharArray(ipBufW, sizeof(ipBufW));
        Serial.printf(" IP: %-33s \n", ipBufW);
        Serial.printf(" RSSI: %d dBm                          \n", getRSSI());
        Serial.printf(" : %d                              \n", WiFi.channel());
    }
    
    Serial.printf("  : %-24s \n", 
                  autoReconnectEnabled ? "" : "");
    Serial.printf("  AP: %d                       \n", apList.size());
    
    Serial.println("\n");
}

bool WiFiResilience::testConnectivity() {
    if (!isConnected()) {
        Serial.println("[WiFiResilience] WiFi  ");
        return false;
    }
    
    Serial.println("[WiFiResilience]   ...");
    
    // DNS 
    IPAddress testIP;
    if (!WiFi.hostByName("www.google.com", testIP)) {
        Serial.println("[WiFiResilience]  DNS ");
        return false;
    }
    
    char dnsIpBuf[16]; testIP.toString().toCharArray(dnsIpBuf, sizeof(dnsIpBuf));
    Serial.printf("[WiFiResilience]  DNS : %s\n", dnsIpBuf);
    return true;
}

// ================================================================
//  
// ================================================================
bool WiFiResilience::connectToAP(int8_t index, uint32_t timeout) {
    if (index < 0 || index >= apList.size()) {
        return false;
    }
    
    APInfo& ap = apList[index];
    Serial.printf("[WiFiResilience]  : %s\n", ap.ssid);
    
    WiFi.begin(ap.ssid, ap.password);
    
    if (waitForConnection(timeout)) {
        currentAPIndex = index;
        ap.connectionCount++;
        ap.lastConnected = millis() / 1000;
        ap.rssi = WiFi.RSSI();
        
        saveAPList();
        
        Serial.printf("[WiFiResilience]   : %s (RSSI: %d dBm)\n", 
                      ap.ssid, ap.rssi);
        return true;
    }
    
    Serial.printf("[WiFiResilience]   : %s\n", ap.ssid);
    return false;
}

int8_t WiFiResilience::findBestAP() {
    if (apList.empty()) return -1;
    
    int8_t bestIndex = 0;
    uint32_t bestScore = 0;
    
    for (uint8_t i = 0; i < apList.size(); i++) {
        if (!apList[i].enabled) continue;
        
        //  :   +  
        uint32_t score = apList[i].lastConnected + (apList[i].connectionCount * 100);
        
        if (score > bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }
    
    return bestIndex;
}

int8_t WiFiResilience::findAPBySSID(const char* ssid) {
    for (uint8_t i = 0; i < apList.size(); i++) {
        if (strcmp(apList[i].ssid, ssid) == 0) {
            return i;
        }
    }
    return -1;
}

void WiFiResilience::updateConnectionStats() {
    stats.totalConnections++;
    connectionStartTime = millis();
    
    Serial.println("[WiFiResilience]  WiFi ");
}

void WiFiResilience::updateRSSI() {
    if (currentAPIndex >= 0 && currentAPIndex < apList.size()) {
        apList[currentAPIndex].rssi = WiFi.RSSI();
    }
}

void WiFiResilience::handleDisconnection() {
    stats.totalDisconnections++;
    stats.lastDisconnectTime = millis() / 1000;
    
    if (connectionStartTime > 0) {
        uint32_t uptime = (millis() - connectionStartTime) / 1000;
        if (uptime > stats.longestUptime) {
            stats.longestUptime = uptime;
        }
    }
    
    state = WIFI_STATE_DISCONNECTED;
    disconnectionTime = millis();
    
    Serial.println("[WiFiResilience]   WiFi  ");
}

bool WiFiResilience::waitForConnection(uint32_t timeout) {
    uint32_t start = millis();
    
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > timeout) {
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    return true;
}

void WiFiResilience::saveAPList() {
    wifiPrefs.begin("wifires", false);
    
    wifiPrefs.putUInt("apCount", apList.size());
    
    for (uint8_t i = 0; i < apList.size(); i++) {
        char key[16];
        
        sprintf(key, "ssid%d", i);
        wifiPrefs.putString(key, apList[i].ssid);
        
        sprintf(key, "pass%d", i);
        wifiPrefs.putString(key, apList[i].password);
        
        sprintf(key, "cnt%d", i);
        wifiPrefs.putUInt(key, apList[i].connectionCount);
        
        sprintf(key, "last%d", i);
        wifiPrefs.putUInt(key, apList[i].lastConnected);
    }
    
    wifiPrefs.end();
}

void WiFiResilience::loadAPList() {
    wifiPrefs.begin("wifires", true);
    
    uint8_t count = wifiPrefs.getUInt("apCount", 0);
    
    for (uint8_t i = 0; i < count && i < WIFI_MAX_STORED_APS; i++) {
        APInfo ap;
        char key[16];
        
        sprintf(key, "ssid%d", i);
        char ssidBuf[33] = {0};
        wifiPrefs.getString(key, ssidBuf, sizeof(ssidBuf));
        if (strlen(ssidBuf) == 0) continue;
        
        strncpy(ap.ssid, ssidBuf, sizeof(ap.ssid) - 1);
        
        sprintf(key, "pass%d", i);
        char passBuf[65] = {0};
        wifiPrefs.getString(key, passBuf, sizeof(passBuf));
        strncpy(ap.password, passBuf, sizeof(ap.password) - 1);
        
        sprintf(key, "cnt%d", i);
        ap.connectionCount = wifiPrefs.getUInt(key, 0);
        
        sprintf(key, "last%d", i);
        ap.lastConnected = wifiPrefs.getUInt(key, 0);
        
        ap.rssi = 0;
        ap.enabled = true;
        
        apList.push_back(ap);
    }
    
    wifiPrefs.end();
}
