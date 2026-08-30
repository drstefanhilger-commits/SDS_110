/*
 * SRPTask.hpp
 *
 * Mathematical / Physical Description
 * -----------------------------------
 * This task implements the SRP‑PHAT (Steered Response Power with Phase
 * Transform) algorithm used for acoustic source localization in a
 * microphone array. The algorithm estimates the direction and distance
 * of an acoustic emitter by evaluating the spatial energy distribution
 * over a predefined grid.
 *
 * Physical Model:
 * ---------------
 * A microphone array measures pressure signals p_i(t). After windowing
 * and FFT transformation, the SRP‑PHAT algorithm computes:
 *
 *      SRP(x) = Σ_i Σ_j  PHAT( cross_spectrum(i,j) , delay(i,j,x) )
 *
 * where:
 *   - x is a candidate position in the SRP grid
 *   - PHAT applies spectral whitening: PHAT(X) = X / |X|
 *   - delay(i,j,x) is the theoretical propagation delay between
 *     microphones i and j for a source at position x
 *
 * The grid point with the maximum SRP value corresponds to the most
 * likely source position.
 *
 * The SRPTask performs:
 *   1. Microphone buffer acquisition (SDS_MicrophoneBuffer)
 *   2. SRP‑PHAT evaluation over the spatial grid
 *   3. Distance estimation (SRP_DAS_Distance + DistanceEstimator)
 *   4. Publishing results into SDS_Data
 *
 * Timing Model:
 * -------------
 * The task is executed periodically with a fixed sampling period T:
 *
 *      T = delayMs / 1000 seconds
 *
 * ensuring deterministic DSP processing and stable localization output.
 *
 * Created on: Aug 6, 2026
 * Author: Stefan (310004)
 */

#pragma once
#include <USBDriver.hpp>
#include <vector>
#include <cstdint>
#include "TaskBase.hpp"

#include "Model.hpp"
#include "Algorithm.hpp"

#include "SDSUSBMicSender.hpp"
#include "DWTTimer.hpp"

class SRPTask : public TaskBase {
public:
    // Singleton instance — ensures only one SRP pipeline runs.
    static SRPTask& instance() { static SRPTask inst; return inst; }

protected:
    // Periodic DSP update — performs SRP‑PHAT and distance estimation.
    void runOnce() override;

    // Initialization hook — prepares buffers, timers, and DSP structures.
    void onStart() override;

private:
    // Constructor: initializes SRP grid, DSP modules, and buffer references.
    SRPTask();

    // Returns a high‑resolution timestamp using DWT cycle counter.
    uint32_t getTimestamp();

    // Handler for detecting
    void detectHandler();

    // Handler for calibrating
    void claibrateHandler();

    // Handler for reading sound samples and writing via USB
    void readHandler();

    // Error-Handler
    void errorHandler();

private:
    // SRP spatial grid (energy values for each candidate position).
    std::vector<float> srpGrid_;

    // DSP modules for SRP‑PHAT and distance estimation.
    SRPPhat srp;
    SRP_DAS_Distance das;
    DistanceEstimator distEst;

    // Microphone buffer manager (triple‑buffered DMA acquisition).
    SDS_MicrophoneBuffer& micBufferManager = SDS_MicrophoneBuffer::instance();

    // Global data model for publishing results.
    SDS_Data& dm = SDS_Data::instance();

    // Distance estimation result container.
    DistanceResult dr;

//    // Pointer to the current microphone buffer.
//    UnifiedMicBuffer* pMic;

    // USB sender for READ(3) mode
    SDS_USB_MicSender usbSender;

    // High‑resolution cycle timer (DWT).
    DWTTimer& dwt = DWTTimer::instance();
};
