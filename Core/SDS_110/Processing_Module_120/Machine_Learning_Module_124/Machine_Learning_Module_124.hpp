/*
 * Machine_Learning_Module_124.hpp
 * ML-Modul 124 (Patent, Abschnitt 3, FIG. 4): FeatureVector -> s(t) = (p_1..p_B),
 * B = 64 Sigmoid-Ausgänge. Zielimplementierung: HBD-ML (Harmonic Band Detection),
 * wird nach der Migration eingesetzt.
 *
 * Bis dahin (ML_PLACEHOLDER = true) liefert infer() eine Näherung aus der
 * normierten Bandleistung, damit 126/128/130 durchlaufen und getestet werden
 * können. Das alte CubeAI-Modell (SDS_AIModel) wird NICHT übernommen.
 */
#pragma once
#include "SDS_110_Config.hpp"
#include "Data_Interface_140/Candidate_Report_140.hpp"
#include "Processing_Module_120/Feature_Extraction_Module_122/Feature_Extraction_Module_122.hpp"

namespace sds110 {

class Machine_Learning_Module_124 {
public:
    bool init();
    bool infer(const FeatureVector& features, AcousticState& state);
    bool ready() const { return ready_; }
private:
    void smooth(AcousticState& state);
    bool  ready_ = false;
    float history_[STATE_SMOOTH_FRAMES][NUM_BANDS] = {};
    uint32_t histIdx_ = 0, histCount_ = 0;
    uint32_t frame_ = 0;
};

} // namespace sds110
