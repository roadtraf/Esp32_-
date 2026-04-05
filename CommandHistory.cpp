// CommandHistory.cpp
#include "CommandHistory.h"

CommandHistory commandHistory;

void CommandHistory::add(const char* cmd) {
    if (!cmd || strlen(cmd) == 0) return;
    if (count > 0 && strcmp(history[(count - 1) % MAX_HISTORY], cmd) == 0) return;
    
    strncpy(history[count % MAX_HISTORY], cmd, MAX_CMD_LENGTH - 1);
    history[count % MAX_HISTORY][MAX_CMD_LENGTH - 1] = '\0';
    count++;
    currentIndex = count;
}

const char* CommandHistory::previous() {
    if (count == 0) return nullptr;
    if (currentIndex > 0) currentIndex--;
    return history[currentIndex % MAX_HISTORY];
}

const char* CommandHistory::next() {
    if (count == 0 || currentIndex >= count) return nullptr;
    currentIndex++;
    if (currentIndex >= count) return nullptr;
    return history[currentIndex % MAX_HISTORY];
}

void CommandHistory::print() const {
    Serial.println("\n=== Command History ===");
    if (count == 0) {
        Serial.println("  (empty)");
    } else {
        int start = (count > MAX_HISTORY) ? (count - MAX_HISTORY) : 0;
        int end = count;
        for (int i = start; i < end; i++) {
            Serial.printf("  %d: %s\n", i + 1, history[i % MAX_HISTORY]);
        }
    }
    Serial.println("=======================\n");
}

void CommandHistory::clear() {
    count = 0;
    currentIndex = 0;
    for (int i = 0; i < MAX_HISTORY; i++) {
        history[i][0] = '\0';
    }
}
