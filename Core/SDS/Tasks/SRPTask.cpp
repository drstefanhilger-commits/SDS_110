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

SRPTask::SRPTask()
    : TaskBase(8192, 50, osPriorityNormal),
      distEst(SDS_DIST_GAIN / 91.025f * 0.85f, 1e-3f)
{
}

void SRPTask::onStart()
{
    // SRP/DSP-Init bleibt wie bisher (falls vorhanden)

    // AI-Initialisierung
    if (!SDS_AIModel_Init(&ai)) {
        // optional: Fehlerflag in dm setzen
        dm.setAiInitError(true);
    } else {
        dm.setAiInitError(false);
    }
}

void SRPTask::runOnce() {
    switch (dm.getMode()) {
        case 1: detectHandler();   break;
        case 2: readHandler();     break;
        case 3: aiHandler();	   break;
        default: errorHandler();   break;
    }

    dm.setSrpLoopTime(execTimeCycles_);
    dm.setSrpTaskFreeStack(freeStackBytes_);
    dm.setSrpLoopCounter(loopNr_);
}

void SRPTask::detectHandler()
{
    UnifiedMicBuffer* micBuffer = micBufferManager.getReadableBuffer();
    if (!micBuffer) {
        return;
    }

    srp.beginAzimuthScan(micBuffer->data,
                         SDS_AZ_MIN,
                         SDS_AZ_MAX,
                         SDS_AZ_STEP);

    while (!srp.stepAzimuthScan()) {}

    dm.setAzimuth(srp.getResult());

    dr = distEst.process(das.makeFrame(micBuffer->data,
                                       SDS_AZ_MIN,
                                       SDS_AZ_MAX,
                                       SDS_AZ_STEP,
                                       1000));

    dm.setDistance(dr.distance_m);

    micBufferManager.markFree(micBuffer);

    USB_SendDetection(getTimestamp(), 0, srp.getResult(), dr.distance_m, dr.confidence);
}

void SRPTask::aiHandler()
{
    // 1. Mel-Features aus SDS_Data erzeugen (40 Werte)
    dm.computeMelFeatures(featureBuffer);   // diese Funktion musst du in SDS_Data passend zu FFT+MelSpectrogram implementieren
    // 2. AI-Inferenz
    if (SDS_AIModel_Run(&ai, featureBuffer, outputBuffer)) {

        const float drone      = outputBuffer[0];
        const float human      = outputBuffer[1];
        const float wind       = outputBuffer[2];
        const float background = outputBuffer[3];

        // 3. Ergebnisse ins Datenmodell schreiben
        dm.setAiDrone(drone);
        dm.setAiHuman(human);
        dm.setAiWind(wind);
        dm.setAiBackground(background);

        // 4. Optional: Detektionslogik
        if (drone > 0.7f && drone > human && drone > wind) {
            dm.setDroneDetected(true);
        } else {
            dm.setDroneDetected(false);
        }

    } else {

    	dm.pushErrorMessage("Fehler in SDS_AIModel_Run");
        dm.setAiRunError(true);
    }
}

void SRPTask::readHandler()
{
//    UnifiedMicBuffer* rb = micBufferManager.getReadableBuffer();
//    if (!rb) {
//        uint8_t rxBuffer[12] = {0xEE,0xFF,0xEE,0xFF,0xEE,0xFF,0xEE,0xFF,0xEE,0xFF,0xEE,0xFF};
//        memcpy(dm.getErrorBuffer(), rxBuffer, 12);
//        dm.setLcdLoopCounter(0);
//        dm.setSrpLoopCounter(0);
//        dm.setErrorCount(30);
//        return;
//    }
//
//    bool ok;
//    bool okSum = true;
//
//    for (uint32_t micNr = 0; micNr < 8; micNr++) {
//        for (uint32_t frameNr = 0; frameNr < 2; frameNr++) {
//            ok = USB_SendRead_Test();
//            okSum = okSum && ok;
//            if (!ok) { dm.setUsbErrorCount(dm.getUsbErrorCount() + 1); }
//            delay(400);
//            if (dm.getMode() != 3) { continue; }
//        }
//    }
//
//    micBufferManager.markFree(rb);
//
//    if (!okSum) {
//        uint8_t rxBuffer[12] = {0xAA,0xBB,0xAA,0xBB,0xAA,0xBB,0xAA,0xBB,0xAA,0xBB,0xAA,0xBB};
//        memcpy(dm.getErrorBuffer(), rxBuffer, 12);
//        dm.setLcdLoopCounter(0);
//        dm.setSrpLoopCounter(0);
//        dm.setErrorCount(30);
//    }
}

void SRPTask::errorHandler()
{
    // optional: zentrale Fehlerbehandlung
}

uint32_t SRPTask::getTimestamp()
{
    return HAL_GetTick();
}
