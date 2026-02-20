/*
 * CloudManager.h - v3.9.1 Phase 1 최적화
 * ThingSpeak 클라우드 통신 + String → char[] 변환
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "ThingSpeak.h"
#include "Config.h"

// ──────────── ThingSpeak 업로드 간격 ─────────────────────
#define UPDATE_INTERVAL  (60 * 1000)  // 60초 (ThingSpeak 무료 제한)

// ──────────── 데이터 구조체 ───────────────────────────────
struct CloudDataPoint {
    float pressure;
    float temperature;
    float current;
    float healthScore;
    uint32_t timestamp;
};

// ──────────── CloudManager 클래스 ─────────────────────────
class CloudManager {
public:
    CloudManager();
    
    // 초기화
    bool begin();
    
    // v3.8: 확장된 업로드
    bool uploadExtendedData();      // 기본 + 건강도 데이터
    bool uploadTrendData();         // 추세 분석 데이터 (별도 채널)
    bool uploadAlertData(           // 알림 데이터 (별도 채널)
        MaintenanceLevel level,
        float healthScore,
        const char* message
    );
    
    // 레거시 (기존 호환)
    bool uploadData(
        float pressure,
        float temperature,
        float current,
        float healthScore,
        int mode,
        int errorStatus,
        float uptime,
        int errorCode
    );
    
    // 업로드 타이밍
    bool shouldUpdate();
    
    // 연결 상태
    bool isCloudConnected();
    
    // 데이터 버퍼링
    void bufferData(float pressure, float temperature, float current, float healthScore);
    CloudDataPoint getBufferedData();
    
    // 통계
    void printStatistics();
    
    // v3.9.1: String → char[] 변환
    void getSystemStatusString(char* buffer, size_t size);
    
private:
    WiFiClient client;
    CloudDataPoint dataBuffer;
    uint32_t lastUpdateTime;
    bool isConnected;
};
