// ================================================================
// SafeMode.h - 안전 모드 시스템 (Phase 3-1)
// ================================================================
#pragma once

#include <Arduino.h>
#include <Preferences.h>

// ================================================================
// 안전 모드 설정
// ================================================================
#define SAFE_MODE_MAX_BOOT_FAILURES    3    // 최대 부팅 실패 허용 횟수
#define SAFE_MODE_BOOT_TIMEOUT         30   // 부팅 완료 판정 시간 (초)
#define SAFE_MODE_PREFERENCE_KEY       "safemode"

// ================================================================
// 부팅 실패 원인
// ================================================================
enum BootFailureReason {
    BOOT_SUCCESS,
    BOOT_WATCHDOG_TIMEOUT,
    BOOT_CONFIG_CORRUPTED,
    BOOT_HARDWARE_FAILURE,
    BOOT_MEMORY_ERROR,
    BOOT_SENSOR_FAILURE,
    BOOT_NETWORK_TIMEOUT,
    BOOT_UNKNOWN_ERROR
};

// ================================================================
// 안전 모드 옵션
// ================================================================
enum SafeModeOption {
    SAFE_RESTORE_BACKUP,        // 백업에서 복원
    SAFE_FACTORY_RESET,         // 공장 초기화
    SAFE_DIAGNOSTIC_MODE,       // 진단 모드
    SAFE_CONTINUE_ANYWAY,       // 계속 진행
    SAFE_REBOOT                 // 재부팅
};

// ================================================================
// 부팅 정보
// ================================================================
struct BootInfo {
    uint32_t bootCount;            // 총 부팅 횟수
    uint32_t successfulBoots;      // 성공한 부팅 횟수
    uint32_t failedBoots;          // 실패한 부팅 횟수
    uint32_t consecutiveFailures;  // 연속 실패 횟수
    BootFailureReason lastFailure; // 마지막 실패 원인
    uint32_t lastBootTime;         // 마지막 부팅 시각
};

// ================================================================
// 안전 모드 관리자 클래스
// ================================================================
class SafeMode {
public:
    // ── 초기화 ──
    void begin();
    
    // ── 부팅 관리 ──
    bool checkBootStatus();           // 부팅 상태 체크
    void markBootStart();             // 부팅 시작 표시
    void markBootSuccess();           // 부팅 성공 표시
    void markBootFailure(BootFailureReason reason);  // 부팅 실패 기록
    
    // ── 안전 모드 ──
    bool isInSafeMode();              // 안전 모드 여부
    void enterSafeMode(BootFailureReason reason);    // 안전 모드 진입
    void exitSafeMode();              // 안전 모드 해제
    
    // ── 복구 옵션 ──
    bool restoreFromBackup();         // 백업 복원
    bool factoryReset();              // 공장 초기화
    bool diagnosticMode();            // 진단 모드 진입
    
    // ── 상태 조회 ──
    BootInfo getBootInfo();
    uint32_t getConsecutiveFailures();
    BootFailureReason getLastFailureReason();
    bool shouldEnterSafeMode();
    
    // ── 통계 ──
    void resetBootStats();
    void printBootInfo();
    void printSafeModeStatus();
    
    // ── UI 지원 ──
    void drawSafeModeScreen();
    SafeModeOption handleSafeModeTouch(uint16_t x, uint16_t y);

private:
    BootInfo bootInfo;
    bool inSafeMode;
    bool bootSuccessMarked;
    uint32_t bootStartTime;
    
    // Preferences 객체
    Preferences prefs;
    
    // ── 내부 메서드 ──
    void loadBootInfo();
    void saveBootInfo();
    void incrementBootCount();
    void resetFailureCount();
    const char* getFailureReasonString(BootFailureReason reason);
};

// ================================================================
// 전역 인스턴스
// ================================================================
extern SafeMode safeMode;

// ================================================================
// 편의 매크로
// ================================================================
#define SAFE_MODE_CHECK() safeMode.checkBootStatus()
#define SAFE_MODE_SUCCESS() safeMode.markBootSuccess()
#define SAFE_MODE_FAIL(reason) safeMode.markBootFailure(reason)
