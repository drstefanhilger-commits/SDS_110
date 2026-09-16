/*
 * Pre_Processor_118.hpp
 *
 * Vorverarbeitung 118 (Patent, Abschnitt 1, FIG. 2), in-place je Mikrofon:
 *   1. Bandpass 80 Hz – 8 kHz   : Butterworth 2. Ordnung HPF + LPF (Biquad DF2T, CMSIS)
 *   2. Adaptive Rauschunterdrückung : Rauschboden-Verfolgung (RMS-Minimum) und
 *                                     Wiener-artige Verstärkung pro Frame
 *   3. AGC                      : Frame-RMS auf AGC_TARGET_RMS regeln, Attack/Release
 *
 * Reihenfolge Bandpass -> NS -> AGC, damit AGC nicht auf Rauschen hochregelt.
 * Alle Zustände pro Kanal; das Frame ist frei von Blockgrenzen-Artefakten,
 * da die Biquad-Zustände über Frames hinweg erhalten bleiben.
 *
 * Migration aus SDS: nur SDS_BANDPASS_LOW/HIGH (800–3000 Hz) als Konstanten –
 * kein lauffähiger Code vorhanden. Grenzen jetzt aus SDS_110_Config (80–8000 Hz).
 */
#pragma once
#include "arm_math.h"
#include "SDS_110_Config.hpp"
#include "Microphone_Array_114.hpp"

namespace sds110 {

class Pre_Processor_118 {
public:
    void init();
    void process(MicFrame& frame);

    /// Diagnose: aktuelle AGC-Verstärkung / Rauschboden je Kanal
    float gain(uint32_t ch) const { return gain_[ch]; }
    float noiseFloor(uint32_t ch) const { return noiseRms_[ch]; }

    void enableBandpass(bool on) { bandpassOn_ = on; }
    void enableNoiseSuppression(bool on) { nsOn_ = on; }
    void enableAgc(bool on) { agcOn_ = on; }

private:
    void bandpass(uint32_t ch, float* x, uint32_t n);
    void noiseSuppress(uint32_t ch, float* x, uint32_t n);
    void agc(uint32_t ch, float* x, uint32_t n);
    static float rms(const float* x, uint32_t n);
    static void  designHighpass(float fc, float fs, float* coeffs);
    static void  designLowpass (float fc, float fs, float* coeffs);

    // Biquad-Kaskade: 2 Stufen (HPF, LPF) je Kanal, 5 Koeffizienten je Stufe
    float coeffs_[2 * 5];
    float state_[NUM_MICS][2 * 2];
    arm_biquad_cascade_df2T_instance_f32 iir_[NUM_MICS];

    float gain_[NUM_MICS];
    float noiseRms_[NUM_MICS];
    bool  bandpassOn_ = true;
    bool  nsOn_       = true;
    bool  agcOn_      = true;
};

} // namespace sds110
