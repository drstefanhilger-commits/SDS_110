/*
 * Machine_Learning_Module_124.hpp
 *
 * Modul 124 (Patent, Abschnitt 3, FIG. 4): Ausgang s(t) = (p_1 .. p_B), B = 64.
 * Implementierung: HBD – Harmonic Band Detector (klassisch, kein trainiertes Modell).
 *
 * Brücke HBD -> s(t):
 *   1. je Patent-Band b: SNR_b = max|X| im Band − mittlerer Noise-Floor (dB)
 *      p_b = σ((SNR_b − HBD_BAND_SNR_DB) / HBD_SIGMOID_DB)
 *   2. Bänder, in die eine Harmonische h fällt:
 *      p_b = max(p_b, c_h · σ((SNR_h − θ_h) / HBD_SIGMOID_DB)), c_h = Konsistenz von h
 *   3. globales Gate g = clamp(score / finalScoreThreshold, 0, 1):
 *      p_b *= HBD_GATE_FLOOR + (1 − HBD_GATE_FLOOR) · g
 *   4. Glättung über STATE_SMOOTH_FRAMES
 *
 * Eingang ist das Betragsspektrum aus 122 (nicht der FeatureVector); der bleibt
 * für einen späteren trainierten Ersatz von HBD im Interface.
 */
#pragma once
#include "SDS_110_Config.hpp"
#include "Data_Interface_140/Candidate_Report_140.hpp"
#include "Processing_Module_120/Feature_Extraction_Module_122/Feature_Extraction_Module_122.hpp"
#include "HBD.hpp"

namespace sds110 {

class Machine_Learning_Module_124 {
public:
    bool init();
    /// mag: |X(t,k)| mit NUM_BINS Werten (122::magnitude())
    bool infer(const float* mag, const FeatureVector& features, AcousticState& state);
    bool ready() const { return ready_; }

    // Diagnose (LCD / Logger)
    const HBD_State&  hbd() const { return hbdState_; }
    HBD_Params&       params()    { return hbdParams_; }

private:
    void bandProbabilities(AcousticState& s) const;
    void smooth(AcousticState& state);

    HBD_Params hbdParams_{};
    HBD_State  hbdState_{};
    bool  ready_ = false;
    float history_[STATE_SMOOTH_FRAMES][NUM_BANDS] = {};
    uint32_t histIdx_ = 0, histCount_ = 0, frame_ = 0;
};

} // namespace sds110
