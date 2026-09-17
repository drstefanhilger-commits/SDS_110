/*
 * Machine_Learning_Module_124.cpp – HBD -> s(t)
 */
#include "Machine_Learning_Module_124.hpp"
#include <cmath>
#include <cstring>

namespace sds110 {

static inline float sigmoid(float x) { return 1.0f / (1.0f + std::exp(-x)); }
static inline float clamp01(float x)  { return (x < 0.0f) ? 0.0f : (x > 1.0f) ? 1.0f : x; }

bool Machine_Learning_Module_124::init()
{
    HBD_InitParams_48k(hbdParams_);
    HBD_InitState(hbdState_, hbdParams_);
    std::memset(history_, 0, sizeof(history_));
    histIdx_ = histCount_ = frame_ = 0;
    ready_ = true;
    return true;
}

bool Machine_Learning_Module_124::infer(const float* mag, const FeatureVector& /*features*/, AcousticState& s)
{
    if (!ready_ || !mag) return false;
    HBD_ProcessFrame(hbdParams_, hbdState_, mag);
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
    for (uint32_t b = 0; b < NUM_BANDS; ++b) {
        uint32_t k0, k1; Feature_Extraction_Module_122::bandBins(b, k0, k1);
        float sig = -1e9f, noise = 0.0f;
        for (uint32_t k = k0; k < k1; ++k) { if (st.magDb[k] > sig) sig = st.magDb[k]; noise += st.noiseFloorDb[k]; }
        const float snr = (k1 > k0) ? sig - noise / static_cast<float>(k1 - k0) : 0.0f;
        s.p[b] = sigmoid((snr - HBD_BAND_SNR_DB) / HBD_SIGMOID_DB);
    }

    // 2) Harmonische verstärken
    const uint8_t H = (p.harmonic.numHarmonics > HBD_MAX_HARMONICS) ? HBD_MAX_HARMONICS : p.harmonic.numHarmonics;
    for (uint8_t h = 0; h < H; ++h) {
        if (st.harmonicBin[h] == 0) continue;
        const float hz = static_cast<float>(st.harmonicBin[h]) * SAMPLE_RATE_HZ / N_FFT;
        if (hz < BAND_LO_HZ || hz >= BAND_HI_HZ) continue;
        const uint32_t b = static_cast<uint32_t>((hz - BAND_LO_HZ) / BAND_WIDTH_HZ);
        if (b >= NUM_BANDS) continue;
        const float ph = st.consistencyHistory[h] * sigmoid((st.lastBandSnr[h] - p.snr.perBandSnrDb[h]) / HBD_SIGMOID_DB);
        if (ph > s.p[b]) s.p[b] = ph;
    }

    // 3) globales Gate
    const float g = clamp01(st.score / (p.decision.finalScoreThreshold + 1e-6f));
    const float gate = HBD_GATE_FLOOR + (1.0f - HBD_GATE_FLOOR) * g;
    for (uint32_t b = 0; b < NUM_BANDS; ++b) s.p[b] *= gate;
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
