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

#include <FFTProcessor.hpp>
#include "SDS_Data.hpp"
#include "MelFilterbank.hpp"
#include "MelSpectrogram.hpp"
#include "SDS_MicrophoneBuffer.hpp"

// Singleton instance
SDS_Data& SDS_Data::instance()
{
    static SDS_Data d;
    return d;
}

// Constructor
SDS_Data::SDS_Data()    :
		// AI internal data
		aiDrone(0.0f),
		aiHuman(0.0f),
		aiWind(0.0f),
		aiBackground(0.0f),
		aiInitError(false),
		aiRunError(false),
		droneDetected(false),

		// Detect values
		azimuthDeg(0.0f),
		distance(0.0f),
		confidence(0.0f),
		detected(false),

		// System Status
		id(0),
		mode(1),
		simulation(1),
		syncTimeDifference(0),

		// SRP Task
		srpTaskFreeStack(0),
		srpLoopTime(0.0f),
		srpLoopCounter(0),

		// LCD Task
		lcdTaskFreeStack(0),
		lcdLoopTime(0.0f),
		lcdLoopCounter(0),

		// MIC Task
		micTaskFreeStack(0),
		micLoopTime(0.0f),
		micLoopCounter(0),

		// USB Task
		usbTaskFreeStack(0),

		// Debug
		debugTime(0.0f),
		debugValue(0.0f),
		debugValue1(0.0f),
		debugValue2(0.0f),
		debugValue3(0.0f),

		// Error
		errorFlag(0),
		errorLen(0),
		errorCount(0),
		usbErrorCount(0)
{
    memset(_errorBuffer, 0, sizeof(_errorBuffer));

    // Mutex erzeugen
    const osMutexAttr_t mutexAttr = {
        .name = "SDS_DataMutex"
    };
    mutex = osMutexNew(&mutexAttr);

    // Event queue erzeugen
    const osMessageQueueAttr_t queueAttr = {
        .name = "SDS_EventQueue"
    };
    eventQueue = osMessageQueueNew(16, sizeof(SDS_DataEvent), &queueAttr);

    errorMsgQueue = osMessageQueueNew(
        8,                          // Anzahl der Nachrichten
        sizeof(SDS_ErrorMessage),   // Größe einer Nachricht
        nullptr                     // Default attributes
    );

    // AI DSP modules
    fft    = new FFTProcessor();
    melFB  = new MelFilterbank();
    melSpec = new MelSpectrogram(*melFB);
}

void SDS_Data::computeMelFeatures(float out[40])
{
    // 1. Mikrofonblock holen (OHNE SDS_Data-Mutex!)
    UnifiedMicBuffer* rb = SDS_MicrophoneBuffer::instance().getReadableBuffer();
    if (!rb) {
        for (int i = 0; i < 40; i++) { out[i] = 0.0f; }

        return;
    }

    // 2. EINEN Kanal auswählen (Mic 0)
    const float* mic0 = rb->data[0];

    // 3. FFT berechnen
    fft->compute(mic0, fftMag);

    // 4. Mel-Spektrum berechnen
    melSpec->compute(fftMag, melOut);

    // 5. Ausgabe kopieren
    for (int i = 0; i < 40; i++) { out[i] = melOut[i]; }

    // 6. Buffer freigeben
    SDS_MicrophoneBuffer::instance().markFree(rb);
}

// -----------------------------------------------------------------------------
// Error-Message Event Queue
// -----------------------------------------------------------------------------
void SDS_Data::pushErrorMessage(const char* msg)
{
    SDS_ErrorMessage em{};
    // Sicher kopieren, max. 31 Zeichen + Nullterminator
    strncpy(em.text, msg, sizeof(em.text) - 1);
    // Non-blocking send
    osMessageQueuePut(errorMsgQueue, &em, 0, 0);
}

bool SDS_Data::popErrorMessage(SDS_ErrorMessage& out)
{
    if (osMessageQueueGet(errorMsgQueue, &out, nullptr, 0) == osOK)
        return true;

    return false;
}



// -----------------------------------------------------------------------------
// Event Queue
// -----------------------------------------------------------------------------
void SDS_Data::pushEvent(SDS_DataEventType type, float processTime)
{
    SDS_DataEvent evt;
    evt.type = type;
    evt.processTime = processTime;

    osMessageQueuePut(eventQueue, &evt, 0, 0);
}
