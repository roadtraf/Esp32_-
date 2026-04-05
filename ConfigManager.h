// ================================================================
// ConfigManager.h - 설정 백업/복원 시스템 (Phase 3-1)
// ================================================================
#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <SPIFFS.h>

// ================================================================
// 설정 파일 경로
// ================================================================
#define CONFIG_PRIMARY_PATH     "/config/primary.dat"
#define CONFIG_BACKUP_PATH      "/config/backup.dat"
#define CONFIG_FACTORY_PATH     "/config/factory.dat"

// ================================================================
// CRC32 설정
// ================================================================
#define CRC32_POLYNOMIAL        0xEDB88320
#define CONFIG_MAGIC            0xC0F1614E  // "CONFIG" in hex

// ================================================================
// 설정 상태
// ================================================================
enum ConfigStatus {
    CONFIG_OK,                  // 정상
    CONFIG_CORRUPTED,           // 손상됨
    CONFIG_MISSING,             // 없음
    CONFIG_CRC_FAILED,          // 체크섬 실패
    CONFIG_VERSION_MISMATCH,    // 버전 불일치
    CONFIG_UNKNOWN_ERROR        // 알 수 없는 오류
};

// ================================================================
// 복원 소스
// ================================================================
enum RestoreSource {
    RESTORE_FROM_PRIMARY,       // 주 설정
    RESTORE_FROM_BACKUP,        // 백업 설정
    RESTORE_FROM_FACTORY,       // 공장 초기값
    RESTORE_FROM_DEFAULT        // 하드코딩된 기본값
};

// ================================================================
// 설정 헤더 (파일 시작 부분)
// ================================================================
struct ConfigHeader {
    uint32_t magic;             // 매직 넘버 (파일 식별)
    uint16_t version;           // 설정 버전
    uint16_t dataSize;          // 데이터 크기
    uint32_t crc32;             // CRC32 체크섬
    uint32_t timestamp;         // 저장 시각
};

// ================================================================
// 설정 통계
// ================================================================
struct ConfigStats {
    uint32_t saveCount;         // 저장 횟수
    uint32_t loadCount;         // 로드 횟수
    uint32_t backupCount;       // 백업 횟수
    uint32_t restoreCount;      // 복원 횟수
    uint32_t corruptionCount;   // 손상 감지 횟수
    uint32_t lastSaveTime;      // 마지막 저장 시각
    uint32_t lastBackupTime;    // 마지막 백업 시각
};

// ================================================================
// 설정 관리자 클래스
// ================================================================
class ConfigManager {
public:
    // ── 초기화 ──
    bool begin();
    
    // ── 저장/로드 ──
    bool saveConfig(const void* data, size_t size, bool createBackup = true);
    ConfigStatus loadConfig(void* data, size_t size);
    
    // ── 백업/복원 ──
    bool createBackup();
    ConfigStatus restoreFromBackup(void* data, size_t size);
    ConfigStatus restoreFromFactory(void* data, size_t size);
    
    // ── 무결성 검증 ──
    ConfigStatus verifyConfig(const char* path);
    bool verifyIntegrity(const void* data, size_t size, uint32_t expectedCRC);
    
    // ── CRC32 계산 ──
    uint32_t calculateCRC32(const void* data, size_t size);
    
    // ── 파일 관리 ──
    bool fileExists(const char* path);
    size_t getFileSize(const char* path);
    bool deleteFile(const char* path);
    
    // ── 공장 초기화 ──
    bool saveFactoryDefaults(const void* data, size_t size);
    bool hasFactoryDefaults();
    
    // ── 상태 조회 ──
    ConfigStatus getPrimaryStatus();
    ConfigStatus getBackupStatus();
    ConfigStats getStats();
    
    // ── 자동 백업 ──
    void enableAutoBackup(uint32_t intervalMinutes = 60);
    void disableAutoBackup();
    void checkAutoBackup();
    
    // ── 진단 ──
    void printStatus();
    void printFileInfo(const char* path);
    void printStats();

private:
    ConfigStats stats;
    bool autoBackupEnabled;
    uint32_t autoBackupInterval;
    uint32_t lastAutoBackup;
    
    // ── 내부 메서드 ──
    bool writeConfigFile(const char* path, const void* data, size_t size);
    ConfigStatus readConfigFile(const char* path, void* data, size_t size);
    
    bool writeHeader(File& file, uint16_t dataSize, uint32_t crc32);
    bool readHeader(File& file, ConfigHeader& header);
    bool validateHeader(const ConfigHeader& header, size_t expectedSize);
    
    void updateStats(const char* operation);
    void ensureDirectoryExists();
};

// ================================================================
// 전역 인스턴스
// ================================================================
extern ConfigManager configManager;

// ================================================================
// 편의 매크로
// ================================================================
#define CONFIG_SAVE(data) configManager.saveConfig(&data, sizeof(data), true)
#define CONFIG_LOAD(data) configManager.loadConfig(&data, sizeof(data))
#define CONFIG_VERIFY() configManager.verifyConfig(CONFIG_PRIMARY_PATH)
