/*
 * SDS_Data.hpp
 *
 * Mathematical / Physical Description
 * -----------------------------------
 * SDS_Data is the central shared data model of the SDS (Sensor‑DSP‑System).
 * It provides deterministic, thread‑safe access to all real‑time DSP results
 * such as:
 *
 *      - azimuth (degrees)
 *      - distance (meters)
 *      - confidence (0…1)
 *      - detection flag
 *      - debug values
 *
 * The class acts as a synchronized state container between:
 *
 *   - DSP‑producing tasks:
 *         SRPTask, MicTask, DSPTask
 *
 *   - DSP‑consuming tasks:
 *         LCDTask (DisplayManager)
 *
 * Thread‑Safety Model:
 * --------------------
 * All write and read operations are protected by a CMSIS‑RTOS2 mutex:
 *
 *      lock(mutex)
 *      update or read shared state
 *      unlock(mutex)
 *
 * This ensures deterministic behavior under concurrent access and prevents
 * race conditions between high‑frequency DSP updates and lower‑frequency
 * visualization tasks.
 *
 * Event Queue:
 * ------------
 * SDS_Data also provides an RTOS message queue for asynchronous events:
 *
 *      pushEvent(type, processTime)
 *
 * Events allow tasks to signal:
 *   - SRP updates
 *   - DSP timing information
 *   - detection changes
 *
 * Timing Model:
 * -------------
 * The data model is updated periodically by DSP tasks with sampling period:
 *
 *      T = delayMs / 1000 seconds
 *
 * ensuring consistent real‑time behavior.
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

//// Counter for the *.c interface
//extern "C" volatile uint32_t usb_counter1 = 0;
//extern "C" volatile uint32_t usb_counter2 = 0;
//extern "C" volatile uint32_t usb_counter3 = 0;


class SDS_Data {
public:
    // Singleton instance — ensures a single global data model.
    static SDS_Data& instance();

    // ---------------------------------------------------------------------
    // Write API (called by DSPTask, SRPTask, MicTask, SystemManager)
    // ---------------------------------------------------------------------
    void setAzimuth(float val);
    void setDistance(float val);
    void setConfidence(float val);
    void setDetected(bool val);

    void setDebugTime(float val);
    void setSRPPhatTime(float val);
    void setDebugValue(float val);
    void setDebugValue1(float val);
    void setDebugValue2(float val);
    void setDebugValue3(float val);
    void setId(uint8_t val);
    void setMode(uint32_t val);
    void setSimulation(uint32_t val);
	void setSyncTimeDifference(uint32_t val);
	void setMicLoopCounter(uint32_t val);
	void setLcdLoopCounter(uint32_t val);
	void setSrpLoopCounter(uint32_t val);
	void setErrorFlag(uint32_t val);
	void setErrorLen(uint32_t len);
	void setErrorCount(uint32_t val);

    // ---------------------------------------------------------------------
    // Read API (called by DisplayManager / LCDTask)
    // ---------------------------------------------------------------------
    float getAzimuth() const;
    float getDistance() const;
    float getConfidence() const;
    bool  getDetected() const;

    float getDebugTime() const;
    float getSRPPhatTime() const;
    float getDebugValue() const;
    float getDebugValue1() const;
    float getDebugValue2() const;
    float getDebugValue3() const;
    uint8_t getId() const;
    uint32_t getMode() const;
	uint32_t getSyncTimeDifference() const;
    uint32_t getSimulation() const;
	uint32_t getMicLoopCounter() const;
	uint32_t getLcdLoopCounter() const;
	uint32_t getSrpLoopCounter() const;
	uint32_t getErrorFlag() const;
	uint32_t getErrorLen() const;
	uint8_t* getErrorBuffer();
	uint32_t getErrorCount();

    // ---------------------------------------------------------------------
    // Event Queue API
    // ---------------------------------------------------------------------
    osMessageQueueId_t getEventQueue() const { return eventQueue; }

    // Pushes an event into the RTOS queue (non‑blocking).
    void pushEvent(SDS_DataEventType type, float processTime);

private:
    // Constructor: initializes mutex and queue.
    SDS_Data();

    // Mutex for thread‑safe access.
    mutable osMutexId_t mutex;

    // Core DSP values.
    float azimuthDeg;
    float distance;
    float confidence;
    bool  detected;

    // Debug values (temporary, can be extended to arrays).
    float debugTime;
    float SRPPhatTime;
    float debugValue;
    float debugValue1;
    float debugValue2;
    float debugValue3;
    uint8_t id;
    uint32_t mode;
    uint32_t simulation;
    uint32_t syncTimeDifference;
	uint32_t micLoopCounter;
	uint32_t lcdLoopCounter;
	uint32_t srpLoopCounter;
	uint32_t errorFlag;
	uint32_t errorLen;
	uint8_t _errorBuffer[64];
	uint32_t errorCount;

    // RTOS event queue for asynchronous notifications.
    osMessageQueueId_t eventQueue;
};
