/*
 * Pre_Processor_118.cpp
 */
#include "Pre_Processor_118.hpp"
#include <cmath>

namespace sds110 {

// ---------------------------------------------------------------- Design
// RBJ Audio-EQ-Cookbook, Q = 1/sqrt(2) (Butterworth 2. Ordnung)
// Koeffizientenlayout CMSIS: b0 b1 b2 a1 a2 (a1/a2 negiert)
void Pre_Processor_118::designHighpass(float fc, float fs, float* c)
{
    const float w0 = 2.0f * PI * fc / fs;
    const float cw = std::cos(w0), sw = std::sin(w0);
    const float alpha = sw / (2.0f * 0.70710678f);
    const float a0 = 1.0f + alpha;
    c[0] = ((1.0f + cw) * 0.5f) / a0;
    c[1] = (-(1.0f + cw)) / a0;
    c[2] = ((1.0f + cw) * 0.5f) / a0;
    c[3] = -((-2.0f * cw) / a0);
    c[4] = -((1.0f - alpha) / a0);
}

void Pre_Processor_118::designLowpass(float fc, float fs, float* c)
{
    const float w0 = 2.0f * PI * fc / fs;
    const float cw = std::cos(w0), sw = std::sin(w0);
    const float alpha = sw / (2.0f * 0.70710678f);
    const float a0 = 1.0f + alpha;
    c[0] = ((1.0f - cw) * 0.5f) / a0;
    c[1] = (1.0f - cw) / a0;
    c[2] = ((1.0f - cw) * 0.5f) / a0;
    c[3] = -((-2.0f * cw) / a0);
    c[4] = -((1.0f - alpha) / a0);
}

void Pre_Processor_118::init()
{
    designHighpass(BANDPASS_LO_HZ, static_cast<float>(SAMPLE_RATE_HZ), &coeffs_[0]);
    designLowpass (BANDPASS_HI_HZ, static_cast<float>(SAMPLE_RATE_HZ), &coeffs_[5]);

    for (uint32_t ch = 0; ch < NUM_MICS; ++ch) {
        arm_biquad_cascade_df2T_init_f32(&iir_[ch], 2, coeffs_, state_[ch]);
        gain_[ch]     = 1.0f;
        noiseRms_[ch] = 1e-3f;
        applied_[ch]  = 1.0f;
    }
}

// ---------------------------------------------------------------- Stufen
float Pre_Processor_118::rms(const float* x, uint32_t n)
{
    float r = 0.0f;
    arm_rms_f32(x, n, &r);
    return r;
}

void Pre_Processor_118::bandpass(uint32_t ch, float* x, uint32_t n)
{
    arm_biquad_cascade_df2T_f32(&iir_[ch], x, x, n);
}

float Pre_Processor_118::noiseSuppress(uint32_t ch, float* x, uint32_t n)
{
    // Rauschboden: schnelles Absenken, langsames Anheben (Minimum-Tracking)
    const float r = rms(x, n);
    float& nf = noiseRms_[ch];
    if (r < nf) nf = r;
    else        nf += NS_FLOOR_ALPHA * (r - nf);

    // Wiener-artige Frame-Verstärkung: g = 1 - (nf/r)^2, begrenzt auf NS_MAX_ATTEN
    if (r <= 1e-9f) return 1.0f;
    const float snr = (r * r) / (nf * nf + 1e-12f);
    float g = 1.0f - 1.0f / snr;
    if (g < NS_MAX_ATTEN) g = NS_MAX_ATTEN;
    if (g < 1.0f) { arm_scale_f32(x, g, x, n); return g; }
    return 1.0f;
}

float Pre_Processor_118::agc(uint32_t ch, float* x, uint32_t n)
{
    const float r = rms(x, n);
    if (r <= 1e-9f) return 1.0f;

    float target = AGC_TARGET_RMS / r;
    if (target > AGC_MAX_GAIN) target = AGC_MAX_GAIN;
    if (target < AGC_MIN_GAIN) target = AGC_MIN_GAIN;

    float& g = gain_[ch];
    const float a = (target < g) ? AGC_ATTACK : AGC_RELEASE;   // Pegel steigt -> schnell runterregeln
    g += a * (target - g);

    arm_scale_f32(x, g, x, n);
    return g;
}

// ---------------------------------------------------------------- Frame
void Pre_Processor_118::process(MicFrame& frame)
{
    for (uint32_t ch = 0; ch < NUM_MICS; ++ch) {
        float* x = frame.data[ch];
        float g = 1.0f;
        if (bandpassOn_) bandpass(ch, x, FRAME_SAMPLES);
        if (nsOn_)       g *= noiseSuppress(ch, x, FRAME_SAMPLES);
        if (agcOn_)      g *= agc(ch, x, FRAME_SAMPLES);
        applied_[ch] = g;
    }
}

} // namespace sds110
