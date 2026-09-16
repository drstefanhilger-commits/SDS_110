/*
 * Processing_Module_120.hpp
 * Verarbeitungsmodul 120 (Patent, Abschnitt 1, FIG. 1).
 * Orchestriert pro Frame: 122 -> 124 -> 126 -> 128 -> 130
 * und speist das Feedback von 150 zurück in 126.
 * Enthält KEINE Trajektorienbildung (Claim 10).
 * Migration: Tasks/SRPTask (detectHandler / aiHandler)
 */
#pragma once
#include "Sensor_Unit_112/Sensor_Unit_112.hpp"
#include "Feature_Extraction_Module_122/Feature_Extraction_Module_122.hpp"
#include "Machine_Learning_Module_124/Machine_Learning_Module_124.hpp"
#include "Correlation_Processing_Module_126/Correlation_Processing_Module_126.hpp"
#include "Localisation_Module_128/Localisation_Module_128.hpp"
#include "Output_Interface_130/Output_Interface_130.hpp"

namespace sds110 {

class Processing_Module_120 {
public:
    static Processing_Module_120& instance();
    bool init();
    /// ein Verarbeitungsintervall (Claim 5: Zustand wird je Intervall neu gebildet)
    void processFrame();
private:
    Processing_Module_120() = default;

    Sensor_Unit_112                   units_[NUM_UNITS] = { Sensor_Unit_112(0) };
    Feature_Extraction_Module_122     feat_;
    Machine_Learning_Module_124       ml_;
    Correlation_Processing_Module_126 corr_;
    Localisation_Module_128           loc_;
    Output_Interface_130              out_;

    Spectrum           spectra_[NUM_UNITS];
    FeatureVector      features_;
    AcousticState      state_;
    ComponentSelection selection_;
    TdoaMeasurement    tdoa_[NUM_UNITS * (NUM_UNITS - 1) / 2 + 1];
    CandidateLocation  location_;
    CandidateReport    report_;
};

} // namespace sds110
