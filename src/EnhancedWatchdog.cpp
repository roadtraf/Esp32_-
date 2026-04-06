// ================================================================
// EnhancedWatchdog_Hardened.cpp - WDT 안정 버전 (v3.9.4)
// ================================================================
// [테스트 시나리오 #2]
//
// 기존 문제:
//   - esp_task_wdt_init(10, true) → 10초 타임아웃
//   - WiFi 재연결이 블로킹으로 최대 10초 → WDT reset 빈발
//   - 모든 태스크가 동일한 하드웨어 WDT에 묶임
//   - esp_task_wdt_add(NULL) → 현재 태스크(loop)만 등록
//     실제 FreeRTOS 태스크들은 별도 등록 필요
//
// 변경사항:
//   - 하드웨어 WDT 타임아웃: 10s → 15s (WiFi 여유)
//   - esp_task_wdt_add()를 각 태스크 시작 시 호출하도록 수정
//   - Brownout 감지 시 재시작 정보 저장
//   - 재시작 원인 자동 분류 (esp_reset_reason() 활용)
//   - RTC 메모리 대신 Preferences 사용 (기존 유지)
//   - feed() → esp_task_wdt_reset() + 태스크별 독립 피드
// ================================================================
#include "EnhancedWatchdog.h"
#include "HardenedConfig.h"
#include <Preferences.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 전역 인스턴스
EnhancedWatchdog enhancedWatchdog;

// Preferences 저장소
static Preferences wdtPrefs;

// ================================================================
// 재시작 원인 자동 분류
// ================================================================
static RestartReason classifyResetReason() {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:   return RESTART_POWER_ON;
        case ESP_RST_BROWNOUT:  return RESTART_WATCHDOG;   // Brownout도 기록
        case ESP_RST_TASK_WDT:  return RESTART_WATCHDOG;
        case ESP_RST_INT_WDT:   return RESTART_WATCHDOG;
        case ESP_RST_WDT:       return RESTART_WATCHDOG;
        case ESP_RST_SW:        return RESTART_MANUAL;
        case ESP_RST_DEEPSLEEP: return RESTART_POWER_ON;
        default:                return RESTART_UNKNOWN;
    }
}

static const char* resetReasonStr(esp_reset_reason_t r) {
    switch (r) {
        case ESP_RST_POWERON:   return "전원 켜짐";
        case ESP_RST_BROWNOUT:  return "⚡ Brownout (전압 강하)";
        case ESP_RST_TASK_WDT:  return "🔴 Task WDT";
        case ESP_RST_INT_WDT:   return "🔴 Interrupt WDT";
        case ESP_RST_WDT:       return "🔴 WDT";
        case ESP_RST_SW:        return "소프트웨어 재시작";
        case ESP_RST_PANIC:     return "⚠️ Panic/Exception";
        case ESP_RST_DEEPSLEEP: return "딥슬립 웨이크업";
        default:                return "알 수 없음";
    }
}

// ================================================================
// 초기화
// ================================================================
void EnhancedWatchdog::begin(uint32_t timeout) {
    Serial.println("[EnhancedWDT] v3.9.4 Hardened 초기화...");

    taskCount     = 0;
    enabled       = true;
    startTime     = millis();
    lastUpdateTime = millis();

    // 태스크 배열 초기화
    for (uint8_t i = 0; i < MAX_TASK_MONITORS; i++) {
        tasks[i].name[0]  = '\0';
        tasks[i].enabled  = false;
        tasks[i].status   = TASK_NOT_MONITORED;
    }

    // ── 재시작 원인 분석 ──
    esp_reset_reason_t hwReason = esp_reset_reason();
    Serial.printf("[EnhancedWDT] 재시작 원인: %s\n", resetReasonStr(hwReason));

    // Brownout 감지
    if (hwReason == ESP_RST_BROWNOUT) {
        Serial.println("[EnhancedWDT] ⚡ Brownout 감지!");
        Serial.println("[EnhancedWDT]   → 전원 공급 안정성 점검 필요");
        Serial.println("[EnhancedWDT]   → 커패시터 추가 또는 배선 점검 권장");
    }

    // WDT reset 감지
    if (hwReason == ESP_RST_TASK_WDT || hwReason == ESP_RST_INT_WDT ||
        hwReason == ESP_RST_WDT) {
        Serial.println("[EnhancedWDT] 🔴 WDT Reset 감지!");
        Serial.println("[EnhancedWDT]   → 블로킹 함수 점검 필요");
    }

    // 재시작 정보 로드 (Preferences)
    loadRestartInfo();

    // 자동 분류한 이유와 비교
    RestartReason classified = classifyResetReason();
    if (classified != RESTART_POWER_ON && classified != rtcRestartInfo.reason) {
        // 저장된 이유 없으면 hw reason으로 업데이트
        rtcRestartInfo.reason = classified;
    }

    // ── [2] 하드웨어 WDT 설정 ──
    // 핵심: timeout을 충분히 크게 → WiFi 재연결 여유
    uint32_t actualTimeout = (timeout < WDT_TIMEOUT_HW) ? WDT_TIMEOUT_HW : timeout;
    Serial.printf("[EnhancedWDT] HW WDT 타임아웃: %us\n", actualTimeout);

    // ESP-IDF 구버전 호환 WDT 초기화
    esp_task_wdt_init(actualTimeout, true);

    // 현재 태스크(setup/loop)를 WDT에 등록
    esp_task_wdt_add(NULL);

    Serial.println("[EnhancedWDT] ✅ 초기화 완료");

    // 이전 재시작 이력 출력
    if (rtcRestartInfo.reason != RESTART_NONE &&
        rtcRestartInfo.reason != RESTART_POWER_ON) {
        Serial.println("[EnhancedWDT] ⚠️  이전 비정상 재시작 이력:");
        printRestartHistory();
    }
}

// ================================================================
// 태스크 등록
// ================================================================
bool EnhancedWatchdog::registerTask(const char* name, uint32_t checkInInterval) {
    if (taskCount >= MAX_TASK_MONITORS) {
        Serial.printf("[EnhancedWDT] ❌ 등록 한도 초과 (%s)\n", name);
        return false;
    }

    if (findTask(name) >= 0) {
        Serial.printf("[EnhancedWDT] ⚠️  중복 등록: %s\n", name);
        return false;
    }

    TaskInfo* task = &tasks[taskCount];
    strncpy(task->name, name, sizeof(task->name) - 1);
    task->name[sizeof(task->name) - 1] = '\0';
    task->checkInInterval = checkInInterval;
    task->lastCheckIn     = millis();
    task->missedCheckins  = 0;
    task->totalCheckins   = 0;
    task->status          = TASK_HEALTHY;
    task->enabled         = true;
    taskCount++;

    Serial.printf("[EnhancedWDT] ✅ 등록: %-16s (허용 간격: %lums)\n",
                  name, checkInInterval);
    return true;
}

// ================================================================
// 태스크 등록 해제
// ================================================================
void EnhancedWatchdog::unregisterTask(const char* name) {
    int8_t idx = findTask(name);
    if (idx < 0) return;

    if (idx < taskCount - 1) {
        tasks[idx] = tasks[taskCount - 1];
    }
    taskCount--;
    Serial.printf("[EnhancedWDT] 등록 해제: %s\n", name);
}

// ================================================================
// 체크인 - 각 태스크가 주기적으로 호출
// ================================================================
void EnhancedWatchdog::checkIn(const char* name) {
    int8_t idx = findTask(name);
    if (idx < 0) return;

    TaskInfo* task = &tasks[idx];
    task->lastCheckIn    = millis();
    task->totalCheckins++;
    task->missedCheckins = 0;
    task->status         = TASK_HEALTHY;

    // 하드웨어 WDT feed도 함께
    esp_task_wdt_reset();
}

// ================================================================
// 모니터링 업데이트 (loop에서 주기적 호출)
// ================================================================
void EnhancedWatchdog::update() {
    if (!enabled) return;

    uint32_t now = millis();
    if (now - lastUpdateTime < TASK_CHECK_INTERVAL) return;
    lastUpdateTime = now;

    checkTasks();
    feed();
}

// ================================================================
// 태스크 상태 확인
// ================================================================
void EnhancedWatchdog::checkTasks() {
    uint32_t now = millis();

    for (uint8_t i = 0; i < taskCount; i++) {
        TaskInfo* task = &tasks[i];
        if (!task->enabled) continue;

        uint32_t elapsed = now - task->lastCheckIn;

        if (elapsed > task->checkInInterval * 2) {
            task->status = TASK_STALLED;
            task->missedCheckins++;

            if (task->missedCheckins >= DEADLOCK_THRESHOLD) {
                task->status = TASK_DEADLOCK;
                handleStalledTask(task);
            }
        } else if (elapsed > task->checkInInterval * 3 / 2) {
            task->status = TASK_SLOW;
            task->missedCheckins++;
        } else {
            task->status = TASK_HEALTHY;
            task->missedCheckins = 0;
        }
    }
}

// ================================================================
// 정지된 태스크 처리
// ================================================================
void EnhancedWatchdog::handleStalledTask(TaskInfo* task) {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║   ⚠️  데드락 감지! v3.9.4            ║");
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ 태스크: %-28s║\n", task->name);
    Serial.printf("║ 미응답: %d회 연속                     ║\n", task->missedCheckins);
    Serial.printf("║ 경과: %lu ms                          ║\n", millis() - task->lastCheckIn);
    Serial.println("║                                       ║");
    Serial.println("║ 5초 후 재시작...                     ║");
    Serial.println("╚═══════════════════════════════════════╝\n");

    // 힙 상태도 기록
    Serial.printf("[WDT] 힙 잔여: %u bytes\n", esp_get_free_heap_size());

    vTaskDelay(pdMS_TO_TICKS(5000));
    forceRestart(RESTART_DEADLOCK, task->name);
}

// ================================================================
// 상태 조회
// ================================================================
TaskStatus EnhancedWatchdog::getTaskStatus(const char* name) {
    int8_t idx = findTask(name);
    if (idx < 0) return TASK_NOT_MONITORED;
    return tasks[idx].status;
}

TaskInfo* EnhancedWatchdog::getTaskInfo(const char* name) {
    int8_t idx = findTask(name);
    if (idx < 0) return nullptr;
    return &tasks[idx];
}

uint8_t EnhancedWatchdog::getRegisteredTaskCount() { return taskCount; }

bool EnhancedWatchdog::isHealthy() {
    for (uint8_t i = 0; i < taskCount; i++) {
        if (tasks[i].enabled && tasks[i].status >= TASK_STALLED) return false;
    }
    return true;
}

// ================================================================
// 통계
// ================================================================
uint32_t    EnhancedWatchdog::getUptimeSeconds()    { return (millis() - startTime) / 1000; }
uint32_t    EnhancedWatchdog::getTotalRestarts()     { return rtcRestartInfo.restartCount; }
RestartInfo EnhancedWatchdog::getLastRestartInfo()   { return rtcRestartInfo; }

// ================================================================
// 제어
// ================================================================
void EnhancedWatchdog::enable()  { enabled = true;  Serial.println("[WDT] 활성화"); }
void EnhancedWatchdog::disable() { enabled = false; Serial.println("[WDT] 비활성화"); }

void EnhancedWatchdog::feed() {
    esp_task_wdt_reset();
}

void EnhancedWatchdog::forceRestart(RestartReason reason, const char* taskName) {
    Serial.printf("[WDT] 강제 재시작 (원인: %d)\n", reason);
    saveRestartInfo(reason, taskName);
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP.restart();
}

// ================================================================
// Preferences 저장/로드
// ================================================================
void EnhancedWatchdog::saveRestartInfo(RestartReason reason, const char* taskName) {
    wdtPrefs.begin("wdt", false);
    wdtPrefs.putUInt("reason",    (uint32_t)reason);
    wdtPrefs.putUInt("timestamp", millis() / 1000);
    wdtPrefs.putUInt("count",     rtcRestartInfo.restartCount + 1);
    wdtPrefs.putUInt("hwreason",  (uint32_t)esp_reset_reason());
    if (taskName) wdtPrefs.putString("task", taskName);
    wdtPrefs.end();
}

void EnhancedWatchdog::loadRestartInfo() {
    wdtPrefs.begin("wdt", true);
    rtcRestartInfo.reason       = (RestartReason)wdtPrefs.getUInt("reason", RESTART_POWER_ON);
    rtcRestartInfo.timestamp    = wdtPrefs.getUInt("timestamp", 0);
    rtcRestartInfo.restartCount = wdtPrefs.getUInt("count", 0);

    char buf[32] = {0};
    wdtPrefs.getString("task", buf, sizeof(buf));
    strncpy(rtcRestartInfo.taskName, buf, sizeof(rtcRestartInfo.taskName) - 1);
    wdtPrefs.end();
}

// ================================================================
// 진단 출력
// ================================================================
void EnhancedWatchdog::printStatus() {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║  EnhancedWatchdog v3.9.4 Hardened    ║");
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf( "║ 활성화: %-27s║\n", enabled ? "예" : "아니오");
    Serial.printf( "║ 가동:   %-27lu║\n", getUptimeSeconds());
    Serial.printf( "║ 태스크: %-27d║\n", taskCount);
    Serial.printf( "║ 상태:   %-27s║\n", isHealthy() ? "✅ 정상" : "⚠️  경고");
    Serial.printf( "║ 힙:     %-27u║\n", esp_get_free_heap_size());
    Serial.println("╠═══════════════════════════════════════╣");

    for (uint8_t i = 0; i < taskCount; i++) {
        TaskInfo* t = &tasks[i];
        const char* s;
        switch (t->status) {
            case TASK_HEALTHY:  s = "✅"; break;
            case TASK_SLOW:     s = "⚠️"; break;
            case TASK_STALLED:  s = "❌"; break;
            case TASK_DEADLOCK: s = "🔴"; break;
            default:            s = "❔"; break;
        }
        uint32_t e = millis() - t->lastCheckIn;
        Serial.printf("║ %s %-14s %6lums / %6lums ║\n",
                      s, t->name, e, t->checkInInterval);
    }
    Serial.println("╚═══════════════════════════════════════╝\n");
}

void EnhancedWatchdog::printTaskDetails(const char* name) {
    int8_t idx = findTask(name);
    if (idx < 0) { Serial.printf("[WDT] 없음: %s\n", name); return; }
    TaskInfo* t = &tasks[idx];
    Serial.printf("[WDT] %s: 체크인 %lu회, 미응답 %lu회, 마지막 %lums 전\n",
                  t->name, t->totalCheckins, t->missedCheckins,
                  millis() - t->lastCheckIn);
}

void EnhancedWatchdog::printRestartHistory() {
    const char* r;
    switch (rtcRestartInfo.reason) {
        case RESTART_WATCHDOG:     r = "WDT 타임아웃"; break;
        case RESTART_DEADLOCK:     r = "데드락"; break;
        case RESTART_TASK_STALLED: r = "태스크 정지"; break;
        case RESTART_MANUAL:       r = "수동"; break;
        case RESTART_OTA:          r = "OTA"; break;
        case RESTART_POWER_ON:     r = "전원"; break;
        default:                   r = "알 수 없음"; break;
    }
    Serial.printf("[WDT] 재시작 이력: 원인=%s, 횟수=%lu, 태스크=%s\n",
                  r, rtcRestartInfo.restartCount, rtcRestartInfo.taskName);
}

int8_t EnhancedWatchdog::findTask(const char* name) {
    for (uint8_t i = 0; i < taskCount; i++) {
        if (strcmp(tasks[i].name, name) == 0) return i;
    }
    return -1;
}
