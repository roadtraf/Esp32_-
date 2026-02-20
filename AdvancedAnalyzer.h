// ================================================================
// AdvancedAnalyzer.h  —  v3.8.3 AI 기반 고급 분석 시스템
// ================================================================
#pragma once
#include "Config.h"
#include "HealthMonitor.h"
#include "MLPredictor.h"
#include "DataLogger.h"

// ─────────────────── 고장 유형 ──────────────────────────────
enum FailureType {
    FAILURE_NONE,
    FAILURE_PUMP_DEGRADATION,      // 펌프 성능 저하
    FAILURE_SEAL_LEAK,             // 씰 누수
    FAILURE_MOTOR_BEARING,         // 모터 베어링 마모
    FAILURE_VALVE_MALFUNCTION,     // 밸브 오작동
    FAILURE_SENSOR_DRIFT,          // 센서 드리프트
    FAILURE_THERMAL_ISSUE,         // 열 문제
    FAILURE_ELECTRICAL,            // 전기적 문제
    FAILURE_MECHANICAL_WEAR        // 기계적 마모
};

// ─────────────────── 예측 결과 ──────────────────────────────
struct FailurePrediction {
    FailureType type;              // 예상 고장 유형
    float confidence;              // 신뢰도 (0-100%)
    uint32_t estimatedDays;        // 예상 발생까지 남은 일수
    char description[128];         // 설명
    char recommendation[256];      // 권장 조치
};

// ─────────────────── 부품 수명 정보 ─────────────────────────
struct ComponentLife {
    const char* name;              // 부품 명
    uint32_t totalHours;           // 총 작동 시간
    uint32_t ratedLifeHours;       // 정격 수명
    float remainingLife;           // 잔여 수명 (%)
    uint32_t daysToReplacement;    // 교체까지 남은 일수
    float healthScore;             // 부품 건강도
};

// ─────────────────── 최적화 제안 ────────────────────────────
struct OptimizationSuggestion {
    char title[64];
    char description[256];
    float estimatedImprovement;    // 예상 개선율 (%)
    uint8_t priority;              // 우선순위 (1-5)
};

// ─────────────────── 분석 리포트 ────────────────────────────
struct AnalysisReport {
    uint32_t timestamp;
    float currentHealth;
    float predictedHealth7d;
    float predictedHealth30d;
    FailurePrediction predictions[3];  // 상위 3개 예측
    uint8_t predictionCount;
    ComponentLife components[5];       // 주요 부품
    uint8_t componentCount;
    OptimizationSuggestion suggestions[5];
    uint8_t suggestionCount;
};

// ─────────────────── AdvancedAnalyzer 클래스 ────────────────
class AdvancedAnalyzer {
public:
    AdvancedAnalyzer();
    
    // 초기화
    void begin();
    
    // 고장 예측
    FailurePrediction predictFailure();
    void predictMultipleFailures(FailurePrediction* predictions, uint8_t maxCount, uint8_t& actualCount);
    
    // 부품 수명 분석
    void analyzeComponentLife(ComponentLife* components, uint8_t& count);
    ComponentLife analyzePump();
    ComponentLife analyzeMotor();
    ComponentLife analyzeSeal();
    ComponentLife analyzeValve();
    ComponentLife analyzeSensor();
    
    // 최적화 제안
    void generateOptimizationSuggestions(OptimizationSuggestion* suggestions, uint8_t& count);
    void suggestMaintenanceSchedule(char* schedule, size_t maxSize);
    void suggestOperationalImprovements(char* improvements, size_t maxSize);
    
    // 종합 리포트
    AnalysisReport generateComprehensiveReport();
    void exportReportToSD(const char* filename);
    
    // 패턴 분석
    bool detectAbnormalPattern(const char* patternType);
    float calculateDegradationRate();  // 시간당 건강도 하락률
    
    // 비용 분석
    float estimateMaintenanceCost();
    float estimateDowntimeCost(uint32_t hours);
    float calculateROI(const char* improvement);  // 개선 ROI
    
    // 벤치마킹
    float compareWithBaseline();       // 기준선과 비교
    void setBaseline();                // 현재 상태를 기준선으로
    
private:
    bool initialized;
    
    // 분석 데이터
    float baselineHealth;
    uint32_t baselineTimestamp;
    float degradationRate;
    
    // 내부 분석 함수
    FailureType analyzeVacuumTrend();
    FailureType analyzeTemperatureTrend();
    FailureType analyzeCurrentTrend();
    FailureType analyzeCombinedPatterns();
    
    // 예측 모델 (간단한 휴리스틱)
    float calculateFailureProbability(FailureType type);
    uint32_t estimateTimeToFailure(FailureType type);
    
    // 부품 분석 헬퍼
    float calculatePumpHealth();
    float calculateMotorHealth();
    float calculateSealHealth();
    float calculateValveHealth();
    float calculateSensorHealth();
    
    // 최적화 헬퍼
    bool shouldOptimizeTiming();
    bool shouldOptimizePID();
    bool shouldReducePower();
    bool shouldIncreaseMaintenance();
    
    // 통계 계산
    float calculateTrendSlope(float* data, uint16_t count);
    float calculateCorrelation(float* x, float* y, uint16_t count);
};

// ─────────────────── 전역 인스턴스 ──────────────────────────
extern AdvancedAnalyzer advancedAnalyzer;

// ─────────────────── 유틸리티 함수 ──────────────────────────
const char* getFailureTypeName(FailureType type);
const char* getFailureTypeDescription(FailureType type);
const char* getFailureTypeRecommendation(FailureType type);
