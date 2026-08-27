/*
 * MicTask.cpp
 *
 * Mathematical / Physical Description
 * -----------------------------------
 * This task generates synthetic microphone array signals for validating
 * the SRP‑PHAT and distance‑estimation pipeline. Instead of acquiring
 * real acoustic data from the ADAU7118 + DMA subsystem, the MicTask
 * produces controlled far‑field test signals.
 *
 * Physical Model:
 * ---------------
 * A virtual acoustic source is defined by:
 *
 *      - trueAz   : azimuth angle (degrees)
 *      - trueDist : distance (meters)
 *
 * The source moves deterministically:
 *
 *      trueAz   += d_az
 *      trueDist += d_Dist   (when azimuth wraps around)
 *
 * This creates a predictable trajectory in polar coordinates:
 *
 *      r[k+1] = r[k] + d_Dist
 *      θ[k+1] = θ[k] + d_az
 *
 * The synthetic microphone frame is generated using:
 *
 *      UnitTestSignals::makeTestFrame360_farfield()
 *
 * which models far‑field propagation:
 *
 *      p_i(t) = A * sin( 2π f (t - τ_i) )
 *
 * with τ_i determined by array geometry and source direction.
 *
 * Timing Model:
 * -------------
 * The task runs periodically with sampling period:
 *
 *      T = delayMs / 1000 seconds
 *
 * ensuring deterministic synthetic signal generation for DSP testing.
 *
 * Created on: Aug 10, 2026
 * Author: Stefan (310004)
 */

#include <MicTask.hpp>

// ---------------------------------------------------------------------------
// Constructor: initializes base task with stack size and period.
// ---------------------------------------------------------------------------
MicTask::MicTask()
    : TaskBase(8192, 20, osPriorityNormal)
{
    // No dynamic allocation — deterministic initialization.
}

// ---------------------------------------------------------------------------
// Initialization hook — executed once before periodic processing.
// ---------------------------------------------------------------------------
void MicTask::onStart()
{
    // Simulation state could be initialized here if needed.
}

// ---------------------------------------------------------------------------
// Periodic update — generates synthetic microphone signals.
// ---------------------------------------------------------------------------
void MicTask::runOnce()
{
	switch (dm.getMode()) {
		case 1: detectHandler(); break;		//Mode Detect
		case 2: detectHandler(); break;		// Mode Read
		case 3: claibrateHandler(); break;
		default: errorHandler(); break;
	}
}

// Handler for sampling
void MicTask::detectHandler() {
	switch(dm.getSimulation()) {
		case 0: readMic(); break;
		case 1: simulateMic(); break;
		default: errorMic(); break;
	}
}

// Read Microphone
void MicTask::readMic() {

}

// Simulate Microphone
void MicTask::simulateMic() {
    // 1) Update virtual source azimuth and distance
    trueAz += d_az;

    if (trueAz >= 360.0f) {
        trueAz = 0.0f;

        // Increase distance when azimuth wraps around
        trueDist += d_Dist;

        if (trueDist >= 100.0f) {
            trueDist = 50.0f;   // Reset distance for cyclic testing
        }
    }

    // 2) Acquire a free microphone buffer
    MicBuffer* pMicBuffer = micBufferManager.getFreeBuffer();

    // Generate synthetic far‑field test frame
    UnitTestSignals::makeTestFrame360_farfield(
        trueAz,
        trueDist,
        0.01f,              // test frequency or phase increment
        pMicBuffer->data
    );

    // Mark buffer as readable for downstream DSP tasks
    micBufferManager.markReadable(pMicBuffer);

    // 3) Publish debug values into the data model
    dm.setDebugValue2(trueAz);
    dm.setDebugValue3(trueDist);

    // Simple loop counter
    dm.setMicLoopCounter(dm.getMicLoopCounter() + 1);
}

// Mic Error
void MicTask::errorMic() {

}

// Handler for calibrating
void MicTask::claibrateHandler() {

}

// Error-Handler
void MicTask::errorHandler() {

}
