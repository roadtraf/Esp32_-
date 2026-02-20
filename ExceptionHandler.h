// ExceptionHandler.h
#pragma once
#include <Arduino.h>

enum class ExceptionType {
    NONE, SENSOR_FAILURE, MEMORY_ERROR, NETWORK_ERROR,
    HARDWARE_ERROR, INVALID_PARAMETER, TIMEOUT_ERROR
};

struct ExceptionInfo {
    ExceptionType type;
    const char* message;
    const char* file;
    int line;
    uint32_t timestamp;
};

class ExceptionHandler {
public:
    static ExceptionHandler& getInstance() {
        static ExceptionHandler instance;
        return instance;
    }
    
    void recordException(ExceptionType type, const char* message, 
                        const char* file = nullptr, int line = 0);
    ExceptionInfo getLastException() const { return lastException; }
    uint32_t getExceptionCount() const { return exceptionCount; }
    void reset();
    void printLastException() const;
    
private:
    ExceptionHandler() : exceptionCount(0) {
        lastException.type = ExceptionType::NONE;
        lastException.message = nullptr;
        lastException.file = nullptr;
        lastException.line = 0;
        lastException.timestamp = 0;
    }
    ExceptionInfo lastException;
    uint32_t exceptionCount;
};

#define RECORD_EXCEPTION(type, msg) \
    ExceptionHandler::getInstance().recordException(type, msg, __FILE__, __LINE__)
