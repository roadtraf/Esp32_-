// ================================================================
// ConfigManager.cpp -  /  
// ================================================================
#include "ConfigManager.h"

//  
ConfigManager configManager;

// CRC32  (  )
static uint32_t crc32Table[256];
static bool crc32TableInitialized = false;

// ================================================================
// CRC32  
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
// 
// ================================================================
bool ConfigManager::begin() {
    Serial.println("[ConfigMgr]  ...");
    
    // CRC32  
    initCRC32Table();
    
    //  
    memset(&stats, 0, sizeof(stats));
    
    //    ()
    autoBackupEnabled = false;
    autoBackupInterval = 0;
    lastAutoBackup = 0;
    
    // SPIFFS 
    if (!SPIFFS.begin(true)) {
        Serial.println("[ConfigMgr]  SPIFFS  ");
        return false;
    }
    
    //   /
    ensureDirectoryExists();
    
    //     
    ConfigStatus status = verifyConfig(CONFIG_PRIMARY_PATH);
    
    Serial.printf("[ConfigMgr]   : ");
    switch (status) {
        case CONFIG_OK:
            Serial.println(" ");
            break;
        case CONFIG_MISSING:
            Serial.println("   ( )");
            break;
        case CONFIG_CORRUPTED:
            Serial.println(" ");
            break;
        case CONFIG_CRC_FAILED:
            Serial.println(" CRC ");
            break;
        default:
            Serial.println("   ");
            break;
    }
    
    //    
    if (fileExists(CONFIG_BACKUP_PATH)) {
        ConfigStatus backupStatus = verifyConfig(CONFIG_BACKUP_PATH);
        Serial.printf("[ConfigMgr]   : ");
        switch (backupStatus) {
            case CONFIG_OK:
                Serial.println(" ");
                break;
            default:
                Serial.println(" ");
                break;
        }
    }
    
    Serial.println("[ConfigMgr]   ");
    return true;
}

// ================================================================
//  
// ================================================================
bool ConfigManager::saveConfig(const void* data, size_t size, bool createBackupFlag) {
    if (!data || size == 0) {
        Serial.println("[ConfigMgr]   ");
        return false;
    }
    
    //   ()
    if (createBackupFlag && fileExists(CONFIG_PRIMARY_PATH)) {
        createBackup();
    }
    
    //    
    bool success = writeConfigFile(CONFIG_PRIMARY_PATH, data, size);
    
    if (success) {
        stats.saveCount++;
        stats.lastSaveTime = millis() / 1000;
        Serial.println("[ConfigMgr]    ");
    } else {
        Serial.println("[ConfigMgr]    ");
    }
    
    return success;
}

// ================================================================
//  
// ================================================================
ConfigStatus ConfigManager::loadConfig(void* data, size_t size) {
    if (!data || size == 0) {
        Serial.println("[ConfigMgr]   ");
        return CONFIG_UNKNOWN_ERROR;
    }
    
    //     
    ConfigStatus status = readConfigFile(CONFIG_PRIMARY_PATH, data, size);
    
    if (status == CONFIG_OK) {
        stats.loadCount++;
        Serial.println("[ConfigMgr]     ");
        return CONFIG_OK;
    }
    
    //       
    Serial.println("[ConfigMgr]      ,   ...");
    stats.corruptionCount++;
    
    status = restoreFromBackup(data, size);
    if (status == CONFIG_OK) {
        //    
        saveConfig(data, size, false);
        return CONFIG_OK;
    }
    
    //      
    Serial.println("[ConfigMgr]    ,   ...");
    status = restoreFromFactory(data, size);
    
    return status;
}

// ================================================================
//  
// ================================================================
bool ConfigManager::createBackup() {
    if (!fileExists(CONFIG_PRIMARY_PATH)) {
        Serial.println("[ConfigMgr]      ,  ");
        return false;
    }
    
    //     
    File src = SPIFFS.open(CONFIG_PRIMARY_PATH, FILE_READ);
    if (!src) {
        Serial.println("[ConfigMgr]      ");
        return false;
    }
    
    File dst = SPIFFS.open(CONFIG_BACKUP_PATH, FILE_WRITE);
    if (!dst) {
        src.close();
        Serial.println("[ConfigMgr]     ");
        return false;
    }
    
    //  
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
        Serial.printf("[ConfigMgr]     (%d bytes)\n", totalCopied);
        return true;
    } else {
        Serial.println("[ConfigMgr]    ");
        return false;
    }
}

// ================================================================
//  
// ================================================================
ConfigStatus ConfigManager::restoreFromBackup(void* data, size_t size) {
    if (!fileExists(CONFIG_BACKUP_PATH)) {
        Serial.println("[ConfigMgr]     ");
        return CONFIG_MISSING;
    }
    
    ConfigStatus status = readConfigFile(CONFIG_BACKUP_PATH, data, size);
    
    if (status == CONFIG_OK) {
        stats.restoreCount++;
        Serial.println("[ConfigMgr]    ");
    } else {
        Serial.println("[ConfigMgr]    ");
    }
    
    return status;
}

// ================================================================
//   
// ================================================================
ConfigStatus ConfigManager::restoreFromFactory(void* data, size_t size) {
    if (!hasFactoryDefaults()) {
        Serial.println("[ConfigMgr]     ");
        return CONFIG_MISSING;
    }
    
    ConfigStatus status = readConfigFile(CONFIG_FACTORY_PATH, data, size);
    
    if (status == CONFIG_OK) {
        stats.restoreCount++;
        Serial.println("[ConfigMgr]     ");
    } else {
        Serial.println("[ConfigMgr]     ");
    }
    
    return status;
}

// ================================================================
//  
// ================================================================
ConfigStatus ConfigManager::verifyConfig(const char* path) {
    if (!fileExists(path)) {
        return CONFIG_MISSING;
    }
    
    File file = SPIFFS.open(path, FILE_READ);
    if (!file) {
        return CONFIG_UNKNOWN_ERROR;
    }
    
    //  
    ConfigHeader header;
    if (!readHeader(file, header)) {
        file.close();
        return CONFIG_CORRUPTED;
    }
    
    //   
    if (header.magic != CONFIG_MAGIC) {
        file.close();
        return CONFIG_CORRUPTED;
    }
    
    //    CRC 
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
    
    // CRC32 
    uint32_t calculatedCRC = calculateCRC32(buffer, header.dataSize);
    delete[] buffer;
    
    if (calculatedCRC != header.crc32) {
        return CONFIG_CRC_FAILED;
    }
    
    return CONFIG_OK;
}

// ================================================================
//   ( )
// ================================================================
bool ConfigManager::verifyIntegrity(const void* data, size_t size, uint32_t expectedCRC) {
    uint32_t calculatedCRC = calculateCRC32(data, size);
    return (calculatedCRC == expectedCRC);
}

// ================================================================
// CRC32 
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
//  
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
//  
// ================================================================
bool ConfigManager::saveFactoryDefaults(const void* data, size_t size) {
    bool success = writeConfigFile(CONFIG_FACTORY_PATH, data, size);
    
    if (success) {
        Serial.println("[ConfigMgr]     ");
    } else {
        Serial.println("[ConfigMgr]     ");
    }
    
    return success;
}

bool ConfigManager::hasFactoryDefaults() {
    return (verifyConfig(CONFIG_FACTORY_PATH) == CONFIG_OK);
}

// ================================================================
//  
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
//  
// ================================================================
void ConfigManager::enableAutoBackup(uint32_t intervalMinutes) {
    autoBackupEnabled = true;
    autoBackupInterval = intervalMinutes * 60000;  //   
    lastAutoBackup = millis();
    
    Serial.printf("[ConfigMgr]    (: %lu)\n", intervalMinutes);
}

void ConfigManager::disableAutoBackup() {
    autoBackupEnabled = false;
    Serial.println("[ConfigMgr]   ");
}

void ConfigManager::checkAutoBackup() {
    if (!autoBackupEnabled) return;
    
    uint32_t now = millis();
    if (now - lastAutoBackup >= autoBackupInterval) {
        Serial.println("[ConfigMgr]   ...");
        createBackup();
        lastAutoBackup = now;
    }
}

// ================================================================
//  
// ================================================================
void ConfigManager::printStatus() {
    Serial.println("\n");
    Serial.println("                         ");
    Serial.println("");
    
    //  
    ConfigStatus primary = getPrimaryStatus();
    Serial.printf("  : ");
    switch (primary) {
        case CONFIG_OK:         Serial.println("                     "); break;
        case CONFIG_MISSING:    Serial.println("                      "); break;
        case CONFIG_CORRUPTED:  Serial.println("                   "); break;
        case CONFIG_CRC_FAILED: Serial.println(" CRC                 "); break;
        default:                Serial.println("                 "); break;
    }
    
    // 
    ConfigStatus backup = getBackupStatus();
    Serial.printf(" : ");
    switch (backup) {
        case CONFIG_OK:         Serial.println("                     "); break;
        case CONFIG_MISSING:    Serial.println("                      "); break;
        default:                Serial.println("                   "); break;
    }
    
    //  
    Serial.printf("  : %-24s \n", 
                  hasFactoryDefaults() ? " " : "  ");
    
    Serial.println("");
    Serial.printf("  : %-26s \n", 
                  autoBackupEnabled ? "" : "");
    
    Serial.println("\n");
}

void ConfigManager::printFileInfo(const char* path) {
    if (!fileExists(path)) {
        Serial.printf("[ConfigMgr]  : %s\n", path);
        return;
    }
    
    File file = SPIFFS.open(path, FILE_READ);
    if (!file) return;
    
    ConfigHeader header;
    readHeader(file, header);
    file.close();
    
    Serial.println("\n");
    Serial.printf(" : %-31s \n", path);
    Serial.println("");
    Serial.printf(" : %lu bytes                       \n", getFileSize(path));
    Serial.printf("  : %u bytes                 \n", header.dataSize);
    Serial.printf(" CRC32: 0x%08lX                        \n", header.crc32);
    Serial.printf(" : %lu                       \n", header.timestamp);
    Serial.println("\n");
}

void ConfigManager::printStats() {
    Serial.println("\n");
    Serial.println("                         ");
    Serial.println("");
    Serial.printf("  : %lu                        \n", stats.saveCount);
    Serial.printf("  : %lu                        \n", stats.loadCount);
    Serial.printf("  : %lu                        \n", stats.backupCount);
    Serial.printf("  : %lu                        \n", stats.restoreCount);
    Serial.printf("  : %lu                        \n", stats.corruptionCount);
    Serial.println("");
    Serial.printf("  : %lu                  \n", 
                  (millis() / 1000) - stats.lastSaveTime);
    Serial.printf("  : %lu                  \n", 
                  (millis() / 1000) - stats.lastBackupTime);
    Serial.println("\n");
}

// ================================================================
//  
// ================================================================
bool ConfigManager::writeConfigFile(const char* path, const void* data, size_t size) {
    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file) {
        Serial.printf("[ConfigMgr]    : %s\n", path);
        return false;
    }
    
    // CRC32 
    uint32_t crc = calculateCRC32(data, size);
    
    //  
    if (!writeHeader(file, size, crc)) {
        file.close();
        return false;
    }
    
    //  
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
    
    //  
    ConfigHeader header;
    if (!readHeader(file, header)) {
        file.close();
        return CONFIG_CORRUPTED;
    }
    
    //  
    if (!validateHeader(header, size)) {
        file.close();
        return CONFIG_CORRUPTED;
    }
    
    //  
    size_t read = file.read((uint8_t*)data, size);
    file.close();
    
    if (read != size) {
        return CONFIG_CORRUPTED;
    }
    
    // CRC 
    if (!verifyIntegrity(data, size, header.crc32)) {
        return CONFIG_CRC_FAILED;
    }
    
    return CONFIG_OK;
}

bool ConfigManager::writeHeader(File& file, uint16_t dataSize, uint32_t crc32) {
    ConfigHeader header;
    header.magic = CONFIG_MAGIC;
    header.version = 1;  //  
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
    // SPIFFS    
    if (!SPIFFS.exists("/config")) {
        Serial.println("[ConfigMgr] /config  ");
    }
}
