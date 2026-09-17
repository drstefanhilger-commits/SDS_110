/*
 * HBD.hpp – Harmonic Band Detector (klassische Routine, kein trainiertes Modell)
 *
 * Übernommen aus der HBD-Vorlage (STM32F746 @ 48 kHz), angepasst an SDS_110:
 *  - arbeitet auf dem Betragsspektrum |X(t,k)| aus 122 (NUM_BINS = N_FFT/2+1 statt N_FFT)
 *  - dB-Werte je Bin werden einmal pro Frame berechnet (magDb_), nicht mehrfach
 *  - Parameter/Zustand bleiben Strukturen wie in der Vorlage, Werte über HBD_InitParams_48k()
 *
 * Ablauf pro Frame: Noise-Floor-Nachführung -> f0-Suche 80..350 Hz -> SNR je Harmonischer
 * -> Konsistenzhistorie -> gewichteter Score -> Entscheidung.
 */
#pragma once
#include <cstdint>
#include "SDS_110_Config.hpp"

namespace sds110 {

struct HBD_FftParams        { uint32_t sampleRateHz; uint16_t fftSize; uint16_t hopSize; float windowGain; };
struct HBD_HarmonicBand     { float fundamentalMinHz; float fundamentalMaxHz; uint8_t numHarmonics; float bandHalfWidthHz; };
struct HBD_NoiseFloorParams { float smoothingFactor; float minFloorDb; float maxFloorDb; };
struct HBD_SnrThresholds    { float globalSnrDb; float perBandSnrDb[8]; uint8_t numBandsUsed; };
struct HBD_ConsistencyParams{ float minConsistency; uint8_t minStableHarmonics; uint8_t temporalWindow; };
struct HBD_DecisionParams   { float bandWeights[8]; uint8_t numWeightsUsed; float finalScoreThreshold; };

struct HBD_Params {
    HBD_FftParams         fft;
    HBD_HarmonicBand      harmonic;
    HBD_NoiseFloorParams  noiseFloor;
    HBD_SnrThresholds     snr;
    HBD_ConsistencyParams consistency;
    HBD_DecisionParams    decision;
};

constexpr uint8_t HBD_MAX_HARMONICS = 8;

struct HBD_State {
    float   noiseFloorDb[NUM_BINS];              // Noise-Floor pro Bin (dB)
    float   magDb[NUM_BINS];                     // |X| in dB, aktuelles Frame
    float   lastBandSnr[HBD_MAX_HARMONICS];      // SNR je Harmonischer (dB)
    float   consistencyHistory[HBD_MAX_HARMONICS];
    uint16_t harmonicBin[HBD_MAX_HARMONICS];     // Mittenbin je Harmonischer (0 = ungültig)
    float   f0Hz;
    float   globalSnrAvgDb;
    float   score;
    uint8_t stableCount;
    uint8_t consistentBands;
    bool    droneDetected;
};

void HBD_InitParams_48k(HBD_Params& p);
void HBD_InitState(HBD_State& s, const HBD_Params& p);

/// mag: Betragsspektrum, NUM_BINS Werte. Rückgabe: Drohne erkannt (Score/Konsistenz/SNR).
bool HBD_ProcessFrame(const HBD_Params& p, HBD_State& s, const float* mag);

} // namespace sds110
