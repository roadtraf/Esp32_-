// ================================================================
// ConfigManager.h -  /  (Phase 3-1)
// ================================================================
#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <SPIFFS.h>

// ================================================================
//   
// ================================================================
#define CONFIG_PRIMARY_PATH     "/config/primary.dat"
#define CONFIG_BACKUP_PATH      "/config/backup.dat"
#define CONFIG_FACTORY_PATH     "/config/factory.dat"

// ================================================================
// CRC32 
// ================================================================
#define CRC32_POLYNOMIAL        0xEDB88320
#define CONFIG_MAGIC            0xC0F1614E  // "CONFIG" in hex

// ================================================================
//  
// ================================================================
enum ConfigStatus {
    CONFIG_OK,                  // 
    CONFIG_CORRUPTED,           // 
    CONFIG_MISSING,             // 
    CONFIG_CRC_FAILED,          //  
    CONFIG_VERSION_MISMATCH,    //  
    CONFIG_UNKNOWN_ERROR        //    
};

// ================================================================
//  
// ================================================================
enum RestoreSource {
    RESTORE_FROM_PRIMARY,       //  
    RESTORE_FROM_BACKUP,        //  
    RESTORE_FROM_FACTORY,       //  
    RESTORE_FROM_DEFAULT        //  
};

// ================================================================
//   (  )
// ================================================================
struct ConfigHeader {
    uint32_t magic;             //   ( )
    uint16_t version;           //  
    uint16_t dataSize;          //  
    uint32_t crc32;             // CRC32 
    uint32_t timestamp;         //  
};

// ================================================================
//  
// ================================================================
struct ConfigStats {
    uint32_t saveCount;         //  
    uint32_t loadCount;         //  
    uint32_t backupCount;       //  
    uint32_t restoreCount;      //  
    uint32_t corruptionCount;   //   
    uint32_t lastSaveTime;      //   
    uint32_t lastBackupTime;    //   
};

// ================================================================
//   
// ================================================================
class ConfigManager {
public:
    //   
    bool begin();
    
    //  / 
    bool saveConfig(const void* data, size_t size, bool createBackup = true);
    ConfigStatus loadConfig(void* data, size_t size);
    
    //  / 
    bool createBackup();
    ConfigStatus restoreFromBackup(void* data, size_t size);
    ConfigStatus restoreFromFactory(void* data, size_t size);
    
    //    
    ConfigStatus verifyConfig(const char* path);
    bool verifyIntegrity(const void* data, size_t size, uint32_t expectedCRC);
    
    //  CRC32  
    uint32_t calculateCRC32(const void* data, size_t size);
    
    //    
    bool fileExists(const char* path);
    size_t getFileSize(const char* path);
    bool deleteFile(const char* path);
    
    //    
    bool saveFactoryDefaults(const void* data, size_t size);
    bool hasFactoryDefaults();
    
    //    
    ConfigStatus getPrimaryStatus();
    ConfigStatus getBackupStatus();
    ConfigStats getStats();
    
    //    
    void enableAutoBackup(uint32_t intervalMinutes = 60);
    void disableAutoBackup();
    void checkAutoBackup();
    
    //   
    void printStatus();
    void printFileInfo(const char* path);
    void printStats();

private:
    ConfigStats stats;
    bool autoBackupEnabled;
    uint32_t autoBackupInterval;
    uint32_t lastAutoBackup;
    
    //    
    bool writeConfigFile(const char* path, const void* data, size_t size);
    ConfigStatus readConfigFile(const char* path, void* data, size_t size);
    
    bool writeHeader(File& file, uint16_t dataSize, uint32_t crc32);
    bool readHeader(File& file, ConfigHeader& header);
    bool validateHeader(const ConfigHeader& header, size_t expectedSize);
    
    void updateStats(const char* operation);
    void ensureDirectoryExists();
};

// ================================================================
//  
// ================================================================
extern ConfigManager configManager;

// ================================================================
//  
// ================================================================
#define CONFIG_SAVE(data) configManager.saveConfig(&data, sizeof(data), true)
#define CONFIG_LOAD(data) configManager.loadConfig(&data, sizeof(data))
#define CONFIG_VERIFY() configManager.verifyConfig(CONFIG_PRIMARY_PATH)
