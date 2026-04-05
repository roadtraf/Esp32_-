// ================================================================
// EnhancedWatchdog.h - 향상된 Watchdog 시스템 (Phase 3-1)
// ================================================================
#pragma once

#include <Arduino.h>
#include <esp_task_wdt.h>

// ================================================================
// Watchdog 설정
// ================================================================
#define WDT_TIMEOUT_SECONDS     10      // Watchdog 타임아웃 (초)
#define TASK_CHECK_INTERVAL     1000    // 태스크 체크 간격 (ms)
#define MAX_TASK_MONITORS       8       // 최대 모니터링 태스크 수
#define DEADLOCK_THRESHOLD      3       // 데드락 판정 임계값 (연속 미응답 횟수)

// ================================================================
// 태스크 상태
// ================================================================
enum TaskStatus {
    TASK_HEALTHY,           // 정상
    TASK_SLOW,              // 느림 (경고)
    TASK_STALLED,           // 정지됨 (위험)
    TASK_DEADLOCK,          // 데드락
    TASK_NOT_MONITORED      // 모니터링 안 됨
};

// ================================================================
// 태스크 정보
// ================================================================
struct TaskInfo {
    char name[24];              // 태스크 이름
    uint32_t lastCheckIn;       // 마지막 체크인 시간
    uint32_t checkInInterval;   // 예상 체크인 간격 (ms)
    uint32_t missedCheckins;    // 연속 미응답 횟수
    uint32_t totalCheckins;     // 총 체크인 횟수
    TaskStatus status;          // 현재 상태
    bool enabled;               // 모니터링 활성화 여부
};

// ================================================================
// 재시작 원인
// ================================================================
enum RestartReason {
    RESTART_NONE,
    RESTART_WATCHDOG,           // Watchdog 타임아웃
    RESTART_DEADLOCK,           // 데드락 감지
    RESTART_TASK_STALLED,       // 태스크 정지
    RESTART_MANUAL,             // 수동 재시작
    RESTART_OTA,                // OTA 업데이트
    RESTART_POWER_ON,           // 전원 켜짐
    RESTART_UNKNOWN             // 알 수 없음
};

// ================================================================
// 재시작 정보 (RTC 메모리 저장용)
// ================================================================
struct RestartInfo {
    RestartReason reason;
    uint32_t timestamp;
    char taskName[16];
    uint32_t restartCount;
};

// ================================================================
// Enhanced Watchdog 클래스
// ================================================================
class EnhancedWatchdog {
public:
    // ── 초기화 ──
    void begin(uint32_t timeout = WDT_TIMEOUT_SECONDS);
    
    // ── 태스크 등록 ──
    bool registerTask(const char* name, uint32_t checkInInterval);
    void unregisterTask(const char* name);
    
    // ── 태스크 체크인 ──
    void checkIn(const char* name);
    
    // ── 모니터링 ──
    void update();  // 주기적으로 호출 (loop에서)
    
    // ── 상태 조회 ──
    TaskStatus getTaskStatus(const char* name);
    TaskInfo* getTaskInfo(const char* name);
    uint8_t getRegisteredTaskCount();
    bool isHealthy();
    
    // ── 통계 ──
    uint32_t getUptimeSeconds();
    uint32_t getTotalRestarts();
    RestartInfo getLastRestartInfo();
    
    // ── 제어 ──
    void enable();
    void disable();
    void feed();  // 하드웨어 watchdog feed
    void forceRestart(RestartReason reason, const char* taskName = nullptr);
    
    // ── 진단 ──
    void printStatus();
    void printTaskDetails(const char* name);
    void printRestartHistory();

private:
    TaskInfo tasks[MAX_TASK_MONITORS];
    uint8_t taskCount;
    bool enabled;
    uint32_t startTime;
    uint32_t lastUpdateTime;
    
    // RTC 메모리 (재부팅 후에도 유지)
    RestartInfo rtcRestartInfo;
    
    // ── 내부 메서드 ──
    int8_t findTask(const char* name);
    void checkTasks();
    void handleStalledTask(TaskInfo* task);
    void saveRestartInfo(RestartReason reason, const char* taskName);
    void loadRestartInfo();
};

// ================================================================
// 전역 인스턴스
// ================================================================
extern EnhancedWatchdog enhancedWatchdog;

// ================================================================
// 편의 매크로
// ================================================================
#define WDT_CHECKIN(taskName) enhancedWatchdog.checkIn(taskName)
#define WDT_FEED() enhancedWatchdog.feed()
