// ================================================================
// AdvancedAnalyzer.h    v3.8.3 AI    
// ================================================================
#pragma once
#include "Config.h"
#include "HealthMonitor.h"
#include "MLPredictor.h"
#include "DataLogger.h"

//    
enum FailureType {
    FAILURE_NONE,
    FAILURE_PUMP_DEGRADATION,      //   
    FAILURE_SEAL_LEAK,             //  
    FAILURE_MOTOR_BEARING,         //   
    FAILURE_VALVE_MALFUNCTION,     //  
    FAILURE_SENSOR_DRIFT,          //  
    FAILURE_THERMAL_ISSUE,         //  
    FAILURE_ELECTRICAL,            //  
    FAILURE_MECHANICAL_WEAR        //  
};

//    
struct FailurePrediction {
    FailureType type;              //   
    float confidence;              //  (0-100%)
    uint32_t estimatedDays;        //    
    char description[128];         // 
    char recommendation[256];      //  
};

//     
struct ComponentLife {
    const char* name;              //  
    uint32_t totalHours;           //   
    uint32_t ratedLifeHours;       //  
    float remainingLife;           //   (%)
    uint32_t daysToReplacement;    //   
    float healthScore;             //  
};

//    
struct OptimizationSuggestion {
    char title[64];
    char description[256];
    float estimatedImprovement;    //   (%)
    uint8_t priority;              //  (1-5)
};

//    
struct AnalysisReport {
    uint32_t timestamp;
    float currentHealth;
    float predictedHealth7d;
    float predictedHealth30d;
    FailurePrediction predictions[3];  //  3 
    uint8_t predictionCount;
    ComponentLife components[5];       //  
    uint8_t componentCount;
    OptimizationSuggestion suggestions[5];
    uint8_t suggestionCount;
};

//  AdvancedAnalyzer  
class AdvancedAnalyzer {
public:
    AdvancedAnalyzer();
    
    // 
    void begin();
    
    //  
    FailurePrediction predictFailure();
    void predictMultipleFailures(FailurePrediction* predictions, uint8_t maxCount, uint8_t& actualCount);
    
    //   
    void analyzeComponentLife(ComponentLife* components, uint8_t& count);
    ComponentLife analyzePump();
    ComponentLife analyzeMotor();
    ComponentLife analyzeSeal();
    ComponentLife analyzeValve();
    ComponentLife analyzeSensor();
    
    //  
    void generateOptimizationSuggestions(OptimizationSuggestion* suggestions, uint8_t& count);
    void suggestMaintenanceSchedule(char* schedule, size_t maxSize);
    void suggestOperationalImprovements(char* improvements, size_t maxSize);
    
    //  
    AnalysisReport generateComprehensiveReport();
    void exportReportToSD(const char* filename);
    
    //  
    bool detectAbnormalPattern(const char* patternType);
    float calculateDegradationRate();  //   
    
    //  
    float estimateMaintenanceCost();
    float estimateDowntimeCost(uint32_t hours);
    float calculateROI(const char* improvement);  //  ROI
    
    // 
    float compareWithBaseline();       //  
    void setBaseline();                //   
    
private:
    bool initialized;
    
    //  
    float baselineHealth;
    uint32_t baselineTimestamp;
    float degradationRate;
    
    //   
    FailureType analyzeVacuumTrend();
    FailureType analyzeTemperatureTrend();
    FailureType analyzeCurrentTrend();
    FailureType analyzeCombinedPatterns();
    
    //   ( )
    float calculateFailureProbability(FailureType type);
    uint32_t estimateTimeToFailure(FailureType type);
    
    //   
    float calculatePumpHealth();
    float calculateMotorHealth();
    float calculateSealHealth();
    float calculateValveHealth();
    float calculateSensorHealth();
    
    //  
    bool shouldOptimizeTiming();
    bool shouldOptimizePID();
    bool shouldReducePower();
    bool shouldIncreaseMaintenance();
    
    //  
    float calculateTrendSlope(float* data, uint16_t count);
    float calculateCorrelation(float* x, float* y, uint16_t count);
};

//    
extern AdvancedAnalyzer advancedAnalyzer;

//    
const char* getFailureTypeName(FailureType type);
const char* getFailureTypeDescription(FailureType type);
const char* getFailureTypeRecommendation(FailureType type);
