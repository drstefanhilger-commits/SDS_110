/*
 * Candidate_Report_140.hpp
 * Datenschnittstelle 140 (Patent, Abschnitt 7, FIG. 6).
 * Candidate Report: SDS 110 -> Tracking Unit 150.
 * Feedback:         Tracking Unit 150 -> Correlation_Processing_Module_126.
 */
#pragma once
#include <cstdint>
#include "SDS_110_Config.hpp"

namespace sds110 {

/// UAV source-specific acoustic state s(t) = (p_1 .. p_B)   [Abschnitt 3]
struct AcousticState {
    float    p[NUM_BANDS] = {};
    uint32_t frame_id     = 0;
};

/// Kandidatenbericht, einmal pro Frame mit gültiger Position   [Abschnitt 7]
struct CandidateReport {
    uint64_t time_utc_us      = 0;   // Zeitreferenz
    float    azimuth_deg      = 0;   // φ
    float    distance_m       = 0;   // r
    uint8_t  accepted_pairs   = 0;   // Qualität
    float    ls_residual      = 0;   // Qualität
    uint8_t  num_selected     = 0;
    uint8_t  band_index[NUM_BANDS] = {};
    float    band_prob [NUM_BANDS] = {};
};

/// Rückmeldung der Tracking Unit                              [Abschnitt 10, FIG. 8]
struct TrackingFeedback {
    float    ref_state[NUM_BANDS] = {}; // ŝ
    float    pred_azimuth_deg     = 0;  // x̂(t+1)
    float    pred_distance_m      = 0;
    bool     valid                = false;
};

} // namespace sds110
