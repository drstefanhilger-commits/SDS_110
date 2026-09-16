#pragma once

#include <cstdint>
#include "arm_math.h"        // CMSIS-DSP FFT + Math
#include "arm_const_structs.h"

/**
 * @brief Führt eine 256-Punkt FFT durch und berechnet das Magnitude-Spektrum.
 *
 * Diese Klasse kapselt:
 *  - Hann-Fenster
 *  - CMSIS-RFFT-Instanz
 *  - Magnitude-Berechnung
 *
 * Sie ist vollständig statisch (keine dynamische Allokation)
 * und damit echtzeitfähig für SDS.
 */
class FFTProcessor
{
public:
    static constexpr int FFT_SIZE = 256;
    static constexpr int FFT_BINS = FFT_SIZE / 2 + 1;

    FFTProcessor();

    /**
     * @brief Berechnet FFT + Magnitude.
     *
     * @param input     256-Sample Zeitbereichssignal
     * @param magnitude 129 FFT-Magnitude-Werte
     */
    void compute(const float input[FFT_SIZE],
                 float magnitude[FFT_BINS]);

private:
    float window[FFT_SIZE];          ///< Hann-Fenster
    arm_rfft_fast_instance_f32 fft;  ///< CMSIS FFT-Instanz

    void applyWindow(float data[FFT_SIZE]) const;
};
