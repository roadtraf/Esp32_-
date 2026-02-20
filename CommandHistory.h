// CommandHistory.h
#pragma once
#include <Arduino.h>

#define MAX_HISTORY 10
#define MAX_CMD_LENGTH 64

class CommandHistory {
public:
    CommandHistory() : count(0), currentIndex(0) {
        for (int i = 0; i < MAX_HISTORY; i++) {
            history[i][0] = '\0';
        }
    }
    
    void add(const char* cmd);
    const char* previous();
    const char* next();
    void print() const;
    void clear();
    uint16_t getCount() const { return min(count, MAX_HISTORY); }

private:
    char history[MAX_HISTORY][MAX_CMD_LENGTH];
    uint16_t count;
    uint16_t currentIndex;
};

extern CommandHistory commandHistory;
