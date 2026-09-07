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

#include "SDS_Data.hpp"

// ---------------------------------------------------------------------------
// Singleton instance
// ---------------------------------------------------------------------------
SDS_Data& SDS_Data::instance()
{
    static SDS_Data instance;
    return instance;
}

// ---------------------------------------------------------------------------
// Constructor: initializes mutex and event queue.
// ---------------------------------------------------------------------------
SDS_Data::SDS_Data()
    : mutex(nullptr),
      azimuthDeg(0.0f),
      distance(0.0f),
      confidence(0.0f),
      detected(false),
      debugTime(0.0f),
      debugValue(0.0f),
      debugValue1(0.0f),
      debugValue2(0.0f),
      debugValue3(0.0f),
	  id(0),
	  mode(1),
	  simulation(1),
	  syncTimeDifference(0),
	  micLoopCounter(0),
	  lcdLoopCounter(0),
	  srpLoopCounter(0),
	  errorFlag(0),
	  errorLen(0),
	  errorCount(0),
	  usbErrorCount(0),
      eventQueue(nullptr)
{
    // Create mutex for thread‑safe access
    mutex = osMutexNew(nullptr);

    // Create event queue (16 messages, each SDS_DataEvent)
    const osMessageQueueAttr_t qAttr = { };
    eventQueue = osMessageQueueNew(16, sizeof(SDS_DataEvent), &qAttr);
}

// ---------------------------------------------------------------------------
// Push event into RTOS queue
// ---------------------------------------------------------------------------
void SDS_Data::pushEvent(SDS_DataEventType type, float processTime)
{
    if (!eventQueue) {
        return;
    }

    SDS_DataEvent evt;
    evt.type        = type;
    evt.processTime = processTime;

    (void)osMessageQueuePut(eventQueue, &evt, 0, 0);
}

// ---------------------------------------------------------------------------
// Write API — DSP tasks update shared state
// ---------------------------------------------------------------------------
void SDS_Data::setAzimuth(float az)
{
    osMutexAcquire(mutex, osWaitForever);
    azimuthDeg = az;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::SRP_UPDATE, 0.0f);
}

void SDS_Data::setDistance(float d)
{
    osMutexAcquire(mutex, osWaitForever);
    distance = d;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::SRP_UPDATE, 0.0f);
}

void SDS_Data::setConfidence(float c)
{
    osMutexAcquire(mutex, osWaitForever);
    confidence = c;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::SRP_UPDATE, 0.0f);
}

void SDS_Data::setDetected(bool d)
{
    osMutexAcquire(mutex, osWaitForever);
    detected = d;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::SRP_UPDATE, 0.0f);
}

// ---------------------------------------------------------------------------
// Debug values
// ---------------------------------------------------------------------------
void SDS_Data::setDebugTime(float ms)
{
    osMutexAcquire(mutex, osWaitForever);
    debugTime = ms;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::DEBUG_UPDATE, 0.0f);
}

void SDS_Data::setSRPPhatTime(float t)
{
    osMutexAcquire(mutex, osWaitForever);
    SRPPhatTime = t;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::DEBUG_UPDATE, 0.0f);
}

void SDS_Data::setDebugValue(float val)
{
    osMutexAcquire(mutex, osWaitForever);
    debugValue = val;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::DEBUG_UPDATE, 0.0f);
}

void SDS_Data::setDebugValue1(float val)
{
    osMutexAcquire(mutex, osWaitForever);
    debugValue1 = val;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::DEBUG_UPDATE, 0.0f);
}

void SDS_Data::setDebugValue2(float val)
{
    osMutexAcquire(mutex, osWaitForever);
    debugValue2 = val;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::DEBUG_UPDATE, 0.0f);
}

void SDS_Data::setDebugValue3(float val)
{
    osMutexAcquire(mutex, osWaitForever);
    debugValue3 = val;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::DEBUG_UPDATE, 0.0f);
}

void SDS_Data::setId(uint8_t val)
{
    osMutexAcquire(mutex, osWaitForever);
    id = val;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::DEBUG_UPDATE, 0.0f);
}

void SDS_Data::setMode(uint32_t val)
{
    osMutexAcquire(mutex, osWaitForever);
    mode = val;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::DEBUG_UPDATE, 0.0f);
}

void SDS_Data::setSimulation(uint32_t val)
{
    osMutexAcquire(mutex, osWaitForever);
    simulation = val;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::DEBUG_UPDATE, 0.0f);
}

void SDS_Data::setSyncTimeDifference(uint32_t val)
{
    osMutexAcquire(mutex, osWaitForever);
    syncTimeDifference = val;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::DEBUG_UPDATE, 0.0f);
}

void SDS_Data::setMicLoopCounter(uint32_t val)
{
    osMutexAcquire(mutex, osWaitForever);
    micLoopCounter = val;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::DEBUG_UPDATE, 0.0f);
}

void SDS_Data::setLcdLoopCounter(uint32_t val)
{
    osMutexAcquire(mutex, osWaitForever);
    lcdLoopCounter = val;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::DEBUG_UPDATE, 0.0f);
}

void SDS_Data::setSrpLoopCounter(uint32_t val)
{
    osMutexAcquire(mutex, osWaitForever);
    srpLoopCounter = val;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::DEBUG_UPDATE, 0.0f);
}

void SDS_Data::setErrorFlag(uint32_t val)
{
    osMutexAcquire(mutex, osWaitForever);
    errorFlag = val;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::DEBUG_UPDATE, 0.0f);
}

void SDS_Data::setErrorLen(uint32_t val)
{
    osMutexAcquire(mutex, osWaitForever);
    errorLen = val;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::DEBUG_UPDATE, 0.0f);
}

void SDS_Data::setErrorCount(uint32_t val)
{
    osMutexAcquire(mutex, osWaitForever);
    errorCount = val;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::DEBUG_UPDATE, 0.0f);
}

void SDS_Data::setUsbErrorCount(uint32_t val)
{
    osMutexAcquire(mutex, osWaitForever);
    usbErrorCount = val;
    osMutexRelease(mutex);

//    pushEvent(SDS_DataEventType::DEBUG_UPDATE, 0.0f);
}

// ---------------------------------------------------------------------------
// Read API — DisplayManager reads shared state
// ---------------------------------------------------------------------------
float SDS_Data::getAzimuth() const
{
    osMutexAcquire(mutex, osWaitForever);
    float v = azimuthDeg;
    osMutexRelease(mutex);
    return v;
}

float SDS_Data::getDistance() const
{
    osMutexAcquire(mutex, osWaitForever);
    float v = distance;
    osMutexRelease(mutex);
    return v;
}

float SDS_Data::getConfidence() const
{
    osMutexAcquire(mutex, osWaitForever);
    float v = confidence;
    osMutexRelease(mutex);
    return v;
}

bool SDS_Data::getDetected() const
{
    osMutexAcquire(mutex, osWaitForever);
    bool v = detected;
    osMutexRelease(mutex);
    return v;
}

float SDS_Data::getSRPPhatTime() const
{
    osMutexAcquire(mutex, osWaitForever);
    float t = SRPPhatTime;
    osMutexRelease(mutex);
    return t;
}

float SDS_Data::getDebugTime() const
{
    osMutexAcquire(mutex, osWaitForever);
    float v = debugTime;
    osMutexRelease(mutex);
    return v;
}

float SDS_Data::getDebugValue() const
{
    osMutexAcquire(mutex, osWaitForever);
    float v = debugValue;
    osMutexRelease(mutex);
    return v;
}

float SDS_Data::getDebugValue1() const
{
    osMutexAcquire(mutex, osWaitForever);
    float v = debugValue1;
    osMutexRelease(mutex);
    return v;
}

float SDS_Data::getDebugValue2() const
{
    osMutexAcquire(mutex, osWaitForever);
    float v = debugValue2;
    osMutexRelease(mutex);
    return v;
}

float SDS_Data::getDebugValue3() const
{
    osMutexAcquire(mutex, osWaitForever);
    float v = debugValue3;
    osMutexRelease(mutex);
    return v;
}

uint8_t SDS_Data::getId() const
{
    osMutexAcquire(mutex, osWaitForever);
    uint8_t val = id;
    osMutexRelease(mutex);
    return val;
}

uint32_t SDS_Data::getMode() const
{
    osMutexAcquire(mutex, osWaitForever);
    uint32_t val = mode;
    osMutexRelease(mutex);
    return val;
}

uint32_t SDS_Data::getSimulation() const
{
    osMutexAcquire(mutex, osWaitForever);
    uint32_t val = simulation;
    osMutexRelease(mutex);
    return val;
}

uint32_t SDS_Data::getSyncTimeDifference() const
{
    osMutexAcquire(mutex, osWaitForever);
    uint32_t val = syncTimeDifference;
    osMutexRelease(mutex);
    return val;
}

uint32_t SDS_Data::getMicLoopCounter() const
{
    osMutexAcquire(mutex, osWaitForever);
    uint32_t val = micLoopCounter;
    osMutexRelease(mutex);
    return val;
}

uint32_t SDS_Data::getLcdLoopCounter() const
{
    osMutexAcquire(mutex, osWaitForever);
    uint32_t val = lcdLoopCounter;
    osMutexRelease(mutex);
    return val;
}

uint32_t SDS_Data::getSrpLoopCounter() const
{
    osMutexAcquire(mutex, osWaitForever);
    uint32_t val = srpLoopCounter;
    osMutexRelease(mutex);
    return val;
}

uint32_t SDS_Data::getErrorFlag() const
{
    osMutexAcquire(mutex, osWaitForever);
    uint32_t val = errorFlag;
    osMutexRelease(mutex);
    return val;
}

uint32_t SDS_Data::getErrorLen() const
{
    osMutexAcquire(mutex, osWaitForever);
    uint32_t val = errorLen;
    osMutexRelease(mutex);
    return val;
}

uint8_t* SDS_Data::getErrorBuffer()
{
    osMutexAcquire(mutex, osWaitForever);
    uint8_t* ptr = &_errorBuffer[0];
    osMutexRelease(mutex);
    return ptr;
}

uint32_t SDS_Data::getErrorCount()
{
    osMutexAcquire(mutex, osWaitForever);
    uint32_t val = errorCount;
    osMutexRelease(mutex);
    return val;
}

uint32_t SDS_Data::getUsbErrorCount()
{
    osMutexAcquire(mutex, osWaitForever);
    uint32_t val = usbErrorCount;
    osMutexRelease(mutex);
    return val;
}

