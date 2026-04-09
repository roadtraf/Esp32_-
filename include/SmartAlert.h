// ================================================================
// SmartAlert.h    v3.8.2   
// ================================================================
#pragma once
#include "Config.h"
#include "HealthMonitor.h"
#include <WiFiClientSecure.h>

//     
struct AlertConfig {
    //  
    bool timeFilterEnabled;       //   
    uint8_t startHour;            //    (0-23)
    uint8_t endHour;              //    (0-23)
    bool weekendAlert;            //   
    
    //  
    bool urgentAlways;            // URGENT  
    bool criticalAlways;          // CRITICAL   
    
    //  
    bool buzzerEnabled;           //  
    bool displayEnabled;          //  
    bool emailEnabled;            //  
    bool smsEnabled;              // SMS  ()
    
    //  
    char smtpServer[64];          // SMTP  (: smtp.gmail.com)
    uint16_t smtpPort;            // SMTP  (: 587)
    char emailFrom[64];           //  
    char emailPassword[64];       //  
    char emailTo[64];             //  
    
    // SMS  ()
    char smsApiKey[64];           // SMS API 
    char phoneNumber[16];         //  
    
    //  
    uint32_t minAlertInterval;    //    (ms)
};

//    
struct AlertHistory {
    uint32_t timestamp;
    MaintenanceLevel level;
    ErrorCode errorCode;
    bool emailSent;
    bool smsSent;
    char message[128];
};

#define MAX_ALERT_HISTORY 50

//  SmartAlert  
class SmartAlert {
public:
    SmartAlert();
    
    // 
    void begin();
    void loadConfig();
    void saveConfig();
    
    // 
    void setConfig(const AlertConfig& cfg);
    AlertConfig getConfig() const { return config; }
    
    //    
    bool shouldAlert(MaintenanceLevel level, ErrorCode error = ERROR_NONE);
    void sendAlert(MaintenanceLevel level, float healthScore, const char* message = nullptr);
    void sendErrorAlert(ErrorCode error, const char* message);
    
    //   
    void sendBuzzerAlert(MaintenanceLevel level);
    void sendDisplayAlert(MaintenanceLevel level, float healthScore, const char* message);
    bool sendEmail(const char* subject, const char* body);
    bool sendSMS(const char* message);
    
    //  
    bool isWorkingHours();
    bool isWeekend();
    
    //  
    void addToHistory(MaintenanceLevel level, ErrorCode error, const char* message);
    AlertHistory* getHistory(uint8_t& count);
    void clearHistory();
    
    // 
    uint32_t getTotalAlertsSent();
    uint32_t getEmailsSent();
    uint32_t getSmsSent();
    uint32_t getLastAlertTime();
    
private:
    AlertConfig config;
    bool initialized;
    
    //  
    AlertHistory history[MAX_ALERT_HISTORY];
    uint8_t historyCount;
    uint8_t historyIndex;
    
    // 
    uint32_t totalAlerts;
    uint32_t emailsSent;
    uint32_t smsSent;
    uint32_t lastAlertTime[5];  //    
    
    //  
    bool canSendAlert(MaintenanceLevel level);
    void formatEmailBody(char* buffer, size_t size, 
                        MaintenanceLevel level, 
                        float healthScore, 
                        const char* message);
    void formatSmsMessage(char* buffer, size_t size,
                         MaintenanceLevel level, 
                         float healthScore);
                         
    //   
    bool connectToSMTP(WiFiClientSecure& client);
    bool sendSMTPCommand(WiFiClientSecure& client, const char* command, const char* expectedResponse);
};

//    
extern SmartAlert smartAlert;

//    
#define DEFAULT_START_HOUR 8
#define DEFAULT_END_HOUR 18
#define DEFAULT_MIN_ALERT_INTERVAL (15 * 60 * 1000)  // 15
