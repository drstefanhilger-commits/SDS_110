/*
 * Signal_Simulator.hpp  (Harness, kein Patentmodul)
 *
 * Erzeugt synthetische Mikrofonsignale ohne Hardware und speist sie über
 * Microphone_Array_114::pushBlock() ein – identisch zum DMA-Pfad von 116.
 * Alles ab 118 läuft damit real auf dem Board.
 *
 * Szenarien (SimScenario):
 *   DroneSweep   – Rotorsignal (f0 + Harmonische mit Blattpass-AM), Azimut dreht, Distanz steigt
 *   DroneStatic  – wie oben, feste Position (Parameter testen)
 *   SingleTone   – ein reiner Ton (harmonisch NICHT drohnentypisch) -> Fehlalarm-Test
 *   WindNoise    – tieffrequentes Rauschen (1/f) -> Fehlalarm-Test
 *   Silence      – nur Grundrauschen -> Noise-Floor-Einschwingen
 *
 * Ersetzt SDS/Harness/UnitTestSignals (Breitbandrauschen) und MicTask::simulateMic.
 */
#pragma once
#include <cstdint>
#include "SDS_110_Config.hpp"
#include "Sensor_Unit_112/Microphone_Array_114.hpp"

namespace sds110 {

enum class SimScenario : uint8_t { DroneSweep = 0, DroneStatic, SingleTone, WindNoise, Silence };

struct SimParams {
    SimScenario scenario   = SimScenario::DroneSweep;
    float azimuth_deg      = 30.0f;    // Startwert bzw. fest
    float distance_m       = 50.0f;
    float f0_hz            = 180.0f;   // Rotor-Grundfrequenz (Drone) / Tonfrequenz (SingleTone)
    uint8_t harmonics      = 8;
    float bpf_mod_hz       = 4.0f;     // Blattpass-Amplitudenmodulation
    float snr_db           = 20.0f;    // Quelle zu Rauschen am Array (bei distance_m)
    float sweep_az_step    = 1.0f;     // DroneSweep: Grad pro Frame
    float sweep_dist_step  = 0.5f;     // DroneSweep: m pro Umdrehung
    float source_level     = 0.3f;     // Amplitude bei 1 m (float-Vollaussteuerung = 1)
};

class Signal_Simulator {
public:
    static Signal_Simulator& instance();
    void init(const SimParams& p);
    SimParams& params() { return p_; }
    /// erzeugt einen Hop (HOP_SAMPLES je Mikrofon) und pusht ihn blockweise in 114
    void generateHop(uint64_t time_utc_us);
    float trueAzimuth() const { return p_.azimuth_deg; }
    float trueDistance() const { return p_.distance_m; }

private:
    Signal_Simulator() = default;
    float noise();
    void  advanceSweep();

    SimParams p_{};
    Microphone_Array_114& array_ = Microphone_Array_114::instance();
    float    delaySamples_[NUM_MICS] = {};
    double   phase_[16] = {};
    double   amPhase_ = 0.0;
    uint32_t rng_ = 0x12345678;
    float    pinkState_[3] = {};
    // Quellsignal: [GUARD Vergangenheit][HOP_SAMPLES aktuell][GUARD Vorlauf]; zwischen zwei
    // Aufrufen wird um HOP_SAMPLES geschoben, jedes Sample wird genau einmal erzeugt
    static constexpr uint32_t GUARD = 64;   // > max. Verzögerung (Radius 0,2 m -> 28 Samples)
    float src_[HOP_SAMPLES + 2 * GUARD];
    bool  primed_ = false;
    float sourceSample();
    int32_t block_[DMA_BLOCK_SAMPLES * NUM_MICS];
};

} // namespace sds110
