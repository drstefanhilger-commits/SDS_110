/*
 * Machine_Learning_Module_124.hpp
 * ML-Modul 124 (Patent, Abschnitt 3, FIG. 4):
 * Modell mit B Sigmoid-Ausgängen -> s(t) = (p_1(t) .. p_B(t)).
 * Optional Glättung über 3 Frames. Training erfolgt offline (kein Teil von 100).
 * Migration: ML/SDS_AIModel.c/h (CubeAI) – aktuell 4 Klassen, muss auf
 *            B = 64 Bandwahrscheinlichkeiten umgestellt werden.
 */
#pragma once
#include "SDS_110_Config.hpp"
#include "Data_Interface_140/Candidate_Report_140.hpp"
#include "Processing_Module_120/Feature_Extraction_Module_122/Feature_Extraction_Module_122.hpp"

namespace sds110 {

class Machine_Learning_Module_124 {
public:
    bool init();                 // Modellparameter laden (CubeAI)
    /// Features -> Acoustic State s(t)
    bool infer(const FeatureVector& features, AcousticState& state);
private:
    void smooth(AcousticState& state);
    float history_[STATE_SMOOTH_FRAMES][NUM_BANDS] = {};
    uint32_t histIdx_ = 0;
};

} // namespace sds110
