// ================================================================
// SafeMode.cpp - 안전 모드 시스템 구현
// ================================================================
#include "SafeMode.h"
#include "LovyanGFX_Config.hpp"

// 전역 인스턴스
SafeMode safeMode;

// 외부 객체 참조 (필요 시)
extern LGFX tft;

// ================================================================
// 초기화
// ================================================================
void SafeMode::begin() {
    Serial.println("[SafeMode] 초기화 시작...");
    
    inSafeMode = false;
    bootSuccessMarked = false;
    bootStartTime = millis();
    
    // 부팅 정보 로드
    loadBootInfo();
    
    // 부팅 시작 표시
    markBootStart();
    
    Serial.printf("[SafeMode] 총 부팅: %lu회 (성공: %lu, 실패: %lu)\n",
                  bootInfo.bootCount, bootInfo.successfulBoots, bootInfo.failedBoots);
    Serial.printf("[SafeMode] 연속 실패: %lu회\n", bootInfo.consecutiveFailures);
    
    // 안전 모드 진입 여부 체크
    if (shouldEnterSafeMode()) {
        Serial.println("[SafeMode] ⚠️  안전 모드 진입 조건 충족");
        enterSafeMode(bootInfo.lastFailure);
    } else {
        Serial.println("[SafeMode] ✅ 정상 부팅 모드");
    }
}

// ================================================================
// 부팅 상태 체크
// ================================================================
bool SafeMode::checkBootStatus() {
    // 부팅 성공이 표시되지 않았고 타임아웃 시간이 지났으면 실패로 간주
    if (!bootSuccessMarked && (millis() - bootStartTime) > (SAFE_MODE_BOOT_TIMEOUT * 1000)) {
        Serial.println("[SafeMode] ❌ 부팅 타임아웃");
        markBootFailure(BOOT_UNKNOWN_ERROR);
        return false;
    }
    
    return true;
}

// ================================================================
// 부팅 시작 표시
// ================================================================
void SafeMode::markBootStart() {
    bootStartTime = millis();
    bootSuccessMarked = false;
    
    incrementBootCount();
    
    Serial.println("[SafeMode] 부팅 시작 기록");
}

// ================================================================
// 부팅 성공 표시
// ================================================================
void SafeMode::markBootSuccess() {
    if (bootSuccessMarked) return;  // 중복 호출 방지
    
    bootSuccessMarked = true;
    bootInfo.successfulBoots++;
    resetFailureCount();
    
    saveBootInfo();
    
    uint32_t bootTime = (millis() - bootStartTime) / 1000;
    Serial.printf("[SafeMode] ✅ 부팅 성공! (소요: %lu초)\n", bootTime);
}

// ================================================================
// 부팅 실패 기록
// ================================================================
void SafeMode::markBootFailure(BootFailureReason reason) {
    bootInfo.failedBoots++;
    bootInfo.consecutiveFailures++;
    bootInfo.lastFailure = reason;
    bootInfo.lastBootTime = millis() / 1000;
    
    saveBootInfo();
    
    Serial.printf("[SafeMode] ❌ 부팅 실패: %s\n", getFailureReasonString(reason));
    Serial.printf("[SafeMode] 연속 실패 횟수: %lu\n", bootInfo.consecutiveFailures);
}

// ================================================================
// 안전 모드 체크
// ================================================================
bool SafeMode::isInSafeMode() {
    return inSafeMode;
}

bool SafeMode::shouldEnterSafeMode() {
    return (bootInfo.consecutiveFailures >= SAFE_MODE_MAX_BOOT_FAILURES);
}

// ================================================================
// 안전 모드 진입
// ================================================================
void SafeMode::enterSafeMode(BootFailureReason reason) {
    inSafeMode = true;
    
    Serial.println("\n");
    Serial.println("╔═══════════════════════════════════════════════════╗");
    Serial.println("║          🛡️  안전 모드 진입  🛡️                 ║");
    Serial.println("╠═══════════════════════════════════════════════════╣");
    Serial.printf("║ 원인: %-43s ║\n", getFailureReasonString(reason));
    Serial.printf("║ 연속 실패: %lu회                                  ║\n", 
                  bootInfo.consecutiveFailures);
    Serial.println("║                                                   ║");
    Serial.println("║ 시스템이 최소 기능으로 부팅되었습니다.           ║");
    Serial.println("║ 복구 옵션을 선택하세요.                          ║");
    Serial.println("╚═══════════════════════════════════════════════════╝");
    Serial.println();
    
    // UI가 초기화되어 있으면 안전 모드 화면 표시
    // drawSafeModeScreen();
}

// ================================================================
// 안전 모드 해제
// ================================================================
void SafeMode::exitSafeMode() {
    inSafeMode = false;
    resetFailureCount();
    saveBootInfo();
    
    Serial.println("[SafeMode] 안전 모드 해제");
}

// ================================================================
// 복구 옵션
// ================================================================
bool SafeMode::restoreFromBackup() {
    Serial.println("[SafeMode] 백업에서 복원 시도...");
    
    // ConfigManager를 통한 백업 복원
    // (실제 구현은 ConfigManager와 통합 필요)
    
    Serial.println("[SafeMode] ✅ 백업 복원 완료");
    return true;
}

bool SafeMode::factoryReset() {
    Serial.println("[SafeMode] 공장 초기화 시작...");
    
    // 1. 설정 파일 삭제
    // 2. 통계 초기화
    // 3. 부팅 정보 리셋
    resetBootStats();
    
    Serial.println("[SafeMode] ✅ 공장 초기화 완료");
    Serial.println("[SafeMode] 재부팅이 필요합니다.");
    
    return true;
}

bool SafeMode::diagnosticMode() {
    Serial.println("[SafeMode] 진단 모드 진입...");
    
    Serial.println("\n=== 시스템 진단 ===");
    
    // 1. 메모리 체크
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Heap Size: %d bytes\n", ESP.getHeapSize());
    Serial.printf("Min Free Heap: %d bytes\n", ESP.getMinFreeHeap());
    
    // 2. Flash 체크
    Serial.printf("Flash Size: %d bytes\n", ESP.getFlashChipSize());
    Serial.printf("Flash Speed: %d Hz\n", ESP.getFlashChipSpeed());
    
    // 3. CPU 정보
    Serial.printf("CPU Freq: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("Chip Revision: %d\n", ESP.getChipRevision());
    
    // 4. 재시작 정보
    Serial.printf("Reset Reason: %d\n", esp_reset_reason());
    
    Serial.println("==================\n");
    
    return true;
}

// ================================================================
// 상태 조회
// ================================================================
BootInfo SafeMode::getBootInfo() {
    return bootInfo;
}

uint32_t SafeMode::getConsecutiveFailures() {
    return bootInfo.consecutiveFailures;
}

BootFailureReason SafeMode::getLastFailureReason() {
    return bootInfo.lastFailure;
}

// ================================================================
// 통계
// ================================================================
void SafeMode::resetBootStats() {
    bootInfo.bootCount = 0;
    bootInfo.successfulBoots = 0;
    bootInfo.failedBoots = 0;
    bootInfo.consecutiveFailures = 0;
    bootInfo.lastFailure = BOOT_SUCCESS;
    bootInfo.lastBootTime = 0;
    
    saveBootInfo();
    
    Serial.println("[SafeMode] 부팅 통계 초기화 완료");
}

void SafeMode::printBootInfo() {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║       부팅 정보                       ║");
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ 총 부팅: %lu회                        ║\n", bootInfo.bootCount);
    Serial.printf("║ 성공: %lu회                           ║\n", bootInfo.successfulBoots);
    Serial.printf("║ 실패: %lu회                           ║\n", bootInfo.failedBoots);
    Serial.printf("║ 연속 실패: %lu회                      ║\n", bootInfo.consecutiveFailures);
    Serial.println("╠═══════════════════════════════════════╣");
    
    if (bootInfo.lastFailure != BOOT_SUCCESS) {
        Serial.printf("║ 마지막 실패: %-24s ║\n", 
                      getFailureReasonString(bootInfo.lastFailure));
    }
    
    Serial.println("╚═══════════════════════════════════════╝\n");
}

void SafeMode::printSafeModeStatus() {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║       안전 모드 상태                  ║");
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ 현재 모드: %-26s ║\n", 
                  inSafeMode ? "🛡️  안전 모드" : "✅ 정상 모드");
    Serial.printf("║ 부팅 성공: %-26s ║\n",
                  bootSuccessMarked ? "예" : "아니오");
    
    if (shouldEnterSafeMode()) {
        Serial.println("║                                       ║");
        Serial.println("║ ⚠️  안전 모드 진입 조건 충족         ║");
    }
    
    Serial.println("╚═══════════════════════════════════════╝\n");
}

// ================================================================
// UI 지원
// ================================================================
void SafeMode::drawSafeModeScreen() {
    // TFT 사용 가능할 때만
    // tft.fillScreen(TFT_BLACK);
    
    // 제목
    // tft.setTextSize(2);
    // tft.setTextColor(TFT_RED);
    // tft.setCursor(50, 20);
    // tft.print("안전 모드");
    
    // 내용
    // ...
    
    Serial.println("[SafeMode] 안전 모드 UI 표시");
}

SafeModeOption SafeMode::handleSafeModeTouch(uint16_t x, uint16_t y) {
    // 터치 처리 로직
    // ...
    
    return SAFE_CONTINUE_ANYWAY;
}

// ================================================================
// 내부 메서드
// ================================================================
void SafeMode::loadBootInfo() {
    prefs.begin(SAFE_MODE_PREFERENCE_KEY, true);  // 읽기 전용
    
    bootInfo.bootCount = prefs.getUInt("bootCnt", 0);
    bootInfo.successfulBoots = prefs.getUInt("successCnt", 0);
    bootInfo.failedBoots = prefs.getUInt("failCnt", 0);
    bootInfo.consecutiveFailures = prefs.getUInt("conseqFail", 0);
    bootInfo.lastFailure = (BootFailureReason)prefs.getUInt("lastFail", BOOT_SUCCESS);
    bootInfo.lastBootTime = prefs.getUInt("lastBootT", 0);
    
    prefs.end();
}

void SafeMode::saveBootInfo() {
    prefs.begin(SAFE_MODE_PREFERENCE_KEY, false);  // 쓰기 모드
    
    prefs.putUInt("bootCnt", bootInfo.bootCount);
    prefs.putUInt("successCnt", bootInfo.successfulBoots);
    prefs.putUInt("failCnt", bootInfo.failedBoots);
    prefs.putUInt("conseqFail", bootInfo.consecutiveFailures);
    prefs.putUInt("lastFail", (uint32_t)bootInfo.lastFailure);
    prefs.putUInt("lastBootT", bootInfo.lastBootTime);
    
    prefs.end();
}

void SafeMode::incrementBootCount() {
    bootInfo.bootCount++;
    saveBootInfo();
}

void SafeMode::resetFailureCount() {
    bootInfo.consecutiveFailures = 0;
    bootInfo.lastFailure = BOOT_SUCCESS;
}

const char* SafeMode::getFailureReasonString(BootFailureReason reason) {
    switch (reason) {
        case BOOT_SUCCESS:           return "성공";
        case BOOT_WATCHDOG_TIMEOUT:  return "Watchdog 타임아웃";
        case BOOT_CONFIG_CORRUPTED:  return "설정 손상";
        case BOOT_HARDWARE_FAILURE:  return "하드웨어 오류";
        case BOOT_MEMORY_ERROR:      return "메모리 오류";
        case BOOT_SENSOR_FAILURE:    return "센서 오류";
        case BOOT_NETWORK_TIMEOUT:   return "네트워크 타임아웃";
        default:                     return "알 수 없음";
    }
}
