#include "MelSpectrogram.hpp"

/**
 * @brief Konstruktor speichert Referenz auf Filterbank.
 */
MelSpectrogram::MelSpectrogram(const MelFilterbank& fb)
    : fb(fb)
{
}

/**
 * @brief Berechnet Mel-Spektrum mit CMSIS-DSP Dot-Product.
 */
void MelSpectrogram::compute(const float fft_mag[MelFilterbank::FFT_BINS],
                             float mel_out[MelFilterbank::MEL_BANDS]) const
{
    for (int b = 0; b < MelFilterbank::MEL_BANDS; b++)
    {
        // Dot-Product: extrem schnell auf STM32F7
        arm_dot_prod_f32(fb.band(b), fft_mag,
                         MelFilterbank::FFT_BINS,
                         &mel_out[b]);

        // Log-Mel (wichtig für CNN)
        mel_out[b] = logf(1e-6f + mel_out[b]);
    }
}
