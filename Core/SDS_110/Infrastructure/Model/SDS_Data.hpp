/*
 * SDS_Data.hpp  (Infrastructure)
 *
 * Synchronisiertes Status-/Debug-Modell für LCD-, Logger- und USB-Task.
 * Enthält KEINE Signalverarbeitung mehr: 120 schreibt Ergebnisse hinein,
 * die Anzeige-/Transport-Tasks lesen sie. Mutex-geschützt (CMSIS-RTOS2).
 *
 * Migration aus SDS/Model/SDS_Data – entfernt:
 *   computeMelFeatures(), FFTProcessor/MelFilterbank/MelSpectrogram, fftMag/melOut
 *       -> Feature_Extraction_Module_122
 *   setAiDrone/Human/Wind/Background, droneDetected  (4-Klassen-Modell)
 *       -> ersetzt durch AcousticState-Spiegel (64 Bandwahrscheinlichkeiten, HBD-ML)
 *   srp-, lcd-, mic-, usb-Task-Statistiken -> generisches TaskStats[TaskId]
 * Beibehalten: Error-Queue, Event-Queue, Mode/Simulation, Debug-Werte, Error-Buffer.
 */
#pragma once
#include <cstdint>
#include <cstring>
#include "cmsis_os2.h"
#include "SDS_Params.hpp"
#include "SDS_Structs.hpp"
#include "Data_Interface_140/Candidate_Report_140.hpp"

enum class TaskId : uint8_t { Proc120 = 0, Lcd, Mic, Usb, Logger, Count };

struct TaskStats {
    uint32_t freeStack  = 0;
    float    loopTime   = 0.0f;
    uint32_t loopCounter = 0;
};

class SDS_Data {
public:
    static SDS_Data& instance();

    // --- Ergebnis der Patent-Kette (von 120 geschrieben) -------------------
    void setAcousticState(const sds110::AcousticState& s);
    void getAcousticState(sds110::AcousticState& out) const;
    /// Kandidat aus 128: Azimut, Distanz, Qualität
    void setCandidate(float azimuthDeg, float distanceM, uint8_t acceptedPairs, float residual, bool valid);

    float    getAzimuth()  const { return getValue(azimuthDeg); }
    float    getDistance() const { return getValue(distance); }
    float    getConfidence() const { return getValue(confidence); }
    bool     getDetected() const { return getValue(detected); }
    uint32_t getSelectedBands() const { return getValue(selectedBands); }
    uint32_t getReportCount() const { return getValue(reportCount); }

    // --- HBD-ML Status ------------------------------------------------------
    void setMlInitError(bool v) { setValue(mlInitError, v); }
    bool getMlInitError() const { return getValue(mlInitError); }
    void setMlRunError(bool v)  { setValue(mlRunError, v); }
    bool getMlRunError() const  { return getValue(mlRunError); }

    // --- System Status ------------------------------------------------------
    /// Unit-ID: Standard aus STM32-UID (siehe SDS110_Init), per USB-Kommando Typ 5 überschreibbar
    void setId(uint16_t v) { setValue(id, v); }
    uint16_t getId() const { return getValue(id); }
    void setMode(SDS_Mode v) { setValue(mode, v); }
    SDS_Mode getMode() const { return getValue(mode); }
    void setSimulation(uint32_t v) { setValue(simulation, v); }
    uint32_t getSimulation() const { return getValue(simulation); }
    void setSyncTimeDifference(uint32_t v) { setValue(syncTimeDifference, v); }
    uint32_t getSyncTimeDifference() const { return getValue(syncTimeDifference); }

    // --- Task-Statistiken ---------------------------------------------------
    void setTaskStats(TaskId t, uint32_t freeStack, float loopTime, uint32_t loopCounter);
    TaskStats getTaskStats(TaskId t) const;

    // --- Debug --------------------------------------------------------------
    void setDebugTime(float v)   { setValue(debugTime, v); }
    float getDebugTime() const   { return getValue(debugTime); }
    void setDebugValue(uint32_t i, float v);
    float getDebugValue(uint32_t i) const;

    // --- Error --------------------------------------------------------------
    void pushErrorMessage(const char* msg);
    bool popErrorMessage(SDS_ErrorMessage& out);
    void setErrorBuffer(const uint8_t* src, uint32_t len);
    uint32_t getErrorBuffer(uint8_t* dst, uint32_t maxLen) const;
    void setErrorFlag(uint32_t v)  { setValue(errorFlag, v); }
    uint32_t getErrorFlag() const  { return getValue(errorFlag); }
    void setErrorCount(uint32_t v) { setValue(errorCount, v); }
    uint32_t getErrorCount() const { return getValue(errorCount); }
    void setUsbErrorCount(uint32_t v) { setValue(usbErrorCount, v); }
    uint32_t getUsbErrorCount() const { return getValue(usbErrorCount); }

    // --- Event-Queue --------------------------------------------------------
    osMessageQueueId_t eventQueue() const { return eventQueue_; }
    void pushEvent(SDS_DataEventType type, float processTime);

private:
    SDS_Data();

    bool lock(uint32_t timeout = 2) const { return osMutexAcquire(mutex_, timeout) == osOK; }
    void unlock() const { osMutexRelease(mutex_); }

    template<typename T> void setValue(T& target, const T& value)
    {
        if (!lock()) { errorFlag = 999; return; }
        target = value;
        unlock();
    }
    template<typename T> T getValue(const T& target) const
    {
        if (!lock()) return T{};
        T v = target;
        unlock();
        return v;
    }

    mutable osMutexId_t mutex_;
    osMessageQueueId_t  eventQueue_;
    osMessageQueueId_t  errorMsgQueue_;

    // Ergebnis 120
    sds110::AcousticState acousticState{};
    float    azimuthDeg = 0, distance = 0, confidence = 0;
    bool     detected = false;
    uint32_t selectedBands = 0;
    uint32_t reportCount = 0;
    bool     mlInitError = false, mlRunError = false;

    // System
    uint16_t id = 0;
    SDS_Mode mode = SDS_Mode::DETECT;
    uint32_t simulation = 1;
    uint32_t syncTimeDifference = 0;

    TaskStats tasks_[static_cast<uint8_t>(TaskId::Count)];

    // Debug
    float debugTime = 0;
    float debugValue[4] = {};

    // Error
    uint32_t errorFlag = 0, errorLen = 0, errorCount = 0, usbErrorCount = 0;
    uint8_t  errorBuffer_[64] = {};
};
