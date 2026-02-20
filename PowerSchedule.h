// PowerSchedule.h (선택 사항)
#ifndef POWER_SCHEDULE_H
#define POWER_SCHEDULE_H

#include "WiFiPowerManager.h"

struct PowerScheduleEntry {
    uint8_t startHour;
    uint8_t startMinute;
    uint8_t endHour;
    uint8_t endMinute;
    WiFiPowerMode mode;
};

class PowerScheduler {
private:
    PowerScheduleEntry schedule[8];
    uint8_t scheduleCount;
    
public:
    PowerScheduler() : scheduleCount(0) {}
    
    void addSchedule(uint8_t startH, uint8_t startM, 
                    uint8_t endH, uint8_t endM, WiFiPowerMode mode) {
        if (scheduleCount < 8) {
            schedule[scheduleCount++] = {startH, startM, endH, endM, mode};
        }
    }
    
    void update() {
        // Get current time (RTC 또는 NTP 필요)
        // 현재 시간에 맞는 스케줄 적용
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) return;
        
        uint16_t currentMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
        
        for (uint8_t i = 0; i < scheduleCount; i++) {
            uint16_t startMinutes = schedule[i].startHour * 60 + schedule[i].startMinute;
            uint16_t endMinutes = schedule[i].endHour * 60 + schedule[i].endMinute;
            
            if (currentMinutes >= startMinutes && currentMinutes < endMinutes) {
                wifiPowerManager.setPowerMode(schedule[i].mode);
                break;
            }
        }
    }
};

#endif // POWER_SCHEDULE_H