#include <FFTProcessor.hpp>
#include <cmath>

/**
 * @brief Konstruktor: Initialisiert Hann-Fenster und CMSIS-FFT.
 */
FFTProcessor::FFTProcessor()
{
    // Hann-Fenster erzeugen
    for (int i = 0; i < FFT_SIZE; i++)
    {
        window[i] = 0.5f - 0.5f * cosf(2.0f * M_PI * i / (FFT_SIZE - 1));
    }

    // CMSIS FFT initialisieren
    arm_rfft_fast_init_f32(&fft, FFT_SIZE);
}

/**
 * @brief Multipliziert das Eingangssignal mit dem Hann-Fenster.
 */
void FFTProcessor::applyWindow(float data[FFT_SIZE]) const
{
    for (int i = 0; i < FFT_SIZE; i++)
    {
        data[i] *= window[i];
    }
}

/**
 * @brief Führt FFT + Magnitude-Berechnung durch.
 */
void FFTProcessor::compute(const float input[FFT_SIZE],
                           float magnitude[FFT_BINS])
{
    float buffer[FFT_SIZE];
    float fft_out[FFT_SIZE];   // enthält Real+Imag

    // 1. Input kopieren
    for (int i = 0; i < FFT_SIZE; i++)
        buffer[i] = input[i];

    // 2. Windowing
    applyWindow(buffer);

    // 3. FFT
    arm_rfft_fast_f32(&fft, buffer, fft_out, 0);

    // 4. Magnitude berechnen
    magnitude[0] = fabsf(fft_out[0]);   // DC

    for (int k = 1; k < FFT_BINS - 1; k++)
    {
        float re = fft_out[k];
        float im = fft_out[FFT_SIZE - k];
        magnitude[k] = sqrtf(re * re + im * im);
    }

    magnitude[FFT_BINS - 1] = fabsf(fft_out[FFT_SIZE / 2]); // Nyquist
}
