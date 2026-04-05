// InitializationHelper.h
#pragma once
#include <Arduino.h>

class InitializationHelper {
public:
    struct InitResult {
        bool success;
        const char* component;
        const char* errorMessage;
    };
    
    template<typename T>
    static InitResult initManager(T& manager, const char* name) {
        InitResult result;
        result.component = name;
        Serial.printf("[Init] %s 초기화 중...\n", name);
        try {
            manager.begin();
            result.success = true;
            result.errorMessage = nullptr;
            Serial.printf("[Init] OK %s\n", name);
        } catch (...) {
            result.success = false;
            result.errorMessage = "Exception";
            Serial.printf("[Init] FAIL %s\n", name);
        }
        return result;
    }
    
    static bool checkCritical(const InitResult& result, bool enterSafeMode = true);
    static void checkOptional(const InitResult& result);
private:
    static uint8_t failedCount;
    static uint8_t criticalFailures;
};

#define INIT_MANAGER(manager, name, critical) \
    do { \
        auto result = InitializationHelper::initManager(manager, name); \
        if (critical && !result.success) { \
            InitializationHelper::checkCritical(result); \
            return; \
        } else if (!result.success) { \
            InitializationHelper::checkOptional(result); \
        } \
    } while(0)
