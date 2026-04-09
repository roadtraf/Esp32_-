
#include "Config.h"                 // Arduino.h 
#include <esp_heap_caps.h>

// ====================   ====================
bool verifyMemory() {
  size_t freeHeap = ESP.getFreeHeap();
  size_t totalHeap = ESP.getHeapSize();
  size_t freePSRAM = ESP.getFreePsram();
  size_t totalPSRAM = ESP.getPsramSize();

  Serial.println("\n==========   ==========");
  Serial.printf("Heap:  %d / %d bytes (%.1f%% )\n", 
    totalHeap - freeHeap, totalHeap, 
    ((totalHeap - freeHeap) * 100.0) / totalHeap);
  Serial.printf("PSRAM: %d / %d bytes (%.1f%% )\n", 
    totalPSRAM - freePSRAM, totalPSRAM, 
    ((totalPSRAM - freePSRAM) * 100.0) / totalPSRAM);

  //   (80%    )
  if (((totalHeap - freeHeap) * 100 / totalHeap) > 80) {
    Serial.println("[] Heap  80% ");
    return false;
  }

  if (totalPSRAM > 0 && ((totalPSRAM - freePSRAM) * 100 / totalPSRAM) > 80) {
    Serial.println("[] PSRAM  80% ");
    return false;
  }

  Serial.println("=================================\n");
  return true;
}

// ====================    ====================
void detectMemoryLeak() {
  static size_t lastFreeHeap = 0;
  static uint8_t consecutiveDecreases = 0;

  size_t currentFreeHeap = ESP.getFreeHeap();

  if (lastFreeHeap > 0) {
    if (currentFreeHeap < lastFreeHeap) {
      consecutiveDecreases++;
      if (consecutiveDecreases >= 10) {
        Serial.printf("[]   : %d bytes \n", 
          lastFreeHeap - currentFreeHeap);
        consecutiveDecreases = 0;
      }
    } else {
      consecutiveDecreases = 0;
    }
  }

  lastFreeHeap = currentFreeHeap;
}

// ==================== PSRAM  ====================
void* allocatePSRAM(size_t size) {
  void* ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  if (ptr == NULL) {
    Serial.printf("[] PSRAM  : %d bytes\n", size);
  } else {
    Serial.printf("[PSRAM] %d bytes \n", size);
  }
  return ptr;
}

// ====================    ====================
void printDetailedMemoryInfo() {
  Serial.println("\n==========    ==========");
  
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
    Serial.println("[PSRAM] ");
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


