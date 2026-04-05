// ================================================================
// ConfigManager.cpp - 설정 백업/복원 시스템 구현
// ================================================================
#include "ConfigManager.h"

// 전역 인스턴스
ConfigManager configManager;

// CRC32 테이블 (계산 속도 향상)
static uint32_t crc32Table[256];
static bool crc32TableInitialized = false;

// ================================================================
// CRC32 테이블 초기화
// ================================================================
static void initCRC32Table() {
    if (crc32TableInitialized) return;
    
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (uint32_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
            } else {
                crc >>= 1;
            }
        }
        crc32Table[i] = crc;
    }
    
    crc32TableInitialized = true;
}

// ================================================================
// 초기화
// ================================================================
bool ConfigManager::begin() {
    Serial.println("[ConfigMgr] 초기화 시작...");
    
    // CRC32 테이블 초기화
    initCRC32Table();
    
    // 통계 초기화
    memset(&stats, 0, sizeof(stats));
    
    // 자동 백업 비활성화 (기본값)
    autoBackupEnabled = false;
    autoBackupInterval = 0;
    lastAutoBackup = 0;
    
    // SPIFFS 확인
    if (!SPIFFS.begin(true)) {
        Serial.println("[ConfigMgr] ❌ SPIFFS 마운트 실패");
        return false;
    }
    
    // 설정 디렉토리 확인/생성
    ensureDirectoryExists();
    
    // 주 설정 파일 상태 확인
    ConfigStatus status = verifyConfig(CONFIG_PRIMARY_PATH);
    
    Serial.printf("[ConfigMgr] 주 설정 상태: ");
    switch (status) {
        case CONFIG_OK:
            Serial.println("✅ 정상");
            break;
        case CONFIG_MISSING:
            Serial.println("⚠️  없음 (초기 부팅)");
            break;
        case CONFIG_CORRUPTED:
            Serial.println("❌ 손상됨");
            break;
        case CONFIG_CRC_FAILED:
            Serial.println("❌ CRC 실패");
            break;
        default:
            Serial.println("❔ 알 수 없음");
            break;
    }
    
    // 백업 파일 상태 확인
    if (fileExists(CONFIG_BACKUP_PATH)) {
        ConfigStatus backupStatus = verifyConfig(CONFIG_BACKUP_PATH);
        Serial.printf("[ConfigMgr] 백업 설정 상태: ");
        switch (backupStatus) {
            case CONFIG_OK:
                Serial.println("✅ 정상");
                break;
            default:
                Serial.println("❌ 손상됨");
                break;
        }
    }
    
    Serial.println("[ConfigMgr] ✅ 초기화 완료");
    return true;
}

// ================================================================
// 설정 저장
// ================================================================
bool ConfigManager::saveConfig(const void* data, size_t size, bool createBackupFlag) {
    if (!data || size == 0) {
        Serial.println("[ConfigMgr] ❌ 잘못된 데이터");
        return false;
    }
    
    // 백업 생성 (옵션)
    if (createBackupFlag && fileExists(CONFIG_PRIMARY_PATH)) {
        createBackup();
    }
    
    // 주 설정 파일 저장
    bool success = writeConfigFile(CONFIG_PRIMARY_PATH, data, size);
    
    if (success) {
        stats.saveCount++;
        stats.lastSaveTime = millis() / 1000;
        Serial.println("[ConfigMgr] ✅ 설정 저장 완료");
    } else {
        Serial.println("[ConfigMgr] ❌ 설정 저장 실패");
    }
    
    return success;
}

// ================================================================
// 설정 로드
// ================================================================
ConfigStatus ConfigManager::loadConfig(void* data, size_t size) {
    if (!data || size == 0) {
        Serial.println("[ConfigMgr] ❌ 잘못된 버퍼");
        return CONFIG_UNKNOWN_ERROR;
    }
    
    // 주 설정 파일에서 로드 시도
    ConfigStatus status = readConfigFile(CONFIG_PRIMARY_PATH, data, size);
    
    if (status == CONFIG_OK) {
        stats.loadCount++;
        Serial.println("[ConfigMgr] ✅ 주 설정 로드 완료");
        return CONFIG_OK;
    }
    
    // 주 설정 실패 시 백업에서 복원 시도
    Serial.println("[ConfigMgr] ⚠️  주 설정 로드 실패, 백업에서 복원 시도...");
    stats.corruptionCount++;
    
    status = restoreFromBackup(data, size);
    if (status == CONFIG_OK) {
        // 백업을 주 설정으로 복사
        saveConfig(data, size, false);
        return CONFIG_OK;
    }
    
    // 백업도 실패 시 공장 초기값 시도
    Serial.println("[ConfigMgr] ⚠️  백업도 실패, 공장 초기값 시도...");
    status = restoreFromFactory(data, size);
    
    return status;
}

// ================================================================
// 백업 생성
// ================================================================
bool ConfigManager::createBackup() {
    if (!fileExists(CONFIG_PRIMARY_PATH)) {
        Serial.println("[ConfigMgr] ⚠️  주 설정 파일 없음, 백업 생략");
        return false;
    }
    
    // 주 설정 파일을 백업으로 복사
    File src = SPIFFS.open(CONFIG_PRIMARY_PATH, FILE_READ);
    if (!src) {
        Serial.println("[ConfigMgr] ❌ 주 설정 파일 열기 실패");
        return false;
    }
    
    File dst = SPIFFS.open(CONFIG_BACKUP_PATH, FILE_WRITE);
    if (!dst) {
        src.close();
        Serial.println("[ConfigMgr] ❌ 백업 파일 생성 실패");
        return false;
    }
    
    // 파일 복사
    size_t size = src.size();
    uint8_t buffer[256];
    size_t totalCopied = 0;
    
    while (src.available()) {
        size_t read = src.read(buffer, sizeof(buffer));
        dst.write(buffer, read);
        totalCopied += read;
    }
    
    src.close();
    dst.close();
    
    if (totalCopied == size) {
        stats.backupCount++;
        stats.lastBackupTime = millis() / 1000;
        Serial.printf("[ConfigMgr] ✅ 백업 생성 완료 (%d bytes)\n", totalCopied);
        return true;
    } else {
        Serial.println("[ConfigMgr] ❌ 백업 복사 불완전");
        return false;
    }
}

// ================================================================
// 백업에서 복원
// ================================================================
ConfigStatus ConfigManager::restoreFromBackup(void* data, size_t size) {
    if (!fileExists(CONFIG_BACKUP_PATH)) {
        Serial.println("[ConfigMgr] ⚠️  백업 파일 없음");
        return CONFIG_MISSING;
    }
    
    ConfigStatus status = readConfigFile(CONFIG_BACKUP_PATH, data, size);
    
    if (status == CONFIG_OK) {
        stats.restoreCount++;
        Serial.println("[ConfigMgr] ✅ 백업에서 복원 완료");
    } else {
        Serial.println("[ConfigMgr] ❌ 백업 복원 실패");
    }
    
    return status;
}

// ================================================================
// 공장 초기값 복원
// ================================================================
ConfigStatus ConfigManager::restoreFromFactory(void* data, size_t size) {
    if (!hasFactoryDefaults()) {
        Serial.println("[ConfigMgr] ⚠️  공장 초기값 없음");
        return CONFIG_MISSING;
    }
    
    ConfigStatus status = readConfigFile(CONFIG_FACTORY_PATH, data, size);
    
    if (status == CONFIG_OK) {
        stats.restoreCount++;
        Serial.println("[ConfigMgr] ✅ 공장 초기값 복원 완료");
    } else {
        Serial.println("[ConfigMgr] ❌ 공장 초기값 복원 실패");
    }
    
    return status;
}

// ================================================================
// 설정 검증
// ================================================================
ConfigStatus ConfigManager::verifyConfig(const char* path) {
    if (!fileExists(path)) {
        return CONFIG_MISSING;
    }
    
    File file = SPIFFS.open(path, FILE_READ);
    if (!file) {
        return CONFIG_UNKNOWN_ERROR;
    }
    
    // 헤더 읽기
    ConfigHeader header;
    if (!readHeader(file, header)) {
        file.close();
        return CONFIG_CORRUPTED;
    }
    
    // 매직 넘버 확인
    if (header.magic != CONFIG_MAGIC) {
        file.close();
        return CONFIG_CORRUPTED;
    }
    
    // 데이터 읽기 및 CRC 검증
    uint8_t* buffer = new uint8_t[header.dataSize];
    if (!buffer) {
        file.close();
        return CONFIG_UNKNOWN_ERROR;
    }
    
    size_t read = file.read(buffer, header.dataSize);
    file.close();
    
    if (read != header.dataSize) {
        delete[] buffer;
        return CONFIG_CORRUPTED;
    }
    
    // CRC32 계산
    uint32_t calculatedCRC = calculateCRC32(buffer, header.dataSize);
    delete[] buffer;
    
    if (calculatedCRC != header.crc32) {
        return CONFIG_CRC_FAILED;
    }
    
    return CONFIG_OK;
}

// ================================================================
// 무결성 검증 (메모리 데이터)
// ================================================================
bool ConfigManager::verifyIntegrity(const void* data, size_t size, uint32_t expectedCRC) {
    uint32_t calculatedCRC = calculateCRC32(data, size);
    return (calculatedCRC == expectedCRC);
}

// ================================================================
// CRC32 계산
// ================================================================
uint32_t ConfigManager::calculateCRC32(const void* data, size_t size) {
    const uint8_t* buffer = (const uint8_t*)data;
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < size; i++) {
        uint8_t index = (crc ^ buffer[i]) & 0xFF;
        crc = (crc >> 8) ^ crc32Table[index];
    }
    
    return ~crc;
}

// ================================================================
// 파일 관리
// ================================================================
bool ConfigManager::fileExists(const char* path) {
    return SPIFFS.exists(path);
}

size_t ConfigManager::getFileSize(const char* path) {
    if (!fileExists(path)) return 0;
    
    File file = SPIFFS.open(path, FILE_READ);
    if (!file) return 0;
    
    size_t size = file.size();
    file.close();
    return size;
}

bool ConfigManager::deleteFile(const char* path) {
    if (!fileExists(path)) return false;
    return SPIFFS.remove(path);
}

// ================================================================
// 공장 초기값
// ================================================================
bool ConfigManager::saveFactoryDefaults(const void* data, size_t size) {
    bool success = writeConfigFile(CONFIG_FACTORY_PATH, data, size);
    
    if (success) {
        Serial.println("[ConfigMgr] ✅ 공장 초기값 저장 완료");
    } else {
        Serial.println("[ConfigMgr] ❌ 공장 초기값 저장 실패");
    }
    
    return success;
}

bool ConfigManager::hasFactoryDefaults() {
    return (verifyConfig(CONFIG_FACTORY_PATH) == CONFIG_OK);
}

// ================================================================
// 상태 조회
// ================================================================
ConfigStatus ConfigManager::getPrimaryStatus() {
    return verifyConfig(CONFIG_PRIMARY_PATH);
}

ConfigStatus ConfigManager::getBackupStatus() {
    return verifyConfig(CONFIG_BACKUP_PATH);
}

ConfigStats ConfigManager::getStats() {
    return stats;
}

// ================================================================
// 자동 백업
// ================================================================
void ConfigManager::enableAutoBackup(uint32_t intervalMinutes) {
    autoBackupEnabled = true;
    autoBackupInterval = intervalMinutes * 60000;  // 분 → 밀리초
    lastAutoBackup = millis();
    
    Serial.printf("[ConfigMgr] 자동 백업 활성화 (간격: %lu분)\n", intervalMinutes);
}

void ConfigManager::disableAutoBackup() {
    autoBackupEnabled = false;
    Serial.println("[ConfigMgr] 자동 백업 비활성화");
}

void ConfigManager::checkAutoBackup() {
    if (!autoBackupEnabled) return;
    
    uint32_t now = millis();
    if (now - lastAutoBackup >= autoBackupInterval) {
        Serial.println("[ConfigMgr] 자동 백업 수행...");
        createBackup();
        lastAutoBackup = now;
    }
}

// ================================================================
// 진단 출력
// ================================================================
void ConfigManager::printStatus() {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║       설정 관리자 상태                ║");
    Serial.println("╠═══════════════════════════════════════╣");
    
    // 주 설정
    ConfigStatus primary = getPrimaryStatus();
    Serial.printf("║ 주 설정: ");
    switch (primary) {
        case CONFIG_OK:         Serial.println("✅ 정상                    ║"); break;
        case CONFIG_MISSING:    Serial.println("⚠️  없음                    ║"); break;
        case CONFIG_CORRUPTED:  Serial.println("❌ 손상됨                  ║"); break;
        case CONFIG_CRC_FAILED: Serial.println("❌ CRC 실패                ║"); break;
        default:                Serial.println("❔ 알 수 없음              ║"); break;
    }
    
    // 백업
    ConfigStatus backup = getBackupStatus();
    Serial.printf("║ 백업: ");
    switch (backup) {
        case CONFIG_OK:         Serial.println("✅ 정상                    ║"); break;
        case CONFIG_MISSING:    Serial.println("⚠️  없음                    ║"); break;
        default:                Serial.println("❌ 손상됨                  ║"); break;
    }
    
    // 공장 초기값
    Serial.printf("║ 공장 초기값: %-24s ║\n", 
                  hasFactoryDefaults() ? "✅ 있음" : "⚠️  없음");
    
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ 자동 백업: %-26s ║\n", 
                  autoBackupEnabled ? "활성화" : "비활성화");
    
    Serial.println("╚═══════════════════════════════════════╝\n");
}

void ConfigManager::printFileInfo(const char* path) {
    if (!fileExists(path)) {
        Serial.printf("[ConfigMgr] 파일 없음: %s\n", path);
        return;
    }
    
    File file = SPIFFS.open(path, FILE_READ);
    if (!file) return;
    
    ConfigHeader header;
    readHeader(file, header);
    file.close();
    
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.printf("║ 파일: %-31s ║\n", path);
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ 크기: %lu bytes                       ║\n", getFileSize(path));
    Serial.printf("║ 데이터 크기: %u bytes                 ║\n", header.dataSize);
    Serial.printf("║ CRC32: 0x%08lX                        ║\n", header.crc32);
    Serial.printf("║ 타임스탬프: %lu                       ║\n", header.timestamp);
    Serial.println("╚═══════════════════════════════════════╝\n");
}

void ConfigManager::printStats() {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║       설정 관리자 통계                ║");
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ 저장 횟수: %lu                        ║\n", stats.saveCount);
    Serial.printf("║ 로드 횟수: %lu                        ║\n", stats.loadCount);
    Serial.printf("║ 백업 횟수: %lu                        ║\n", stats.backupCount);
    Serial.printf("║ 복원 횟수: %lu                        ║\n", stats.restoreCount);
    Serial.printf("║ 손상 감지: %lu                        ║\n", stats.corruptionCount);
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ 마지막 저장: %lu초 전                 ║\n", 
                  (millis() / 1000) - stats.lastSaveTime);
    Serial.printf("║ 마지막 백업: %lu초 전                 ║\n", 
                  (millis() / 1000) - stats.lastBackupTime);
    Serial.println("╚═══════════════════════════════════════╝\n");
}

// ================================================================
// 내부 메서드
// ================================================================
bool ConfigManager::writeConfigFile(const char* path, const void* data, size_t size) {
    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file) {
        Serial.printf("[ConfigMgr] ❌ 파일 생성 실패: %s\n", path);
        return false;
    }
    
    // CRC32 계산
    uint32_t crc = calculateCRC32(data, size);
    
    // 헤더 쓰기
    if (!writeHeader(file, size, crc)) {
        file.close();
        return false;
    }
    
    // 데이터 쓰기
    size_t written = file.write((const uint8_t*)data, size);
    file.close();
    
    return (written == size);
}

ConfigStatus ConfigManager::readConfigFile(const char* path, void* data, size_t size) {
    if (!fileExists(path)) {
        return CONFIG_MISSING;
    }
    
    File file = SPIFFS.open(path, FILE_READ);
    if (!file) {
        return CONFIG_UNKNOWN_ERROR;
    }
    
    // 헤더 읽기
    ConfigHeader header;
    if (!readHeader(file, header)) {
        file.close();
        return CONFIG_CORRUPTED;
    }
    
    // 헤더 검증
    if (!validateHeader(header, size)) {
        file.close();
        return CONFIG_CORRUPTED;
    }
    
    // 데이터 읽기
    size_t read = file.read((uint8_t*)data, size);
    file.close();
    
    if (read != size) {
        return CONFIG_CORRUPTED;
    }
    
    // CRC 검증
    if (!verifyIntegrity(data, size, header.crc32)) {
        return CONFIG_CRC_FAILED;
    }
    
    return CONFIG_OK;
}

bool ConfigManager::writeHeader(File& file, uint16_t dataSize, uint32_t crc32) {
    ConfigHeader header;
    header.magic = CONFIG_MAGIC;
    header.version = 1;  // 설정 버전
    header.dataSize = dataSize;
    header.crc32 = crc32;
    header.timestamp = millis() / 1000;
    
    size_t written = file.write((const uint8_t*)&header, sizeof(header));
    return (written == sizeof(header));
}

bool ConfigManager::readHeader(File& file, ConfigHeader& header) {
    size_t read = file.read((uint8_t*)&header, sizeof(header));
    return (read == sizeof(header));
}

bool ConfigManager::validateHeader(const ConfigHeader& header, size_t expectedSize) {
    if (header.magic != CONFIG_MAGIC) return false;
    if (header.dataSize != expectedSize) return false;
    return true;
}

void ConfigManager::ensureDirectoryExists() {
    // SPIFFS는 디렉토리를 자동 생성하므로 체크만
    if (!SPIFFS.exists("/config")) {
        Serial.println("[ConfigMgr] /config 디렉토리 생성됨");
    }
}
