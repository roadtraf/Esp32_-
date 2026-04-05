// MemoryPool.h
#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <Arduino.h>

// Simple Memory Pool for frequent allocations
template<size_t BLOCK_SIZE, size_t POOL_SIZE>
class MemoryPool {
private:
    struct Block {
        uint8_t data[BLOCK_SIZE];
        bool inUse;
    };
    
    Block pool[POOL_SIZE];
    SemaphoreHandle_t poolMutex;
    
public:
    MemoryPool() {
        poolMutex = xSemaphoreCreateMutex();
        for (size_t i = 0; i < POOL_SIZE; i++) {
            pool[i].inUse = false;
        }
    }
    
    ~MemoryPool() {
        if (poolMutex) {
            vSemaphoreDelete(poolMutex);
        }
    }
    
    void* allocate() {
        if (xSemaphoreTake(poolMutex, portMAX_DELAY) == pdTRUE) {
            for (size_t i = 0; i < POOL_SIZE; i++) {
                if (!pool[i].inUse) {
                    pool[i].inUse = true;
                    xSemaphoreGive(poolMutex);
                    return pool[i].data;
                }
            }
            xSemaphoreGive(poolMutex);
        }
        return nullptr;  // Pool exhausted
    }
    
    void deallocate(void* ptr) {
        if (!ptr) return;
        
        if (xSemaphoreTake(poolMutex, portMAX_DELAY) == pdTRUE) {
            for (size_t i = 0; i < POOL_SIZE; i++) {
                if (pool[i].data == ptr) {
                    pool[i].inUse = false;
                    break;
                }
            }
            xSemaphoreGive(poolMutex);
        }
    }
    
    size_t getUsedBlocks() const {
        size_t count = 0;
        for (size_t i = 0; i < POOL_SIZE; i++) {
            if (pool[i].inUse) count++;
        }
        return count;
    }
    
    size_t getAvailableBlocks() const {
        return POOL_SIZE - getUsedBlocks();
    }
};

// Global Memory Pools
extern MemoryPool<256, 8> smallPool;    // 8 blocks of 256 bytes
extern MemoryPool<512, 4> mediumPool;   // 4 blocks of 512 bytes
extern MemoryPool<1024, 2> largePool;   // 2 blocks of 1024 bytes

#endif // MEMORY_POOL_H