// ================================================================
// EnhancedWatchdog_Hardened.cpp - WDT   (v3.9.4)
// ================================================================
// [  #2]
//
//  :
//   - esp_task_wdt_init(10, true)  10 
//   - WiFi    10  WDT reset 
//   -     WDT 
//   - esp_task_wdt_add(NULL)   (loop) 
//      FreeRTOS    
//
// :
//   -  WDT : 10s  15s (WiFi )
//   - esp_task_wdt_add()      
//   - Brownout     
//   -     (esp_reset_reason() )
//   - RTC   Preferences  ( )
//   - feed()  esp_task_wdt_reset() +   
// ================================================================
#include "EnhancedWatchdog.h"
#include "HardenedConfig.h"
#include <Preferences.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

//  
EnhancedWatchdog enhancedWatchdog;

// Preferences 
static Preferences wdtPrefs;

// ================================================================
//    
// ================================================================
static RestartReason classifyResetReason() {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:   return RESTART_POWER_ON;
        case ESP_RST_BROWNOUT:  return RESTART_WATCHDOG;   // Brownout 
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
        case ESP_RST_POWERON:   return " ";
        case ESP_RST_BROWNOUT:  return " Brownout ( )";
        case ESP_RST_TASK_WDT:  return " Task WDT";
        case ESP_RST_INT_WDT:   return " Interrupt WDT";
        case ESP_RST_WDT:       return " WDT";
        case ESP_RST_SW:        return " ";
        case ESP_RST_PANIC:     return " Panic/Exception";
        case ESP_RST_DEEPSLEEP: return " ";
        default:                return "  ";
    }
}

// ================================================================
// 
// ================================================================
void EnhancedWatchdog::begin(uint32_t timeout) {
    Serial.println("[EnhancedWDT] v3.9.4 Hardened ...");

    taskCount     = 0;
    enabled       = true;
    startTime     = millis();
    lastUpdateTime = millis();

    //   
    for (uint8_t i = 0; i < MAX_TASK_MONITORS; i++) {
        tasks[i].name[0]  = '\0';
        tasks[i].enabled  = false;
        tasks[i].status   = TASK_NOT_MONITORED;
    }

    //     
    esp_reset_reason_t hwReason = esp_reset_reason();
    Serial.printf("[EnhancedWDT]  : %s\n", resetReasonStr(hwReason));

    // Brownout 
    if (hwReason == ESP_RST_BROWNOUT) {
        Serial.println("[EnhancedWDT]  Brownout !");
        Serial.println("[EnhancedWDT]        ");
        Serial.println("[EnhancedWDT]         ");
    }

    // WDT reset 
    if (hwReason == ESP_RST_TASK_WDT || hwReason == ESP_RST_INT_WDT ||
        hwReason == ESP_RST_WDT) {
        Serial.println("[EnhancedWDT]  WDT Reset !");
        Serial.println("[EnhancedWDT]       ");
    }

    //    (Preferences)
    loadRestartInfo();

    //    
    RestartReason classified = classifyResetReason();
    if (classified != RESTART_POWER_ON && classified != rtcRestartInfo.reason) {
        //    hw reason 
        rtcRestartInfo.reason = classified;
    }

    //  [2]  WDT  
    // : timeout    WiFi  
    uint32_t actualTimeout = (timeout < WDT_TIMEOUT_HW) ? WDT_TIMEOUT_HW : timeout;
    Serial.printf("[EnhancedWDT] HW WDT : %us\n", actualTimeout);

    // ESP-IDF   WDT 
    const esp_task_wdt_config_t wdt_cfg = {
    .timeout_ms     = actualTimeout * 1000U,
    .idle_core_mask = (1U << portNUM_PROCESSORS) - 1U,
    .trigger_panic  = true,
};
    esp_task_wdt_init(&wdt_cfg);
    
    //  (setup/loop) WDT 
    esp_task_wdt_add(NULL);

    Serial.println("[EnhancedWDT]   ");

    //    
    if (rtcRestartInfo.reason != RESTART_NONE &&
        rtcRestartInfo.reason != RESTART_POWER_ON) {
        Serial.println("[EnhancedWDT]      :");
        printRestartHistory();
    }
}

// ================================================================
//  
// ================================================================
bool EnhancedWatchdog::registerTask(const char* name, uint32_t checkInInterval) {
    if (taskCount >= MAX_TASK_MONITORS) {
        Serial.printf("[EnhancedWDT]     (%s)\n", name);
        return false;
    }

    if (findTask(name) >= 0) {
        Serial.printf("[EnhancedWDT]    : %s\n", name);
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

    Serial.printf("[EnhancedWDT]  : %-16s ( : %lums)\n",
                  name, checkInInterval);
    return true;
}

// ================================================================
//   
// ================================================================
void EnhancedWatchdog::unregisterTask(const char* name) {
    int8_t idx = findTask(name);
    if (idx < 0) return;

    if (idx < taskCount - 1) {
        tasks[idx] = tasks[taskCount - 1];
    }
    taskCount--;
    Serial.printf("[EnhancedWDT]  : %s\n", name);
}

// ================================================================
//  -    
// ================================================================
void EnhancedWatchdog::checkIn(const char* name) {
    int8_t idx = findTask(name);
    if (idx < 0) return;

    TaskInfo* task = &tasks[idx];
    task->lastCheckIn    = millis();
    task->totalCheckins++;
    task->missedCheckins = 0;
    task->status         = TASK_HEALTHY;

    //  WDT feed 
    esp_task_wdt_reset();
}

// ================================================================
//   (loop  )
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
//   
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
//   
// ================================================================
void EnhancedWatchdog::handleStalledTask(TaskInfo* task) {
    Serial.println("\n");
    Serial.println("      ! v3.9.4            ");
    Serial.println("");
    Serial.printf(" : %-28s\n", task->name);
    Serial.printf(" : %d                      \n", task->missedCheckins);
    Serial.printf(" : %lu ms                          \n", millis() - task->lastCheckIn);
    Serial.println("                                       ");
    Serial.println(" 5  ...                     ");
    Serial.println("\n");

    //   
    Serial.printf("[WDT]  : %u bytes\n", esp_get_free_heap_size());

    vTaskDelay(pdMS_TO_TICKS(5000));
    forceRestart(RESTART_DEADLOCK, task->name);
}

// ================================================================
//  
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
// 
// ================================================================
uint32_t    EnhancedWatchdog::getUptimeSeconds()    { return (millis() - startTime) / 1000; }
uint32_t    EnhancedWatchdog::getTotalRestarts()     { return rtcRestartInfo.restartCount; }
RestartInfo EnhancedWatchdog::getLastRestartInfo()   { return rtcRestartInfo; }

// ================================================================
// 
// ================================================================
void EnhancedWatchdog::enable()  { enabled = true;  Serial.println("[WDT] "); }
void EnhancedWatchdog::disable() { enabled = false; Serial.println("[WDT] "); }

void EnhancedWatchdog::feed() {
    esp_task_wdt_reset();
}

void EnhancedWatchdog::forceRestart(RestartReason reason, const char* taskName) {
    Serial.printf("[WDT]   (: %d)\n", reason);
    saveRestartInfo(reason, taskName);
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP.restart();
}

// ================================================================
// Preferences /
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
//  
// ================================================================
void EnhancedWatchdog::printStatus() {
    Serial.println("\n");
    Serial.println("  EnhancedWatchdog v3.9.4 Hardened    ");
    Serial.println("");
    Serial.printf( " : %-27s\n", enabled ? "" : "");
    Serial.printf( " :   %-27lu\n", getUptimeSeconds());
    Serial.printf( " : %-27d\n", taskCount);
    Serial.printf( " :   %-27s\n", isHealthy() ? " " : "  ");
    Serial.printf( " :     %-27u\n", esp_get_free_heap_size());
    Serial.println("");

    for (uint8_t i = 0; i < taskCount; i++) {
        TaskInfo* t = &tasks[i];
        const char* s;
        switch (t->status) {
            case TASK_HEALTHY:  s = ""; break;
            case TASK_SLOW:     s = ""; break;
            case TASK_STALLED:  s = ""; break;
            case TASK_DEADLOCK: s = ""; break;
            default:            s = ""; break;
        }
        uint32_t e = millis() - t->lastCheckIn;
        Serial.printf(" %s %-14s %6lums / %6lums \n",
                      s, t->name, e, t->checkInInterval);
    }
    Serial.println("\n");
}

void EnhancedWatchdog::printTaskDetails(const char* name) {
    int8_t idx = findTask(name);
    if (idx < 0) { Serial.printf("[WDT] : %s\n", name); return; }
    TaskInfo* t = &tasks[idx];
    Serial.printf("[WDT] %s:  %lu,  %lu,  %lums \n",
                  t->name, t->totalCheckins, t->missedCheckins,
                  millis() - t->lastCheckIn);
}

void EnhancedWatchdog::printRestartHistory() {
    const char* r;
    switch (rtcRestartInfo.reason) {
        case RESTART_WATCHDOG:     r = "WDT "; break;
        case RESTART_DEADLOCK:     r = ""; break;
        case RESTART_TASK_STALLED: r = " "; break;
        case RESTART_MANUAL:       r = ""; break;
        case RESTART_OTA:          r = "OTA"; break;
        case RESTART_POWER_ON:     r = ""; break;
        default:                   r = "  "; break;
    }
    Serial.printf("[WDT]  : =%s, =%lu, =%s\n",
                  r, rtcRestartInfo.restartCount, rtcRestartInfo.taskName);
}

int8_t EnhancedWatchdog::findTask(const char* name) {
    for (uint8_t i = 0; i < taskCount; i++) {
        if (strcmp(tasks[i].name, name) == 0) return i;
    }
    return -1;
}
