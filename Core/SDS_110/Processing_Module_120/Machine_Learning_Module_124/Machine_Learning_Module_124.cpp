/*
 * Machine_Learning_Module_124.cpp – HBD -> s(t)
 */
#include "Machine_Learning_Module_124.hpp"
#include <cmath>
#include <cstring>

namespace sds110 {

static inline float sigmoid(float x) { return 1.0f / (1.0f + std::exp(-x)); }

bool Machine_Learning_Module_124::init()
{
    HBD_InitParams_48k(hbdParams_);
    HBD_InitState(hbdState_, hbdParams_);
    std::memset(history_, 0, sizeof(history_));
    histIdx_ = histCount_ = frame_ = 0;
    sinceDetect_ = HBD_HOLD_FRAMES;
    ready_ = true;
    return true;
}

bool Machine_Learning_Module_124::infer(const float* mag, const FeatureVector& /*features*/, AcousticState& s)
{
    if (!ready_ || !mag) return false;
    HBD_ProcessFrame(hbdParams_, hbdState_, mag);
    sinceDetect_ = hbdState_.droneDetected ? 0 : (sinceDetect_ < HBD_HOLD_FRAMES ? sinceDetect_ + 1 : HBD_HOLD_FRAMES);
    bandProbabilities(s);
    s.frame_id = frame_++;
    smooth(s);
    return true;
}

void Machine_Learning_Module_124::bandProbabilities(AcousticState& s) const
{
    const HBD_State& st = hbdState_;
    const HBD_Params& p = hbdParams_;

    // 1) Band-SNR aus Noise-Floor
    float q[NUM_BANDS];
    for (uint32_t b = 0; b < NUM_BANDS; ++b) {
        uint32_t k0, k1; Feature_Extraction_Module_122::bandBins(b, k0, k1);
        float sig = -1e9f, noise = 0.0f;
        for (uint32_t k = k0; k < k1; ++k) { if (st.magDb[k] > sig) sig = st.magDb[k]; noise += st.noiseFloorDb[k]; }
        const float snr = (k1 > k0) ? sig - noise / static_cast<float>(k1 - k0) : 0.0f;
        q[b] = sigmoid((snr - HBD_BAND_SNR_DB) / HBD_SIGMOID_DB);
        s.p[b] = HBD_GATE_FLOOR * q[b];                     // ohne Harmonische: nie selektiert
    }

    // 2) Bänder mit Harmonischen
    const uint8_t H = (p.harmonic.numHarmonics > HBD_MAX_HARMONICS) ? HBD_MAX_HARMONICS : p.harmonic.numHarmonics;
    for (uint8_t h = 0; h < H; ++h) {
        if (st.harmonicBin[h] == 0) continue;
        const float hz = static_cast<float>(st.harmonicBin[h]) * SAMPLE_RATE_HZ / N_FFT;
        if (hz < BAND_LO_HZ || hz >= BAND_HI_HZ) continue;
        const uint32_t b = static_cast<uint32_t>((hz - BAND_LO_HZ) / BAND_WIDTH_HZ);
        if (b >= NUM_BANDS) continue;
        const float ph = st.consistencyHistory[h] * sigmoid((st.lastBandSnr[h] - p.snr.perBandSnrDb[h]) / HBD_SIGMOID_DB);
        const float v = (ph > q[b]) ? ph : q[b];
        if (v > s.p[b]) s.p[b] = v;
    }

    // 3) Gate: HBD-Detektion innerhalb der letzten HBD_HOLD_FRAMES Frames
    if (sinceDetect_ >= HBD_HOLD_FRAMES)
        for (uint32_t b = 0; b < NUM_BANDS; ++b) s.p[b] *= HBD_GATE_FLOOR;
}

void Machine_Learning_Module_124::smooth(AcousticState& s)
{
    std::memcpy(history_[histIdx_], s.p, sizeof(s.p));
    histIdx_ = (histIdx_ + 1) % STATE_SMOOTH_FRAMES;
    if (histCount_ < STATE_SMOOTH_FRAMES) ++histCount_;
    for (uint32_t b = 0; b < NUM_BANDS; ++b) {
        float acc = 0.0f;
        for (uint32_t i = 0; i < histCount_; ++i) acc += history_[i][b];
        s.p[b] = acc / histCount_;
    }
}

} // namespace sds110
