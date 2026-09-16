/*
 * Correlation_Processing_Module_126.hpp
 * Korrelationsmodul 126 (Patent, Abschnitte 4, 5, 10; FIG. 2 unten, FIG. 3, FIG. 8):
 *  (d)(i)  Komponentenselektion S(t) = { (t,k): p_b(t) > θ_sel }
 *  (d)(ii) Gewicht w(t,k) = g(p_b(t))
 *  (e)     Quellkonditionierte GCC-PHAT nur über S(t), gewichtet mit w
 *          -> TDOA τ_ij pro Unit-Paar, Peak-Ratio-Test
 *  Feedback: ŝ senkt θ_sel für Referenzbänder, x̂ beschränkt Suchfenster
 * Migration: Algorithm/SRPPhat (prepareGCCPHAT, computePHATIFFT), kiss_fft
 */
#pragma once
#include "SDS_110_Config.hpp"
#include "Data_Interface_140/Candidate_Report_140.hpp"
#include "Processing_Module_120/Feature_Extraction_Module_122/Feature_Extraction_Module_122.hpp"

namespace sds110 {

struct ComponentSelection {
    bool     selected[NUM_BINS] = {};
    float    weight  [NUM_BINS] = {};
    uint32_t num_bands          = 0;
};

struct TdoaMeasurement {
    uint32_t unit_i, unit_j;
    float    tdoa_s;
    float    peak_ratio;
    bool     valid;
};

class Correlation_Processing_Module_126 {
public:
    void init();
    /// Abschnitt 4: S(t) und w(t,k) aus s(t) (einmal, für alle Units gültig)
    void deriveSelection(const AcousticState& state, ComponentSelection& sel) const;
    /// Abschnitt 5: quellkonditionierte GCC-PHAT für ein Unit-Paar
    bool crossCorrelate(const Spectrum& Xi, const Spectrum& Xj,
                        const ComponentSelection& sel, float maxDelay_s,
                        TdoaMeasurement& out);
    /// Abschnitt 10: Feedback der Tracking Unit für das nächste Intervall
    void applyFeedback(const TrackingFeedback& fb);
private:
    float thetaSel_[NUM_BANDS];    // pro Band (Standard θ_sel, ggf. θ_low)
    float weightBoost_[NUM_BANDS]; // 1 + ŝ_b oder 1
    TrackingFeedback feedback_;
};

} // namespace sds110
