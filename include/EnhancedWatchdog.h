// ================================================================
// EnhancedWatchdog.h -  Watchdog  (Phase 3-1)
// ================================================================
#pragma once

#include <Arduino.h>
#include <esp_task_wdt.h>

// ================================================================
// Watchdog 
// ================================================================
#define WDT_TIMEOUT_SECONDS     10      // Watchdog  ()
#define TASK_CHECK_INTERVAL     1000    //    (ms)
#define MAX_TASK_MONITORS       8       //    
#define DEADLOCK_THRESHOLD      3       //    (  )

// ================================================================
//  
// ================================================================
enum TaskStatus {
    TASK_HEALTHY,           // 
    TASK_SLOW,              //  ()
    TASK_STALLED,           //  ()
    TASK_DEADLOCK,          // 
    TASK_NOT_MONITORED      //   
};

// ================================================================
//  
// ================================================================
struct TaskInfo {
    char name[24];              //  
    uint32_t lastCheckIn;       //   
    uint32_t checkInInterval;   //    (ms)
    uint32_t missedCheckins;    //   
    uint32_t totalCheckins;     //   
    TaskStatus status;          //  
    bool enabled;               //   
};

// ================================================================
//  
// ================================================================
enum RestartReason {
    RESTART_NONE,
    RESTART_WATCHDOG,           // Watchdog 
    RESTART_DEADLOCK,           //  
    RESTART_TASK_STALLED,       //  
    RESTART_MANUAL,             //  
    RESTART_OTA,                // OTA 
    RESTART_POWER_ON,           //  
    RESTART_UNKNOWN             //   
};

// ================================================================
//   (RTC  )
// ================================================================
struct RestartInfo {
    RestartReason reason;
    uint32_t timestamp;
    char taskName[16];
    uint32_t restartCount;
};

// ================================================================
// Enhanced Watchdog 
// ================================================================
class EnhancedWatchdog {
public:
    //   
    void begin(uint32_t timeout = WDT_TIMEOUT_SECONDS);
    
    //    
    bool registerTask(const char* name, uint32_t checkInInterval);
    void unregisterTask(const char* name);
    
    //    
    void checkIn(const char* name);
    
    //   
    void update();  //   (loop)
    
    //    
    TaskStatus getTaskStatus(const char* name);
    TaskInfo* getTaskInfo(const char* name);
    uint8_t getRegisteredTaskCount();
    bool isHealthy();
    
    //   
    uint32_t getUptimeSeconds();
    uint32_t getTotalRestarts();
    RestartInfo getLastRestartInfo();
    
    //   
    void enable();
    void disable();
    void feed();  //  watchdog feed
    void forceRestart(RestartReason reason, const char* taskName = nullptr);
    
    //   
    void printStatus();
    void printTaskDetails(const char* name);
    void printRestartHistory();

private:
    TaskInfo tasks[MAX_TASK_MONITORS];
    uint8_t taskCount;
    bool enabled;
    uint32_t startTime;
    uint32_t lastUpdateTime;
    
    // RTC  (  )
    RestartInfo rtcRestartInfo;
    
    //    
    int8_t findTask(const char* name);
    void checkTasks();
    void handleStalledTask(TaskInfo* task);
    void saveRestartInfo(RestartReason reason, const char* taskName);
    void loadRestartInfo();
};

// ================================================================
//  
// ================================================================
extern EnhancedWatchdog enhancedWatchdog;

// ================================================================
//  
// ================================================================
#define WDT_CHECKIN(taskName) enhancedWatchdog.checkIn(taskName)
#define WDT_FEED() enhancedWatchdog.feed()
