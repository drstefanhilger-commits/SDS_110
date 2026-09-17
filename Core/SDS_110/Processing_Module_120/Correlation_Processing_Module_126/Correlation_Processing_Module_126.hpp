/*
 * Correlation_Processing_Module_126.hpp
 *
 * Korrelationsmodul 126 (Patent, Abschnitte 4, 5, 10; FIG. 2 unten, FIG. 3, FIG. 8):
 *  (d)(i)  Komponentenselektion S(t) = { k : p_b(k)(t) > θ_sel }
 *  (d)(ii) Gewichtung w(t,k) = g(p_b(t)) = p^γ
 *  (e)     Quellkonditionierte GCC-PHAT: R_ij(k) = w(k) · X_i X_j* / |X_i X_j*| nur auf S(t),
 *          IFFT -> Kreuzkorrelation, Peak im Fenster ±τ_max, Peak-Ratio-Test -> TDOA τ_ij
 *  Feedback (Abschnitt 10): ŝ senkt θ_sel für Referenzbänder, x̂ verengt das Suchfenster.
 *
 *  Inter-Unit (N ≥ 3): crossCorrelate() auf den Referenzkanal-Spektren zweier Units.
 *  Intra-Unit (Peilung, "no beamforming is required"): estimateBearing() korreliert die
 *  8 Mikrofone einer Unit paarweise und löst die Fernfeld-Richtung u per gewichteter LS.
 *
 * Migration aus SDS/Algorithm/SRPPhat + SDS_SRPBuffers:
 *  - computePHATIFFT -> crossCorrelate (mit Selektion/Gewichtung, CMSIS statt kiss_fft)
 *  - stepAzimuthScan (SRP-Gitter über 360 Hypothesen) -> estimateBearing (TDOA-LS, kein Scan)
 *  - calibrateAzimuth/filterAzimuth -> entfallen (Kalibrierung in 128, Glättung ist Tracking 150)
 */
#pragma once
#include "arm_math.h"
#include "SDS_110_Config.hpp"
#include "Data_Interface_140/Candidate_Report_140.hpp"
#include "Sensor_Unit_112/Microphone_Array_114.hpp"
#include "Processing_Module_120/Feature_Extraction_Module_122/Feature_Extraction_Module_122.hpp"

namespace sds110 {

struct ComponentSelection {
    bool     selected[NUM_BINS] = {};
    float    weight  [NUM_BINS] = {};
    uint32_t num_bands = 0;      // selektierte Bänder
    uint32_t num_bins  = 0;      // selektierte Bins
};

struct TdoaMeasurement {
    uint32_t i = 0, j = 0;       // Unit- oder Mikrofonindizes
    float    tdoa_s = 0.0f;
    float    peak = 0.0f;
    float    peak_ratio = 0.0f;
    bool     valid = false;
};

struct Bearing {
    float    azimuth_deg = 0.0f;
    float    residual = 0.0f;    // gewichtetes LS-Residuum (s)
    uint8_t  valid_pairs = 0;
    float    mean_peak = 0.0f;   // mittlere GCC-PHAT-Peakhöhe (Pegelmaß für 128-Fallback)
    bool     valid = false;
};

class Correlation_Processing_Module_126 {
public:
    void init(const Microphone_Array_114& array);

    /// Abschnitt 4: S(t), w(t,k) aus s(t)
    void deriveSelection(const AcousticState& state, ComponentSelection& sel) const;

    /// Abschnitt 5: quellkonditionierte GCC-PHAT eines Paars, Suchfenster ±maxDelay_s
    bool crossCorrelate(const Spectrum& Xi, const Spectrum& Xj, const ComponentSelection& sel,
                        float maxDelay_s, TdoaMeasurement& out);

    /// Intra-Unit-Peilung aus allen Mikrofonpaaren einer Unit (TDOA-Least-Squares, Patentpfad)
    bool estimateBearing(const Spectrum* micSpectra, const ComponentSelection& sel, Bearing& out);

    /// Referenz: SRP-PHAT-Scan über die in estimateBearing() gespeicherten Paarkorrelationen
    /// (klassisches Verfahren aus SDS/Algorithm/SRPPhat, hier ohne eigene FFTs). Nur Vergleich.
    bool srpScan(float& azimuth_deg, float& peakPower, float& peakRatio) const;

    /// Abschnitt 10: Feedback der Tracking Unit
    void applyFeedback(const TrackingFeedback& fb);
    void clearFeedback();

    const TdoaMeasurement* lastPairTdoa() const { return pairTdoa_; }

private:
    float thetaSel_[NUM_BANDS];
    float weightBoost_[NUM_BANDS];
    TrackingFeedback feedback_{};

    Vec3  micPos_[NUM_MICS];
    float maxIntraDelay_s_ = 0.0f;   // Arraydurchmesser / c
    arm_rfft_fast_instance_f32 ifft_;
    float spec_[N_FFT];              // gepacktes Spektrum für die IFFT
    float corr_[N_FFT];              // Kreuzkorrelation (zeitlich)
    TdoaMeasurement pairTdoa_[NUM_MIC_PAIRS];
    // Fenster ±SRP_MAX_LAG jeder Paarkorrelation für srpScan()
    float pairCorr_[NUM_MIC_PAIRS][2 * SRP_MAX_LAG + 1];
    float pairDx_[NUM_MIC_PAIRS], pairDy_[NUM_MIC_PAIRS];   // (p_i - p_j)/c * fs
};

} // namespace sds110
