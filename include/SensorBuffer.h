// SensorBuffer.h
#ifndef SENSOR_BUFFER_H
#define SENSOR_BUFFER_H

#include <Arduino.h>

// SensorData  ( )
#ifndef SENSOR_DATA_DEFINED
#define SENSOR_DATA_DEFINED
struct SensorData {
    float    pressure;
    float    current;
    float    temperature;
    bool     limitSwitch;
    bool     photoSensor;
    bool     emergencyStop;
    uint32_t timestamp;
};
#endif


// ================================================================
// Ring Buffer Template -   
// ================================================================
template<typename T, size_t CAPACITY>
class RingBuffer {
private:
    T buffer[CAPACITY];
    volatile size_t head;
    volatile size_t tail;
    volatile size_t count;
    SemaphoreHandle_t mutex;
    
public:
    RingBuffer() : head(0), tail(0), count(0) {
        mutex = xSemaphoreCreateMutex();
        memset(buffer, 0, sizeof(buffer));
    }
    
    ~RingBuffer() {
        if (mutex) {
            vSemaphoreDelete(mutex);
        }
    }
    
    // Add item to buffer
    bool push(const T& item) {
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (count >= CAPACITY) {
                // Buffer full, overwrite oldest
                tail = (tail + 1) % CAPACITY;
            } else {
                count++;
            }
            
            buffer[head] = item;
            head = (head + 1) % CAPACITY;
            
            xSemaphoreGive(mutex);
            return true;
        }
        return false;
    }
    
    // Get item from buffer
    bool pop(T& item) {
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (count == 0) {
                xSemaphoreGive(mutex);
                return false;
            }
            
            item = buffer[tail];
            tail = (tail + 1) % CAPACITY;
            count--;
            
            xSemaphoreGive(mutex);
            return true;
        }
        return false;
    }
    
    // Peek at latest item without removing
    bool peek(T& item) const {
        if (count == 0) return false;
        size_t lastIndex = (head == 0) ? CAPACITY - 1 : head - 1;
        item = buffer[lastIndex];
        return true;
    }
    
    // Get current size
    size_t size() const {
        return count;
    }
    
    // Check if buffer is full
    bool isFull() const {
        return count >= CAPACITY;
    }
    
    // Check if buffer is empty
    bool isEmpty() const {
        return count == 0;
    }
    
    // Clear buffer
    void clear() {
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            head = 0;
            tail = 0;
            count = 0;
            xSemaphoreGive(mutex);
        }
    }
    
    // Get average of all values in buffer
    T getAverage() const {
        if (count == 0) return T(0);
        
        T sum = 0;
        size_t index = tail;
        for (size_t i = 0; i < count; i++) {
            sum += buffer[index];
            index = (index + 1) % CAPACITY;
        }
        return sum / count;
    }
    
    // Get maximum value in buffer
    T getMax() const {
        if (count == 0) return T(0);
        
        T maxVal = buffer[tail];
        size_t index = tail;
        for (size_t i = 0; i < count; i++) {
            if (buffer[index] > maxVal) {
                maxVal = buffer[index];
            }
            index = (index + 1) % CAPACITY;
        }
        return maxVal;
    }
    
    // Get minimum value in buffer
    T getMin() const {
        if (count == 0) return T(0);
        
        T minVal = buffer[tail];
        size_t index = tail;
        for (size_t i = 0; i < count; i++) {
            if (buffer[index] < minVal) {
                minVal = buffer[index];
            }
            index = (index + 1) % CAPACITY;
        }
        return minVal;
    }
    
    // Get standard deviation
    float getStdDev() const {
        if (count < 2) return 0.0f;
        
        float avg = static_cast<float>(getAverage());
        float sumSquares = 0.0f;
        
        size_t index = tail;
        for (size_t i = 0; i < count; i++) {
            float diff = static_cast<float>(buffer[index]) - avg;
            sumSquares += diff * diff;
            index = (index + 1) % CAPACITY;
        }
        
        return sqrt(sumSquares / count);
    }
};

// ================================================================
//   
// ================================================================

// ================================================================
//   
// ================================================================
#define SENSORSTATS_DEFINED
struct SensorStats {
    // Temperature stats
    float avgTemperature;
    float maxTemperature;
    float minTemperature;
    float tempStdDev;
    
    // Pressure stats
    float avgPressure;
    float maxPressure;
    float minPressure;
    float pressureStdDev;
    
    // Current stats
    float avgCurrent;
    float maxCurrent;
    float minCurrent;
    float currentStdDev;
    
    uint32_t sampleCount;
    
    void reset() {
        memset(this, 0, sizeof(SensorStats));
    }
};


// ================================================================
//   
// ================================================================
constexpr size_t TEMP_BUFFER_SIZE = 60;       // 60 samples = 1 @ 1Hz
constexpr size_t PRESSURE_BUFFER_SIZE = 60;
constexpr size_t CURRENT_BUFFER_SIZE = 60;
constexpr size_t SENSOR_DATA_BUFFER_SIZE = 20;

extern RingBuffer<float, TEMP_BUFFER_SIZE> temperatureBuffer;
extern RingBuffer<float, PRESSURE_BUFFER_SIZE> pressureBuffer;
extern RingBuffer<float, CURRENT_BUFFER_SIZE> currentBuffer;
extern RingBuffer<SensorData, SENSOR_DATA_BUFFER_SIZE> sensorDataBuffer;

// ================================================================
//   
// ================================================================

//    
void updateSensorBuffers();

//  
void calculateSensorStats(SensorStats& stats);

//  
void clearSensorBuffers();

//   
void printBufferStatus();

//   
float getAvgTemperature();
float getAvgPressure();
float getAvgCurrent();
float getMaxTemperature();
float getMaxPressure();
float getMaxCurrent();
float getMinTemperature();
float getMinPressure();
float getMinCurrent();

#endif // SENSOR_BUFFER_H