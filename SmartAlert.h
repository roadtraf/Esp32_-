// ================================================================
// SmartAlert.h  —  v3.8.2 스마트 알림 시스템
// ================================================================
#pragma once
#include "Config.h"
#include "HealthMonitor.h"
#include <WiFiClientSecure.h>

// ─────────────────── 알림 설정 구조체 ───────────────────────
struct AlertConfig {
    // 시간 필터
    bool timeFilterEnabled;       // 시간대 필터 활성화
    uint8_t startHour;            // 알림 시작 시간 (0-23)
    uint8_t endHour;              // 알림 종료 시간 (0-23)
    bool weekendAlert;            // 주말 알림 허용
    
    // 우선순위 설정
    bool urgentAlways;            // URGENT는 항상 알림
    bool criticalAlways;          // CRITICAL 에러는 항상 알림
    
    // 알림 채널
    bool buzzerEnabled;           // 부저 알림
    bool displayEnabled;          // 화면 팝업
    bool emailEnabled;            // 이메일 알림
    bool smsEnabled;              // SMS 알림 (선택)
    
    // 이메일 설정
    char smtpServer[64];          // SMTP 서버 (예: smtp.gmail.com)
    uint16_t smtpPort;            // SMTP 포트 (예: 587)
    char emailFrom[64];           // 발신 이메일
    char emailPassword[64];       // 앱 비밀번호
    char emailTo[64];             // 수신 이메일
    
    // SMS 설정 (선택)
    char smsApiKey[64];           // SMS API 키
    char phoneNumber[16];         // 수신 전화번호
    
    // 알림 간격
    uint32_t minAlertInterval;    // 최소 알림 간격 (ms)
};

// ─────────────────── 알림 이력 ──────────────────────────────
struct AlertHistory {
    uint32_t timestamp;
    MaintenanceLevel level;
    ErrorCode errorCode;
    bool emailSent;
    bool smsSent;
    char message[128];
};

#define MAX_ALERT_HISTORY 50

// ─────────────────── SmartAlert 클래스 ──────────────────────
class SmartAlert {
public:
    SmartAlert();
    
    // 초기화
    void begin();
    void loadConfig();
    void saveConfig();
    
    // 설정
    void setConfig(const AlertConfig& cfg);
    AlertConfig getConfig() const { return config; }
    
    // 알림 체크 및 발송
    bool shouldAlert(MaintenanceLevel level, ErrorCode error = ERROR_NONE);
    void sendAlert(MaintenanceLevel level, float healthScore, const char* message = nullptr);
    void sendErrorAlert(ErrorCode error, const char* message);
    
    // 개별 알림 채널
    void sendBuzzerAlert(MaintenanceLevel level);
    void sendDisplayAlert(MaintenanceLevel level, float healthScore, const char* message);
    bool sendEmail(const char* subject, const char* body);
    bool sendSMS(const char* message);
    
    // 시간 체크
    bool isWorkingHours();
    bool isWeekend();
    
    // 알림 이력
    void addToHistory(MaintenanceLevel level, ErrorCode error, const char* message);
    AlertHistory* getHistory(uint8_t& count);
    void clearHistory();
    
    // 통계
    uint32_t getTotalAlertsSent();
    uint32_t getEmailsSent();
    uint32_t getSmsSent();
    uint32_t getLastAlertTime();
    
private:
    AlertConfig config;
    bool initialized;
    
    // 알림 이력
    AlertHistory history[MAX_ALERT_HISTORY];
    uint8_t historyCount;
    uint8_t historyIndex;
    
    // 통계
    uint32_t totalAlerts;
    uint32_t emailsSent;
    uint32_t smsSent;
    uint32_t lastAlertTime[5];  // 레벨별 마지막 알림 시간
    
    // 내부 함수
    bool canSendAlert(MaintenanceLevel level);
    void formatEmailBody(char* buffer, size_t size, 
                        MaintenanceLevel level, 
                        float healthScore, 
                        const char* message);
    void formatSmsMessage(char* buffer, size_t size,
                         MaintenanceLevel level, 
                         float healthScore);
                         
    // 이메일 전송 헬퍼
    bool connectToSMTP(WiFiClientSecure& client);
    bool sendSMTPCommand(WiFiClientSecure& client, const char* command, const char* expectedResponse);
};

// ─────────────────── 전역 인스턴스 ──────────────────────────
extern SmartAlert smartAlert;

// ─────────────────── 기본 설정 ──────────────────────────────
#define DEFAULT_START_HOUR 8
#define DEFAULT_END_HOUR 18
#define DEFAULT_MIN_ALERT_INTERVAL (15 * 60 * 1000)  // 15분
