/*
 * Machine_Learning_Module_124.cpp – Platzhalter bis HBD-ML
 */
#include "Machine_Learning_Module_124.hpp"
#include <cmath>
#include <cstring>

namespace sds110 {

bool Machine_Learning_Module_124::init()
{
    // TODO HBD-ML: Modellparameter laden
    ready_ = ML_PLACEHOLDER;
    return ready_;
}

bool Machine_Learning_Module_124::infer(const FeatureVector& f, AcousticState& s)
{
    if (!ready_) return false;

    // Platzhalter: Bandleistung relativ zum Median aller Bänder, per Sigmoid auf [0,1].
    // Bänder, die deutlich über dem Median liegen (tonale Komponenten), bekommen hohe p_b.
    float sorted[NUM_BANDS];
    std::memcpy(sorted, f.band_log_power, sizeof(sorted));
    for (uint32_t i = 1; i < NUM_BANDS; ++i) {           // Insertion-Sort, 64 Werte
        float v = sorted[i]; int j = static_cast<int>(i) - 1;
        while (j >= 0 && sorted[j] > v) { sorted[j + 1] = sorted[j]; --j; }
        sorted[j + 1] = v;
    }
    const float median = sorted[NUM_BANDS / 2];
    for (uint32_t b = 0; b < NUM_BANDS; ++b) {
        const float z = (f.band_log_power[b] - median) - 1.5f;   // 1.5 Neper ≈ +13 dB über Median
        s.p[b] = 1.0f / (1.0f + std::exp(-2.0f * z));
    }
    s.frame_id = frame_++;
    smooth(s);
    return true;
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
