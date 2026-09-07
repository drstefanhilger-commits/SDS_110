/*
 * SRPTask.cpp
 *
 * Mathematical / Physical Description
 * -----------------------------------
 * This file implements the runtime behavior of the SRP‑PHAT acoustic
 * localization pipeline. The SRPTask periodically processes microphone
 * array data to estimate:
 *
 *   1. Azimuth (angle of arrival)
 *   2. Distance to the acoustic source
 *
 * using SRP‑PHAT and DAS‑based distance estimation.
 *
 * SRP‑PHAT Model:
 * ---------------
 * Microphones measure pressure signals p_i(t). After FFT transformation,
 * the cross‑spectra between microphone pairs are computed:
 *
 *      C_ij(f) = P_i(f) * conj(P_j(f))
 *
 * PHAT whitening is applied:
 *
 *      PHAT(C_ij(f)) = C_ij(f) / |C_ij(f)|
 *
 * For each candidate azimuth θ in the scan range:
 *
 *      SRP(θ) = Σ_i Σ_j  PHAT( C_ij(f), τ_ij(θ) )
 *
 * where τ_ij(θ) is the theoretical propagation delay for a source at
 * azimuth θ. The azimuth with maximum SRP value is the estimated angle.
 *
 * Distance Estimation:
 * --------------------
 * The DAS (Delay‑And‑Sum) frame is constructed and passed to the
 * DistanceEstimator, which applies a calibrated gain model:
 *
 *      d = f( DAS_energy , gain , threshold )
 *
 * Timing Model:
 * -------------
 * The task runs periodically with sampling period:
 *
 *      T = delayMs / 1000 seconds
 *
 * ensuring deterministic DSP processing and stable localization output.
 *
 * Created on: Aug 6, 2026
 * Author: Stefan (310004)
 */

#include "SRPTask.hpp"

// Temporary global buffer (to be moved into Model)
extern SRPBuffers g_srp;

extern SDS_USB_MicSender usbSender;

// ---------------------------------------------------------------------------
// Constructor: initializes base task and distance estimator.
// ---------------------------------------------------------------------------
SRPTask::SRPTask()
    : TaskBase(8192, 50, osPriorityNormal),
      distEst(SDS_DIST_GAIN / 91.025f * 0.85f, 1e-3f)
{
    // No dynamic allocation here — DSP modules are constructed in-place.
}

// ---------------------------------------------------------------------------
// Initialization hook — executed once before periodic processing.
// ---------------------------------------------------------------------------
void SRPTask::onStart()
{
    // Initialization of DSP modules or buffers could be placed here.
}

// ---------------------------------------------------------------------------
// Periodic DSP update — performs SRP‑PHAT and distance estimation.
// ---------------------------------------------------------------------------
void SRPTask::runOnce() {
	switch (dm.getMode()) {
		case 1: detectHandler(); break;
		case 2:	readHandler(); break;
		case 3: claibrateHandler(); break;
		default: errorHandler(); break;
	}

	dm.setSrpLoopCounter(dm.getSrpLoopCounter() + 1);
}

// Handler for detecting
void SRPTask::detectHandler()
{
    // 0) Start high‑resolution timing (DWT cycle counter)
    dwt.getStartTime();

    // 1) Acquire a readable microphone buffer (triple‑buffered DMA)
    UnifiedMicBuffer* micBuffer = micBufferManager.getReadableBuffer();

    // 2) Perform SRP‑PHAT azimuth scan
    srp.beginAzimuthScan(micBuffer->data,
                         SDS_AZ_MIN,
                         SDS_AZ_MAX,
                         SDS_AZ_STEP);

    // Step through the entire azimuth grid
    while (!srp.stepAzimuthScan()) {}

    // Publish azimuth result
    dm.setAzimuth(srp.getResult());

    // Optional calibration/filtering:
    // float rawAz = srp.getResult();
    // float az    = srp.calibrateAzimuth(rawAz);
    // az          = srp.filterAzimuth(az);

    // 3) Distance estimation using DAS frame
    dr = distEst.process(das.makeFrame(micBuffer->data,
                         SDS_AZ_MIN,
                         SDS_AZ_MAX,
                         SDS_AZ_STEP,
                         1000));

    dm.setDistance(dr.distance_m);

    // 4) Release microphone buffer for reuse
    micBufferManager.markFree(micBuffer);

    // 5) Send USB Message
    USB_SendDetection(getTimestamp(), 0, srp.getResult(), dr.distance_m, dr.confidence);

    // 6) Stop timing and publish DSP execution time
    dwt.getStopTime();
    dm.setSRPPhatTime(dwt.getTimeDifferenceMs());

    // Optional Logger
    // Logger::instance().write("angle=%.2f\n", dm.getAzimuth());
}

// Handler for calibrating
void SRPTask::claibrateHandler() {

}


//SDS_MsgRead msgXXX;

// Handler for reading sound samples and writing via USB
void SRPTask::readHandler() {

    // Fertigen Buffer holen
	UnifiedMicBuffer* rb = micBufferManager.getReadableBuffer();
    if (!rb) {
    	uint8_t rxBuffer[12] = {0xEE, 0xFF, 0xEE, 0xFF, 0xEE, 0xFF, 0xEE, 0xFF, 0xEE, 0xFF, 0xEE, 0xFF};
    	memcpy(dm.getErrorBuffer(), rxBuffer, 12);
    	dm.setLcdLoopCounter(0);
    	dm.setSrpLoopCounter(0);
    	dm.setErrorCount(30);	// ~10 sec
        return; // kein fertiger Block → nichts zu tun
    }

	bool ok;
	bool okSum = true;

	for (uint32_t micNr = 0; micNr<8; micNr++) {
		for (uint32_t frameNr = 0; frameNr<2; frameNr++) {
//			ok = USB_SendRead(getTimestamp(), micNr, frameNr, rb);
			ok = USB_SendRead_Test();
			okSum = okSum && ok;
			if (!ok) {dm.setUsbErrorCount(dm.getUsbErrorCount()+1);}
			delay(400);
			if (dm.getMode() != 3) {continue;}
		}
	}

    // Buffer freigeben
    micBufferManager.markFree(rb);

    // Optional: Fehlerbehandlung
    if (!okSum) {
    	uint8_t rxBuffer[12] = {0xAA, 0xBB, 0xAA, 0xBB, 0xAA, 0xBB, 0xAA, 0xBB, 0xAA, 0xBB, 0xAA, 0xBB};
    	memcpy(dm.getErrorBuffer(), rxBuffer, 12);
    	dm.setLcdLoopCounter(0);
    	dm.setSrpLoopCounter(0);
    	dm.setErrorCount(30);	// ~10 sec
        // USB überlastet oder blockiert
        // → keine SRP-Pipeline, nur Logging
    }

//    delay(200); //Notwend, da Ueberlauf
}

// Error-Handler
void SRPTask::errorHandler() {

}

// ---------------------------------------------------------------------------
// Returns a millisecond timestamp using HAL tick counter.
// ---------------------------------------------------------------------------
uint32_t SRPTask::getTimestamp()
{
    return HAL_GetTick();
}
