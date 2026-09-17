/*
 * Signal_Simulator.cpp
 */
#include "Signal_Simulator.hpp"
#include <cmath>
#include <cstring>

namespace sds110 {

Signal_Simulator& Signal_Simulator::instance() { static Signal_Simulator inst; return inst; }

void Signal_Simulator::init(const SimParams& p)
{
    p_ = p;
    for (auto& ph : phase_) ph = 0.0;
    amPhase_ = 0.0;
    std::memset(pinkState_, 0, sizeof(pinkState_));
}

float Signal_Simulator::noise()
{
    rng_ = rng_ * 1664525u + 1013904223u;                 // LCG, uniform
    const float u1 = (rng_ >> 8) * (1.0f / 16777216.0f);
    rng_ = rng_ * 1664525u + 1013904223u;
    const float u2 = (rng_ >> 8) * (1.0f / 16777216.0f);
    return std::sqrt(-2.0f * std::log(u1 + 1e-9f)) * std::cos(6.2831853f * u2);   // ~N(0,1)
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

void Signal_Simulator::generateFrame(uint64_t time_utc_us)
{
    if (p_.scenario == SimScenario::DroneSweep) advanceSweep();

    // --- Fernfeld-Verzögerung je Mikrofon: τ_m = -(p_m · u) / c ---
    const float az = p_.azimuth_deg * 3.14159265f / 180.0f;
    const float ux = std::cos(az), uy = std::sin(az);
    for (uint32_t m = 0; m < NUM_MICS; ++m) {
        const Vec3& pm = array_.position(m);
        delaySamples_[m] = -(pm.x * ux + pm.y * uy) / SPEED_OF_SOUND * SAMPLE_RATE_HZ;
    }

    // --- Quellsignal mit Vorlauf ---
    const float amp = p_.source_level / (p_.distance_m > 1.0f ? p_.distance_m : 1.0f);   // 1/r
    const float noiseAmp = amp * std::pow(10.0f, -p_.snr_db / 20.0f);
    const double dt = 1.0 / SAMPLE_RATE_HZ;
    const double twoPi = 6.283185307179586;

    for (uint32_t n = 0; n < FRAME_SAMPLES + 2 * GUARD; ++n) {
        float s = 0.0f;
        switch (p_.scenario) {
        case SimScenario::DroneSweep:
        case SimScenario::DroneStatic: {
            // Harmonische mit fallender Amplitude (1/h) und Blattpass-AM
            const float am = 1.0f + 0.5f * static_cast<float>(std::sin(amPhase_));
            amPhase_ += twoPi * p_.bpf_mod_hz * dt;
            for (uint8_t h = 0; h < p_.harmonics && h < 16; ++h) {
                s += static_cast<float>(std::sin(phase_[h])) / (h + 1);
                phase_[h] += twoPi * p_.f0_hz * (h + 1) * dt;
                if (phase_[h] > twoPi) phase_[h] -= twoPi;
            }
            s *= am * 0.5f;
            break; }
        case SimScenario::SingleTone:
            s = static_cast<float>(std::sin(phase_[0]));
            phase_[0] += twoPi * p_.f0_hz * dt;
            if (phase_[0] > twoPi) phase_[0] -= twoPi;
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
        src_[n] = s * amp;
    }

    // --- pro Mikrofon: verzögert + eigenes Rauschen, blockweise wie der DMA ---
    constexpr float toPcm24 = static_cast<float>(1 << 23);
    for (uint32_t b0 = 0; b0 < FRAME_SAMPLES; b0 += DMA_BLOCK_SAMPLES) {
        for (uint32_t s = 0; s < DMA_BLOCK_SAMPLES; ++s) {
            const uint32_t n = b0 + s;
            for (uint32_t m = 0; m < NUM_MICS; ++m) {
                const float pos = static_cast<float>(n + GUARD) - delaySamples_[m];
                const int   i0  = static_cast<int>(pos);
                const float fr  = pos - static_cast<float>(i0);
                float v = 0.0f;
                if (i0 >= 0 && i0 + 1 < static_cast<int>(FRAME_SAMPLES + 2 * GUARD))
                    v = src_[i0] * (1.0f - fr) + src_[i0 + 1] * fr;
                v += noiseAmp * noise();
                if (v >  0.999f) v =  0.999f;
                if (v < -0.999f) v = -0.999f;
                block_[s * NUM_MICS + m] = static_cast<int32_t>(v * toPcm24);
            }
        }
        array_.pushBlock(block_, DMA_BLOCK_SAMPLES, time_utc_us + static_cast<uint64_t>(b0) * 1000000ULL / SAMPLE_RATE_HZ);
    }
}

} // namespace sds110
