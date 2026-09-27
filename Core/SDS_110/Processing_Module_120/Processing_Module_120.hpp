/*
 * Processing_Module_120.hpp
 *
 * Board-seitige Vorstufe des Verarbeitungsmoduls 120 (Patent, Abschnitt 1, FIG. 1).
 *
 * Zuordnung: Das STM32-Board ist die Sensoreinheit 112-n. Die Stufen 122, 124 und der
 * Intra-Unit-Teil von 126 werden hier auf der Einheit vorgerechnet; das eigentliche
 * Processing Module 120 (Inter-Unit-GCC-PHAT, hyperbolische Lokalisation 128, Candidate
 * Report 130) läuft auf dem PC-Monitor, ebenso – getrennt davon – die Tracking Unit 150.
 *
 * Orchestriert pro Frame:  112 -> 122 -> 124 -> 126(intra) -> UnitReport -> 140 (USB)
 * Enthält KEINE Trajektorienbildung (Claim 10).
 *
 * Migration aus SDS/Tasks/SRPTask (detectHandler/aiHandler) – Ablauf statt SRP-Scan:
 *   Frame -> STFT aller Mikrofone -> s(t) -> S(t)/w -> Intra-Unit-TDOA -> Peilung ->
 *   Kandidat -> Report; Status nach SDS_Data.
 */
#pragma once
#include "stm32f7xx_hal.h"
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
    bool init(SAI_HandleTypeDef* hsai, I2C_HandleTypeDef* hi2c);
    bool start();
    /// ein Verarbeitungsintervall; true wenn ein Frame verarbeitet wurde
    bool processFrame();
    /// READ-Modus: nächsten Frame roh über USB streamen (kein Patentschritt)
    bool streamFrame();

    const UnitReport& lastReport() const { return report_; }
    Sensor_Unit_112& unit() { return unit_; }

private:
    Processing_Module_120() : unit_(0) {}

    Sensor_Unit_112                   unit_;
    // 122 und 124 im internen RAM (statisch) statt in dieser Instanz im SDRAM: die FFT in 122
    // arbeitet in-place auf 16-kB-Puffern und verfehlte im SDRAM fast immer den 4-kB-D-Cache
    // (Board: 122 ~36 ms je Frame). 122/124 selbst bleiben unverändert (Merkmalsversion).
    static Feature_Extraction_Module_122 featInst_;
    static Machine_Learning_Module_124   mlInst_;
    Feature_Extraction_Module_122&    feat_ = featInst_;
    Machine_Learning_Module_124&      ml_   = mlInst_;
    static Correlation_Processing_Module_126 corrInst_;   // ebenfalls internes RAM (18 kB)
    Correlation_Processing_Module_126& corr_ = corrInst_;
    Localisation_Module_128           loc_;
    Output_Interface_130              out_;

    FeatureVector      features_{};
    AcousticState      state_{};
    ComponentSelection selection_{};
    Bearing            bearing_{};
    CandidateLocation  location_{};
    UnitReport         report_{};
    TrackingFeedback   feedback_{};

    float tPre_ = 0, tFeat_ = 0, tMl_ = 0, tCorr_ = 0, tRest_ = 0;   // ms je Stufe, geglättet
    float tSel_ = 0, tGcc_ = 0, tSrp_ = 0;                          // 126 aufgeteilt

    static Spectrum spectra_[NUM_MICS];   // SDRAM: 8 x 16 kB
};

} // namespace sds110
