// ================================================================
// WiFiResilience.cpp - WiFi 재연결 강화 시스템 구현
// ================================================================
#include "WiFiResilience.h"
#include <Preferences.h>

// FreeRTOS (delay 개선)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 전역 인스턴스
WiFiResilience wifiResilience;

// Preferences 객체
static Preferences wifiPrefs;

// ================================================================
// 초기화
// ================================================================
void WiFiResilience::begin() {
    Serial.println("[WiFiResilience] 초기화 시작...");
    
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
    
    // WiFi 모드 설정
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);  // 수동 관리
    
    // AP 목록 로드
    loadAPList();
    
    Serial.printf("[WiFiResilience] 초기화 완료 (저장된 AP: %d개)\n", apList.size());
}

// ================================================================
// AP 관리
// ================================================================
bool WiFiResilience::addAP(const char* ssid, const char* password) {
    if (apList.size() >= WIFI_MAX_STORED_APS) {
        Serial.println("[WiFiResilience] ⚠️  AP 저장 공간 부족");
        return false;
    }
    
    // 중복 확인
    if (findAPBySSID(ssid) >= 0) {
        Serial.printf("[WiFiResilience] ⚠️  AP 이미 존재: %s\n", ssid);
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
    
    Serial.printf("[WiFiResilience] ✅ AP 추가: %s\n", ssid);
    return true;
}

bool WiFiResilience::removeAP(const char* ssid) {
    int8_t index = findAPBySSID(ssid);
    if (index < 0) {
        Serial.printf("[WiFiResilience] ⚠️  AP 없음: %s\n", ssid);
        return false;
    }
    
    apList.erase(apList.begin() + index);
    saveAPList();
    
    Serial.printf("[WiFiResilience] AP 제거: %s\n", ssid);
    return true;
}

void WiFiResilience::clearAPs() {
    apList.clear();
    saveAPList();
    Serial.println("[WiFiResilience] 모든 AP 제거");
}

uint8_t WiFiResilience::getAPCount() {
    return apList.size();
}

// ================================================================
// 연결 관리
// ================================================================
bool WiFiResilience::connect(uint32_t timeout) {
    if (apList.empty()) {
        Serial.println("[WiFiResilience] ❌ 저장된 AP 없음");
        return false;
    }
    
    Serial.println("[WiFiResilience] 연결 시도...");
    state = WIFI_STATE_CONNECTING;
    connectionStartTime = millis();
    
    // 최근 연결했던 AP 먼저 시도
    int8_t bestIndex = findBestAP();
    
    if (connectToAP(bestIndex, timeout)) {
        return true;
    }
    
    // 실패 시 다른 AP들 시도
    for (uint8_t i = 0; i < apList.size(); i++) {
        if (i == bestIndex) continue;  // 이미 시도함
        
        if (connectToAP(i, timeout)) {
            return true;
        }
    }
    
    state = WIFI_STATE_FAILED;
    stats.failedAttempts++;
    
    Serial.println("[WiFiResilience] ❌ 모든 AP 연결 실패");
    return false;
}

bool WiFiResilience::reconnect(ReconnectStrategy strategy) {
    Serial.printf("[WiFiResilience] 재연결 시도 (전략: %d)...\n", strategy);
    state = WIFI_STATE_RECONNECTING;
    
    switch (strategy) {
        case STRATEGY_FAST:
            // 현재 AP로 빠른 재연결
            if (currentAPIndex >= 0) {
                return connectToAP(currentAPIndex, WIFI_CONNECTION_TIMEOUT_MS);
            }
            return connect();
            
        case STRATEGY_ALTERNATE:
            // 다른 AP 시도
            for (uint8_t i = 0; i < apList.size(); i++) {
                if (i == currentAPIndex) continue;
                if (connectToAP(i, WIFI_CONNECTION_TIMEOUT_MS)) {
                    return true;
                }
            }
            return false;
            
        case STRATEGY_SCAN:
            // 스캔 후 최적 AP 연결
            return scanAndConnectBest();
            
        case STRATEGY_RESET:
            // WiFi 리셋 후 재연결
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
    
    Serial.println("[WiFiResilience] 연결 해제");
}

// ================================================================
// 모니터링
// ================================================================
void WiFiResilience::update() {
    uint32_t now = millis();
    
    // 모니터링 간격 체크
    if (now - lastMonitorTime < reconnectInterval) {
        return;
    }
    lastMonitorTime = now;
    
    // 연결 상태 확인
    if (WiFi.status() == WL_CONNECTED) {
        if (state != WIFI_STATE_CONNECTED) {
            state = WIFI_STATE_CONNECTED;
            updateConnectionStats();
        }
        
        // RSSI 업데이트
        updateRSSI();
        
        // 신호 약하면 더 나은 AP 찾기
        if (shouldSwitchAP()) {
            Serial.println("[WiFiResilience] 신호 약함, AP 전환 시도...");
            scanAndConnectBest();
        }
        
    } else {
        // 연결 끊김 감지
        if (state == WIFI_STATE_CONNECTED) {
            handleDisconnection();
        }
        
        // 자동 재연결
        if (autoReconnectEnabled && 
            (now - lastReconnectAttempt) > WIFI_RETRY_DELAY_MS) {
            
            lastReconnectAttempt = now;
            retryCount++;
            
            if (retryCount <= WIFI_MAX_RETRY_ATTEMPTS) {
                Serial.printf("[WiFiResilience] 자동 재연결 시도 (%d/%d)...\n", 
                              retryCount, WIFI_MAX_RETRY_ATTEMPTS);
                
                ReconnectStrategy strategy = STRATEGY_FAST;
                if (retryCount > 2) strategy = STRATEGY_ALTERNATE;
                if (retryCount > 4) strategy = STRATEGY_SCAN;
                
                if (reconnect(strategy)) {
                    retryCount = 0;
                }
            } else {
                Serial.println("[WiFiResilience] ⚠️  최대 재시도 초과");
                retryCount = 0;
                lastReconnectAttempt = now + 30000;  // 30초 후 재시도
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
// 자동 재연결
// ================================================================
void WiFiResilience::enableAutoReconnect(bool enable) {
    autoReconnectEnabled = enable;
    Serial.printf("[WiFiResilience] 자동 재연결: %s\n", enable ? "활성화" : "비활성화");
}

void WiFiResilience::setReconnectInterval(uint32_t intervalMs) {
    reconnectInterval = intervalMs;
    Serial.printf("[WiFiResilience] 재연결 간격: %lums\n", intervalMs);
}

// ================================================================
// 신호 품질
// ================================================================
bool WiFiResilience::isSignalWeak() {
    if (!isConnected()) return true;
    return (getRSSI() < WIFI_RSSI_THRESHOLD);
}

bool WiFiResilience::shouldSwitchAP() {
    if (!isConnected() || apList.size() <= 1) return false;
    
    int8_t currentRSSI = getRSSI();
    
    // 현재 신호가 약하고 다른 AP가 있으면 전환 고려
    return (currentRSSI < WIFI_RSSI_THRESHOLD);
}

bool WiFiResilience::scanAndConnectBest() {
    Serial.println("[WiFiResilience] WiFi 스캔 시작...");
    state = WIFI_STATE_SCANNING;
    
    int n = WiFi.scanNetworks();
    Serial.printf("[WiFiResilience] %d개 네트워크 발견\n", n);
    
    if (n == 0) {
        Serial.println("[WiFiResilience] 스캔 결과 없음");
        return false;
    }
    
    // 저장된 AP 중 가장 강한 신호 찾기
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
        Serial.printf("[WiFiResilience] 최적 AP: %s (RSSI: %d dBm)\n", 
                      apList[bestAPIndex].ssid, bestRSSI);
        return connectToAP(bestAPIndex, WIFI_CONNECTION_TIMEOUT_MS);
    }
    
    Serial.println("[WiFiResilience] 저장된 AP를 찾을 수 없음");
    return false;
}

// ================================================================
// 통계
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
    Serial.println("[WiFiResilience] 통계 초기화");
}

void WiFiResilience::printStats() {
    WiFiStats s = getStats();
    
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║       WiFi 통계                       ║");
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ 총 연결: %lu회                        ║\n", s.totalConnections);
    Serial.printf("║ 연결 해제: %lu회                      ║\n", s.totalDisconnections);
    Serial.printf("║ 재연결: %lu회                         ║\n", s.totalReconnections);
    Serial.printf("║ 실패: %lu회                           ║\n", s.failedAttempts);
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ 현재 가동: %lu초                      ║\n", s.currentUptime);
    Serial.printf("║ 최장 가동: %lu초                      ║\n", s.longestUptime);
    
    if (isConnected()) {
        Serial.printf("║ 현재 RSSI: %d dBm                    ║\n", getRSSI());
    }
    
    Serial.println("╚═══════════════════════════════════════╝\n");
}

void WiFiResilience::printAPList() {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║       저장된 AP 목록                  ║");
    Serial.println("╠═══════════════════════════════════════╣");
    
    if (apList.empty()) {
        Serial.println("║ (없음)                                ║");
    } else {
        for (uint8_t i = 0; i < apList.size(); i++) {
            Serial.printf("║ %d. %-33s ║\n", i + 1, apList[i].ssid);
            Serial.printf("║    연결: %lu회                        ║\n", apList[i].connectionCount);
        }
    }
    
    Serial.println("╚═══════════════════════════════════════╝\n");
}

// ================================================================
// 진단
// ================================================================
void WiFiResilience::printDiagnostics() {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║       WiFi 진단 정보                  ║");
    Serial.println("╠═══════════════════════════════════════╣");
    
    const char* stateStr;
    switch (state) {
        case WIFI_STATE_DISCONNECTED: stateStr = "연결 끊김"; break;
        case WIFI_STATE_CONNECTING:   stateStr = "연결 중"; break;
        case WIFI_STATE_CONNECTED:    stateStr = "연결됨"; break;
        case WIFI_STATE_RECONNECTING: stateStr = "재연결 중"; break;
        case WIFI_STATE_FAILED:       stateStr = "실패"; break;
        case WIFI_STATE_SCANNING:     stateStr = "스캔 중"; break;
        default:                      stateStr = "알 수 없음"; break;
    }
    
    Serial.printf("║ 상태: %-31s ║\n", stateStr);
    
    if (isConnected()) {
        Serial.printf("║ SSID: %-31s ║\n", WiFi.SSID().c_str());
        char ipBufW[16]; WiFi.localIP().toString().toCharArray(ipBufW, sizeof(ipBufW));
        Serial.printf("║ IP: %-33s ║\n", ipBufW);
        Serial.printf("║ RSSI: %d dBm                          ║\n", getRSSI());
        Serial.printf("║ 채널: %d                              ║\n", WiFi.channel());
    }
    
    Serial.printf("║ 자동 재연결: %-24s ║\n", 
                  autoReconnectEnabled ? "활성화" : "비활성화");
    Serial.printf("║ 저장된 AP: %d개                       ║\n", apList.size());
    
    Serial.println("╚═══════════════════════════════════════╝\n");
}

bool WiFiResilience::testConnectivity() {
    if (!isConnected()) {
        Serial.println("[WiFiResilience] WiFi 연결되지 않음");
        return false;
    }
    
    Serial.println("[WiFiResilience] 연결 테스트 중...");
    
    // DNS 테스트
    IPAddress testIP;
    if (!WiFi.hostByName("www.google.com", testIP)) {
        Serial.println("[WiFiResilience] ❌ DNS 실패");
        return false;
    }
    
    char dnsIpBuf[16]; testIP.toString().toCharArray(dnsIpBuf, sizeof(dnsIpBuf));
    Serial.printf("[WiFiResilience] ✅ DNS 성공: %s\n", dnsIpBuf);
    return true;
}

// ================================================================
// 내부 메서드
// ================================================================
bool WiFiResilience::connectToAP(int8_t index, uint32_t timeout) {
    if (index < 0 || index >= apList.size()) {
        return false;
    }
    
    APInfo& ap = apList[index];
    Serial.printf("[WiFiResilience] 연결 시도: %s\n", ap.ssid);
    
    WiFi.begin(ap.ssid, ap.password);
    
    if (waitForConnection(timeout)) {
        currentAPIndex = index;
        ap.connectionCount++;
        ap.lastConnected = millis() / 1000;
        ap.rssi = WiFi.RSSI();
        
        saveAPList();
        
        Serial.printf("[WiFiResilience] ✅ 연결 성공: %s (RSSI: %d dBm)\n", 
                      ap.ssid, ap.rssi);
        return true;
    }
    
    Serial.printf("[WiFiResilience] ❌ 연결 실패: %s\n", ap.ssid);
    return false;
}

int8_t WiFiResilience::findBestAP() {
    if (apList.empty()) return -1;
    
    int8_t bestIndex = 0;
    uint32_t bestScore = 0;
    
    for (uint8_t i = 0; i < apList.size(); i++) {
        if (!apList[i].enabled) continue;
        
        // 점수 계산: 최근 연결 + 연결 횟수
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
    
    Serial.println("[WiFiResilience] ✅ WiFi 연결됨");
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
    
    Serial.println("[WiFiResilience] ⚠️  WiFi 연결 끊김");
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
