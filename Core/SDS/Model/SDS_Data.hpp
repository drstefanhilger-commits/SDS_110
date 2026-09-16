/*
 * SDS_Data.cpp
 *
 * System Integration Description
 * ------------------------------
 * SDS_Data implements the synchronized global data model of the SDS
 * (Sensor‑DSP‑System). It provides deterministic, thread‑safe access to
 * all real‑time DSP results such as:
 *
 *      - azimuth (degrees)
 *      - distance (meters)
 *      - confidence (0…1)
 *      - detection flag
 *      - debug values
 *
 * Thread‑Safety Model:
 * --------------------
 * All read/write operations are protected by a CMSIS‑RTOS2 mutex:
 *
 *      osMutexAcquire(mutex, osWaitForever);
 *      ... update or read shared state ...
 *      osMutexRelease(mutex);
 *
 * This ensures deterministic behavior under concurrent access from:
 *
 *   - SRPTask (azimuth + distance)
 *   - MicTask (confidence + debug values)
 *   - LCDTask (display reads)
 *
 * Event Queue:
 * ------------
 * SDS_Data also provides an RTOS message queue for asynchronous events.
 * Each write operation pushes an event:
 *
 *      pushEvent(SDS_DataEventType::SRP_UPDATE, processTime);
 *
 * This allows the DisplayManager or other tasks to react to DSP updates
 * without polling.
 *
 * Created on: Jun 18, 2026
 * Author: Stefan (310004)
 */

#pragma once

#include <cstdint>
#include "cmsis_os.h"
#include "string.h"
#include "SDS_Params.hpp"
#include "SDS_Structs.hpp"

// Forward declarations für FFT/Mel
class FFTProcessor;
class MelFilterbank;
class MelSpectrogram;

class SDS_Data {
public:
    static SDS_Data& instance();

    void computeMelFeatures(float out[40]);   // 40 Mel-Bänder

    // --- Error-Message Queue ------------------------------------------------
    void pushErrorMessage(const char* msg);
    bool popErrorMessage(SDS_ErrorMessage& out);
    // ------------------------------------------------------------------------


    // --- Azimuth ---
    void setAzimuth(float v) { setValue(azimuthDeg, v); }
    float getAzimuth() const { return getValue(azimuthDeg); }

    // --- Distance ---
    void setDistance(float v) { setValue(distance, v); }
    float getDistance() const { return getValue(distance); }

    // --- Confidence ---
    void setConfidence(float v) { setValue(confidence, v); }
    float getConfidence() const { return getValue(confidence); }

    // --- Detection ---
    void setDetected(bool v) { setValue(detected, v); }
    bool getDetected() const { return getValue(detected); }
    // ------------------------------------------------------------------------


    // --- System Status ------------------------------------------------------
    void setId(uint8_t v) { setValue(id, v); }
    uint8_t getId() const { return getValue(id); }

    void setMode(uint32_t v) { setValue(mode, v); }
    uint32_t getMode() const { return getValue(mode); }

    void setSimulation(uint32_t v) { setValue(simulation, v); }
    uint32_t getSimulation() const { return getValue(simulation); }

    void setSyncTimeDifference(uint32_t v) { setValue(syncTimeDifference, v); }
    uint32_t getSyncTimeDifference() const { return getValue(syncTimeDifference); }
    // -------------------------------------------------------------------------


    // --- SRP Task -----------------------------------------------------------
    void setSrpTaskFreeStack(uint32_t v) { setValue(srpTaskFreeStack, v); }
    uint32_t getSrpTaskFreeStack() const { return getValue(srpTaskFreeStack); }

    void setSrpLoopTime(float v) { setValue(srpLoopTime, v); }
    float getSrpLoopTime() const { return getValue(srpLoopTime); }

    void setSrpLoopCounter(uint32_t v) { setValue(srpLoopCounter, v); }
    uint32_t getSrpLoopCounter() const { return getValue(srpLoopCounter); }
    // -------------------------------------------------------------------------


    // --- LCD Task ------------------------------------------------------------
    void setLcdTaskFreeStack(uint32_t v) { setValue(lcdTaskFreeStack, v); }
    uint32_t getLcdTaskFreeStack() const { return getValue(lcdTaskFreeStack); }

    void setLcdLoopTime(float v) { setValue(lcdLoopTime, v); }
    float getLcdLoopTime() const { return getValue(lcdLoopTime); }

    void setLcdLoopCounter(uint32_t v) { setValue(lcdLoopCounter, v); }
    uint32_t getLcdLoopCounter() const { return getValue(lcdLoopCounter); }
    // -------------------------------------------------------------------------


    // --- Mic Task ------------------------------------------------------------
    void setMicTaskFreeStack(uint32_t v) { setValue(micTaskFreeStack, v); }
    uint32_t getMicTaskFreeStack() const { return getValue(micTaskFreeStack); }

    void setMicLoopTime(float v) { setValue(micLoopTime, v); }
    float getMicLoopTime() const { return getValue(micLoopTime); }

    void setMicLoopCounter(uint32_t v) { setValue(micLoopCounter, v); }
    uint32_t getMicLoopCounter() const { return getValue(micLoopCounter); }
   // -------------------------------------------------------------------------


    // --- USB Task ------------------------------------------------------------
    void setUsbTaskFreeStack(uint32_t v) { setValue(usbTaskFreeStack, v); }
    uint32_t getUsbTaskFreeStack() const { return getValue(usbTaskFreeStack); }
    // -------------------------------------------------------------------------


    // --- Debug ---------------------------------------------------------------
    void setDebugTime(float v) { setValue(debugTime, v); }
    float getDebugTime() const { return getValue(debugTime); }

    void setDebugValue(float v) { setValue(debugValue, v); }
    float getDebugValue() const { return getValue(debugValue); }

    void setDebugValue1(float v) { setValue(debugValue1, v); }
    float getDebugValue1() const { return getValue(debugValue1); }

    void setDebugValue2(float v) { setValue(debugValue2, v); }
    float getDebugValue2() const { return getValue(debugValue2); }

    void setDebugValue3(float v) { setValue(debugValue3, v); }
    float getDebugValue3() const { return getValue(debugValue3); }
    // -------------------------------------------------------------------------


    // --- Error Handler -------------------------------------------------------
    void setErrorBuffer(const uint8_t* src, uint32_t len)
    {
        if (!lock()) {
            errorFlag = 998;
            return;
        }
        memcpy(_errorBuffer, src, len);
        errorLen = len;
        unlock();
    }

    uint8_t* getErrorBuffer()
    {
        if (!lock()) return nullptr;
        auto ptr = _errorBuffer;
        unlock();
        return ptr;
    }

    void setErrorFlag(uint32_t v) { setValue(errorFlag, v); }
    uint32_t getErrorFlag() const { return getValue(errorFlag); }

    void setErrorLen(uint32_t v) { setValue(errorLen, v); }
    uint32_t getErrorLen() const { return getValue(errorLen); }

    void setErrorCount(uint32_t v) { setValue(errorCount, v); }
    uint32_t getErrorCount() const { return getValue(errorCount); }

    void setUsbErrorCount(uint32_t v) { setValue(usbErrorCount, v); }
    uint32_t getUsbErrorCount() const { return getValue(usbErrorCount); }
    // -------------------------------------------------------------------------


    // --- AI Werte ------------------------------------------------------------
    void setAiDrone(float v) { setValue(aiDrone, v); }
    float getAiDrone() const { return getValue(aiDrone); }

    void setAiHuman(float v) { setValue(aiHuman, v); }
    float getAiHuman() const { return getValue(aiHuman); }

    void setAiWind(float v) { setValue(aiWind, v); }
    float getAiWind() const { return getValue(aiWind); }

    void setAiBackground(float v) { setValue(aiBackground, v); }
    float getAiBackground() const { return getValue(aiBackground); }

    void setAiInitError(bool v) { setValue(aiInitError, v); }
    bool getAiInitError() const { return getValue(aiInitError); }

    void setAiRunError(bool v) { setValue(aiRunError, v); }
    bool getAiRunError() const { return getValue(aiRunError); }

    void setDroneDetected(bool v) { setValue(droneDetected, v); }
    bool getDroneDetected() const { return getValue(droneDetected); }
    // -------------------------------------------------------------------------


private:
    SDS_Data();

    inline bool lock(uint32_t timeout = 2) const
    {
        return osMutexAcquire(mutex, timeout) == osOK;
    }

    inline void unlock() const
    {
        osMutexRelease(mutex);
    }

    template<typename T>
    inline void setValue(T& target, const T& value)
    {
        if (!lock()) {
            errorFlag = 999;   // deterministischer Fehlercode
            return;
        }
        target = value;
        unlock();
    }

    template<typename T>
    inline T getValue(const T& target) const
    {
        if (!lock()) {
            return T{};        // deterministischer Fallback
        }
        T v = target;
        unlock();
        return v;
    }

    // ---------------------------------------------------------------------
    // Event Queue API
    // ---------------------------------------------------------------------
    osMessageQueueId_t getEventQueue() const { return eventQueue; }
    void pushEvent(SDS_DataEventType type, float processTime);

    osMessageQueueId_t errorMsgQueue;


//
private:
    mutable osMutexId_t mutex;

    // AI internal data
    float aiDrone;
    float aiHuman;
    float aiWind;
    float aiBackground;
    bool aiInitError;
    bool aiRunError;
    bool droneDetected;

    // Detect values
    float azimuthDeg;
    float distance;
    float confidence;
    bool  detected;

    // System Status
    uint8_t id;
    uint32_t mode;
    uint32_t simulation;
    uint32_t syncTimeDifference;

    // SRPTask
    uint32_t srpTaskFreeStack;
    float srpLoopTime;
    uint32_t srpLoopCounter;

    // LCDTask
    uint32_t lcdTaskFreeStack;
    float lcdLoopTime;
    uint32_t lcdLoopCounter;

    // MicTask
    uint32_t micTaskFreeStack;
    float micLoopTime;
    uint32_t micLoopCounter;

    // USBTask
    uint32_t usbTaskFreeStack;
    float usbLoopTime;
    uint32_t usbLoopCounter;

    // Debug values
    float debugTime;
    float debugValue;
    float debugValue1;
    float debugValue2;
    float debugValue3;

    // Error
    uint32_t errorFlag;
    uint32_t errorLen;
    uint32_t errorCount;
    uint32_t usbErrorCount;
    uint8_t _errorBuffer[64];

    // ---------------------------------------------------------------------
    // NEW: FFT + Mel objects
    // ---------------------------------------------------------------------
    FFTProcessor* fft;
    MelFilterbank* melFB;
    MelSpectrogram* melSpec;

    float fftMag[129];
    float melOut[40];

    // Event queue
    osMessageQueueId_t eventQueue;
};
