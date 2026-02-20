
#include "Config.h"                 // Arduino.h 포함
#include <esp_heap_caps.h>

// ==================== 메모리 검증 ====================
bool verifyMemory() {
  size_t freeHeap = ESP.getFreeHeap();
  size_t totalHeap = ESP.getHeapSize();
  size_t freePSRAM = ESP.getFreePsram();
  size_t totalPSRAM = ESP.getPsramSize();

  Serial.println("\n========== 메모리 검증 ==========");
  Serial.printf("Heap:  %d / %d bytes (%.1f%% 사용)\n", 
    totalHeap - freeHeap, totalHeap, 
    ((totalHeap - freeHeap) * 100.0) / totalHeap);
  Serial.printf("PSRAM: %d / %d bytes (%.1f%% 사용)\n", 
    totalPSRAM - freePSRAM, totalPSRAM, 
    ((totalPSRAM - freePSRAM) * 100.0) / totalPSRAM);

  // 임계값 체크 (80% 이상 사용 시 경고)
  if (((totalHeap - freeHeap) * 100 / totalHeap) > 80) {
    Serial.println("[경고] Heap 사용률 80% 초과");
    return false;
  }

  if (totalPSRAM > 0 && ((totalPSRAM - freePSRAM) * 100 / totalPSRAM) > 80) {
    Serial.println("[경고] PSRAM 사용률 80% 초과");
    return false;
  }

  Serial.println("=================================\n");
  return true;
}

// ==================== 메모리 누수 감지 ====================
void detectMemoryLeak() {
  static size_t lastFreeHeap = 0;
  static uint8_t consecutiveDecreases = 0;

  size_t currentFreeHeap = ESP.getFreeHeap();

  if (lastFreeHeap > 0) {
    if (currentFreeHeap < lastFreeHeap) {
      consecutiveDecreases++;
      if (consecutiveDecreases >= 10) {
        Serial.printf("[경고] 메모리 누수 의심: %d bytes 감소\n", 
          lastFreeHeap - currentFreeHeap);
        consecutiveDecreases = 0;
      }
    } else {
      consecutiveDecreases = 0;
    }
  }

  lastFreeHeap = currentFreeHeap;
}

// ==================== PSRAM 할당 ====================
void* allocatePSRAM(size_t size) {
  void* ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  if (ptr == NULL) {
    Serial.printf("[에러] PSRAM 할당 실패: %d bytes\n", size);
  } else {
    Serial.printf("[PSRAM] %d bytes 할당됨\n", size);
  }
  return ptr;
}

// ==================== 메모리 정보 출력 ====================
void printDetailedMemoryInfo() {
  Serial.println("\n========== 상세 메모리 정보 ==========");
  
  // Heap
  Serial.println("[Heap]");
  Serial.printf("  Free:  %d bytes\n", ESP.getFreeHeap());
  Serial.printf("  Total: %d bytes\n", ESP.getHeapSize());
  Serial.printf("  Min Free: %d bytes\n", ESP.getMinFreeHeap());
  Serial.printf("  Max Alloc: %d bytes\n", ESP.getMaxAllocHeap());

  // PSRAM
  if (ESP.getPsramSize() > 0) {
    Serial.println("[PSRAM]");
    Serial.printf("  Free:  %d bytes\n", ESP.getFreePsram());
    Serial.printf("  Total: %d bytes\n", ESP.getPsramSize());
    Serial.printf("  Min Free: %d bytes\n", ESP.getMinFreePsram());
    Serial.printf("  Max Alloc: %d bytes\n", ESP.getMaxAllocPsram());
  } else {
    Serial.println("[PSRAM] 없음");
  }

  // Flash
  Serial.println("[Flash]");
  Serial.printf("  Size: %d bytes\n", ESP.getFlashChipSize());
  Serial.printf("  Speed: %d Hz\n", ESP.getFlashChipSpeed());

  // Chip
  Serial.println("[Chip]");
  Serial.printf("  Model: %s\n", ESP.getChipModel());
  Serial.printf("  Cores: %d\n", ESP.getChipCores());
  Serial.printf("  Revision: %d\n", ESP.getChipRevision());
  Serial.printf("  Frequency: %d MHz\n", ESP.getCpuFreqMHz());

  Serial.println("======================================\n");
}


