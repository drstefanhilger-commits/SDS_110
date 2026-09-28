/*
 * Signal_Simulator.cpp
 */
#include "Infrastructure/Utils/DspOptimize.hpp"   // zuerst: -O2 auf dem Board
#include "Signal_Simulator.hpp"
#include <cmath>
#include <cstring>

namespace sds110 {

Signal_Simulator& Signal_Simulator::instance() { static Signal_Simulator inst; return inst; }

void Signal_Simulator::init(const SimParams& p)
{
    p_ = p;
    primed_ = false;
    for (uint32_t h = 0; h < MAX_HARM; ++h) { oscRe_[h] = 1.0f; oscIm_[h] = 0.0f; }   // Phase 0
    amRe_ = 1.0f; amIm_ = 0.0f;
    std::memset(pinkState_, 0, sizeof(pinkState_));
}

float Signal_Simulator::noise()
{
    // ~N(0,1) als Summe von 4 Gleichverteilten (Irwin-Hall, Varianz 1, Grenzen ±3,46σ).
    // Vorher Box-Muller (log, sqrt, cos je Sample und Mikrofon, 12 288 Aufrufe je Hop).
    float sum = 0.0f;
    for (int i = 0; i < 4; ++i) {
        rng_ = rng_ * 1664525u + 1013904223u;             // LCG, obere 24 Bit
        sum += static_cast<float>(rng_ >> 8) * (1.0f / 16777216.0f);
    }
    return (sum - 2.0f) * 1.7320508f;                     // sqrt(12/4)
}

void Signal_Simulator::updateOscillators()
{
    constexpr float twoPi = 6.2831853f;
    const float fs = static_cast<float>(SAMPLE_RATE_HZ);
    for (uint32_t h = 0; h < MAX_HARM; ++h) {
        const float d = twoPi * p_.f0_hz * static_cast<float>(h + 1) / fs;
        stepRe_[h] = std::cos(d); stepIm_[h] = std::sin(d);
        const float r = 1.0f / std::sqrt(oscRe_[h] * oscRe_[h] + oscIm_[h] * oscIm_[h]);
        oscRe_[h] *= r; oscIm_[h] *= r;                   // Rundungsdrift je Hop zurücksetzen
    }
    const float d = twoPi * p_.bpf_mod_hz / fs;
    amStepRe_ = std::cos(d); amStepIm_ = std::sin(d);
    const float r = 1.0f / std::sqrt(amRe_ * amRe_ + amIm_ * amIm_);
    amRe_ *= r; amIm_ *= r;
}

void Signal_Simulator::advanceSweep()
{
    p_.azimuth_deg += p_.sweep_az_step;
    if (p_.azimuth_deg >= 360.0f) {
        p_.azimuth_deg -= 360.0f;
        p_.distance_m += p_.sweep_dist_step;
        if (p_.distance_m > 100.0f) p_.distance_m = 20.0f;
    }
}

// Ein Quellsample (phasenkontinuierlich), Amplitude ohne 1/r
float Signal_Simulator::sourceSample()
{
    // Zeiger um einen Schritt drehen; Im = sin(Phase) vor dem Schritt
    auto rotate = [](float& re, float& im, float sr, float si) {
        const float r = re * sr - im * si; im = re * si + im * sr; re = r;
    };
    float s = 0.0f;
    switch (p_.scenario) {
    case SimScenario::DroneSweep:
    case SimScenario::DroneStatic: {
        // Harmonische mit fallender Amplitude (1/h) und Blattpass-AM
        const float am = 1.0f + 0.5f * amIm_;
        rotate(amRe_, amIm_, amStepRe_, amStepIm_);
        for (uint8_t h = 0; h < p_.harmonics && h < MAX_HARM; ++h) {
            s += oscIm_[h] / static_cast<float>(h + 1);
            rotate(oscRe_[h], oscIm_[h], stepRe_[h], stepIm_[h]);
        }
        s *= am * 0.5f;
        break; }
    case SimScenario::SingleTone:
        s = oscIm_[0];
        rotate(oscRe_[0], oscIm_[0], stepRe_[0], stepIm_[0]);
        break;
    case SimScenario::WindNoise: {
        // 1/f-Näherung (Paul-Kellet-Filter, 3 Pole)
        const float w = noise();
        pinkState_[0] = 0.99765f * pinkState_[0] + w * 0.0990460f;
        pinkState_[1] = 0.96300f * pinkState_[1] + w * 0.2965164f;
        pinkState_[2] = 0.57000f * pinkState_[2] + w * 1.0526913f;
        s = (pinkState_[0] + pinkState_[1] + pinkState_[2] + w * 0.1848f) * 0.3f;
        break; }
    case SimScenario::Silence:
    default:
        s = 0.0f;
        break;
    }
    return s;
}

void Signal_Simulator::generateHop(uint64_t time_utc_us)
{
    if (p_.scenario == SimScenario::DroneSweep) advanceSweep();

    // --- Fernfeld-Verzögerung je Mikrofon: τ_m = -(p_m · u) / c ---
    const float az = p_.azimuth_deg * 3.14159265f / 180.0f;
    const float ux = std::cos(az), uy = std::sin(az);
    for (uint32_t m = 0; m < NUM_MICS; ++m) {
        const Vec3& pm = array_.position(m);
        delaySamples_[m] = -(pm.x * ux + pm.y * uy) / c_ * SAMPLE_RATE_HZ;
    }

    // --- Quellsignal mit Vorlauf ---
    const float amp = p_.source_level / (p_.distance_m > 1.0f ? p_.distance_m : 1.0f);   // 1/r
    const float noiseAmp = amp * std::pow(10.0f, -p_.snr_db / 20.0f);

    // Nur neue Samples erzeugen: beim ersten Aufruf das ganze Fenster, danach um HOP_SAMPLES schieben
    updateOscillators();
    uint32_t first = 0;
    if (primed_) { std::memmove(src_, src_ + HOP_SAMPLES, sizeof(float) * 2 * GUARD); first = 2 * GUARD; }
    for (uint32_t n = first; n < HOP_SAMPLES + 2 * GUARD; ++n) src_[n] = sourceSample() * amp;
    primed_ = true;

    // --- pro Mikrofon: verzögert + eigenes Rauschen, blockweise wie der DMA ---
    // Format wie die Hardware (116): 24-bit PCM linksbündig im 32-bit-Slot (pcm24 << 8)
    constexpr float toPcm24 = static_cast<float>(1 << 23);
    for (uint32_t b0 = 0; b0 < HOP_SAMPLES; b0 += DMA_BLOCK_SAMPLES) {
        for (uint32_t s = 0; s < DMA_BLOCK_SAMPLES; ++s) {
            const uint32_t n = b0 + s;
            for (uint32_t m = 0; m < NUM_MICS; ++m) {
                const float pos = static_cast<float>(n + GUARD) - delaySamples_[m];
                const int   i0  = static_cast<int>(pos);
                const float fr  = pos - static_cast<float>(i0);
                float v = 0.0f;
                if (i0 >= 0 && i0 + 1 < static_cast<int>(HOP_SAMPLES + 2 * GUARD))
                    v = src_[i0] * (1.0f - fr) + src_[i0 + 1] * fr;
                v += noiseAmp * noise();
                if (v >  0.999f) v =  0.999f;
                if (v < -0.999f) v = -0.999f;
                const int32_t pcm24 = static_cast<int32_t>(v * toPcm24);
                block_[s * NUM_MICS + m] = static_cast<int32_t>(static_cast<uint32_t>(pcm24) << 8);
            }
        }
        array_.pushBlock(block_, DMA_BLOCK_SAMPLES, time_utc_us + static_cast<uint64_t>(b0) * 1000000ULL / SAMPLE_RATE_HZ);
    }
}

} // namespace sds110
