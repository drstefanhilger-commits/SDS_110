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

extern "C" {
#include "SDS_AIModel.h"
}

#include "SDSUSBMicSender.hpp"
//#include <DWTTimer1.hppp>

class SRPTask : public TaskBase {
public:
    static SRPTask& instance() { static SRPTask inst; return inst; }

protected:
    void runOnce() override;
    void onStart() override;

    DWTTimer& dwt = DWTTimer::instance();

private:
    SRPTask();

    uint32_t getTimestamp();
    void detectHandler();
    void aiHandler();
    void readHandler();
    void errorHandler();

private:
    // ML Instance
    SDS_AIModel ai;

    // 40 Mel-Bänder → 4 Klassen
    static constexpr int FEATURE_DIM = 40;
    static constexpr int NUM_CLASSES = 4;

    float featureBuffer[FEATURE_DIM];
    float outputBuffer[NUM_CLASSES];

    std::vector<float> srpGrid_;

    SRPPhat srp;
    SRP_DAS_Distance das;
    DistanceEstimator distEst;

    SDS_MicrophoneBuffer& micBufferManager = SDS_MicrophoneBuffer::instance();
    SDS_Data& dm = SDS_Data::instance();

    DistanceResult dr;
    SDS_USB_MicSender usbSender;

    uint32_t loop = 0;
};
