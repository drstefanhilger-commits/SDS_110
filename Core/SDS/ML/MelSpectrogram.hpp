#pragma once

#include "MelFilterbank.hpp"
#include "arm_math.h"

/**
 * @brief Berechnet Mel-Spektrum aus FFT-Magnitude.
 *
 * Schritte:
 *  1. Dot-Product mit Mel-Filterbank (CMSIS-optimiert)
 *  2. Log-Mel Transformation
 */
class MelSpectrogram
{
public:
    MelSpectrogram(const MelFilterbank& fb);

    /**
     * @brief Berechnet Mel-Spektrum (40 Werte).
     *
     * @param fft_mag     129 FFT-Magnitude-Werte
     * @param mel_out     40 Mel-Band-Energien
     */
    void compute(const float fft_mag[MelFilterbank::FFT_BINS],
                 float mel_out[MelFilterbank::MEL_BANDS]) const;

private:
    const MelFilterbank& fb; ///< Referenz auf Filterbank
};
