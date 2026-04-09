// ================================================================
// HardenedConfig.h -      
// v3.9.4 Hardened Edition
// ================================================================
//  :
//   [1] Brownout false reset 
//   [2] Task Watchdog  
//   [3] PSRAM   
//   [4] SD  
//   [5] I2C   (SDA LOW , ,  )
//   [6] WiFi reconnect non-blocking
//   [7] Heap fragmentation 
//   [8] SPI   (ILI9488 + XPT2046 + SD)
//   [9] DS18B20   + WDT 
// ================================================================
#pragma once

#include <Arduino.h>
#include <esp_system.h>
#include <soc/rtc.h>

//  [1] BROWNOUT  
// ADC  brownout detector     
// esp_brownout_disable()     
// : BOD      ( 2.43V)
#define BROWNOUT_DET_LEVEL      0       // 0=2.43V (,  )
                                        // 7=3.00V (,  )

// Brownout         
#define BROWNOUT_RETRY_DELAY_MS  500    // 500ms  
#define BROWNOUT_MAX_RETRIES     3      //  3   

//  [2] TASK WATCHDOG   
// : WDT_TIMEOUT=10s  WiFi connectWiFi()  10s   WDT reset
// : WDT timeout  +  Task  WDT 

#define WDT_TIMEOUT_HW          15      //  WDT () -  
#define WDT_TIMEOUT_TASK_VACUUM  3000   // VacuumCtrl    (ms)
#define WDT_TIMEOUT_TASK_SENSOR  3000   // SensorRead    (ms)
#define WDT_TIMEOUT_TASK_UI      5000   // UIUpdate    (ms)
#define WDT_TIMEOUT_TASK_WIFI    30000  // WiFiMgr  (ms) -  
#define WDT_TIMEOUT_TASK_MQTT    10000  // MQTTHandler  (ms)
#define WDT_TIMEOUT_TASK_LOGGER  5000   // DataLogger  (ms)

// WDT feed   -     feed
#define WDT_FEED_MAX_INTERVAL_MS 8000

//  [3] PSRAM    
// ESP32-S3: 8MB PSRAM (OPI PSRAM)
// :  malloc  SRAM     crash
// :    PSRAM 
#define PSRAM_BUFFER_THRESHOLD   1024   // 1KB   PSRAM 
#define PSRAM_SAFE_ALLOC(size)  \
    ((size >= PSRAM_BUFFER_THRESHOLD && psramFound()) ? \
     heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : \
     malloc(size))
#define PSRAM_SAFE_FREE(ptr)    free(ptr)

//  SRAM    ( PSRAM )
#define INTERNAL_HEAP_MIN_FREE  32768   // 32KB
//  
#define HEAP_WARN_THRESHOLD     16384   // 16KB  

// SensorBuffer: PSRAM   
#define SENSOR_BUFFER_SIZE_PSRAM 500    // PSRAM 500  
#define SENSOR_BUFFER_SIZE_SRAM  50     // SRAM fallback

//  [4] SD   
// SD.open()   SPI     
// : FreeRTOS  +  
#define SD_WRITE_TIMEOUT_MS     2000    // SD   (ms)
#define SD_OPEN_TIMEOUT_MS      1000    // SD.open()  (ms)
#define SD_MAX_RETRY_COUNT      3       // SD   
#define SD_RETRY_DELAY_MS       200     //   (ms)

// SD SPI CS  (Config.h )
#define SD_CS_PIN               46      // SD  CS 

//  [5] I2C   
// : SDA LOW  Wire.begin()  
// : GPIO    9   SDA  
#define I2C_SDA_PIN             19      // I2C SDA 
#define I2C_SCL_PIN             20      // I2C SCL 
#define I2C_FREQ_HZ             100000  // 100kHz (  400kHz )
#define I2C_TIMEOUT_MS          50      // I2C   (ms)
#define I2C_RECOVER_CLOCK_COUNT 9       // SDA   SCL  
#define I2C_RECOVER_DELAY_US    5       //    (us)
#define I2C_MAX_RETRY           3       // I2C   
#define I2C_SENSOR_WARMUP_MS    200     //       (ms)

//  [6] WiFi   
// : connectWiFi() while(WL_CONNECTED)  10s 
//        Core0 WiFi Task WDT reset 
// :     

#define WIFI_CONNECT_STEP_MS    500     //     (ms)
#define WIFI_MAX_CONNECT_STEPS  20      //  20 * 500ms = 10s
#define WIFI_BACKOFF_BASE_MS    1000    //   
#define WIFI_BACKOFF_MAX_MS     30000   //    (30s)
#define WIFI_BACKOFF_MULTIPLIER 2       //   

//  [7] HEAP FRAGMENTATION  
// : std::vector::push_back   realloc  
// : capacity   +    

// SensorBuffer: std::vector    
#define CIRCULAR_BUFFER_SIZE    100     //    ()

// ArduinoJson  
#define JSON_DOC_SIZE_SENSOR    256     //   JSON
#define JSON_DOC_SIZE_MQTT      512     // MQTT  JSON
#define JSON_DOC_SIZE_CONFIG    1024    //  JSON

//  [8] SPI    
// : ILI9488 (TFT) + XPT2046 () + SD    SPI  
// : UITask TFT   TouchTask XPT2046   
//       beginTransaction    
// : SPI   Mutex +      

// SPI CS  
#define TFT_CS_PIN              10      // ILI9488 CS (LovyanGFX )
#define TOUCH_CS_PIN            14      // XPT2046 CS
#define SD_CS_PIN_SPI           46      // SD CS ( [4] )

// SPI   
#define SPI_MUTEX_TIMEOUT_MS    100     // 100ms     skip

//  [9] DS18B20   
// : requestTemperatures()  750ms  (setWaitForConversion=true)
//       OneWire      UI/ 
// : FreeRTOS Task  +   +  

#define DS18B20_CONVERSION_TIME_MS  800 // 12bit : 750ms + 50ms 
#define DS18B20_TASK_STACK         2048 // DS18B20   
#define DS18B20_TASK_PRIORITY       1   //   (  )
#define DS18B20_FALLBACK_TEMP      25.0f //    

//     
#define HEALTH_CHECK_INTERVAL_MS    5000  //    
#define LOG_HEAP_INTERVAL_MS        10000 //    

//    
#define HW_HARDENED_VERSION "v3.9.4"
#define HW_HARDENED_DATE    "2026-02-17"
