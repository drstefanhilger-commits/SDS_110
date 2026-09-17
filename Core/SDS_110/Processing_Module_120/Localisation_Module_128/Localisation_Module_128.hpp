/*
 * Localisation_Module_128.hpp
 *
 * Lokalisationsmodul 128 (Patent, Abschnitt 6, FIG. 3 rechts, FIG. 5):
 *  Hyperbolische Gleichungen |x-u_i| - |x-u_j| = c·τ_ij aus ≥ MIN_PAIRS Unit-Paaren,
 *  gewichtete kleinste Quadrate (Gauss-Newton) -> x; Ausgabe als Azimut φ und
 *  Distanz r relativ zum Referenzpunkt (Zentroid der Unit-Positionen).
 *
 *  solve() ist die Referenzimplementierung für das Processing Module auf dem PC
 *  (Inter-Unit-TDOA aus >= MIN_PAIRS Unit-Paaren); auf dem Board wird nur fromBearing() genutzt.
 *
 *  Einzel-Unit-Betrieb (NUM_UNITS = 1): Azimut aus der Intra-Unit-Peilung (126);
 *  Distanz optional aus dem Legacy-Pegelmodell r = K/(A+eps) (DistanceEstimator) –
 *  markiert als Fallback, nicht Teil des Patentverfahrens.
 *
 * Migration aus SDS/Algorithm/DistanceEstimator, SRP_DAS_Distance (Pegelmodell),
 * SRPPhat::calibrateAzimuth (Offset/Skalierung -> calibrate()).
 */
#pragma once
#include "SDS_110_Config.hpp"
#include "Sensor_Unit_112/Microphone_Array_114.hpp"
#include "Processing_Module_120/Correlation_Processing_Module_126/Correlation_Processing_Module_126.hpp"

namespace sds110 {

struct CandidateLocation {
    float   azimuth_deg = 0.0f;
    float   distance_m  = 0.0f;
    uint8_t accepted_pairs = 0;
    float   ls_residual = 0.0f;
    bool    valid = false;
};

class Localisation_Module_128 {
public:
    void init(const Vec3* unitPositions, uint32_t numUnits);

    /// Mehrere Units: hyperbolische TDOA-Lösung
    bool solve(const TdoaMeasurement* tdoa, uint32_t count, CandidateLocation& out);
    /// Eine Unit: Peilung + Pegel-Fallback
    bool fromBearing(const Bearing& b, float levelA, CandidateLocation& out);

    /// Azimut-Kalibrierung (aus SRPPhat::calibrateAzimuth); Standard: keine Korrektur
    void setCalibration(float offsetDeg, float scale) { azOffset_ = offsetDeg; azScale_ = scale; }

private:
    float calibrate(float az) const;
    Vec3     units_[8]{};
    uint32_t numUnits_ = 0;
    Vec3     ref_{};
    float    azOffset_ = 0.0f, azScale_ = 1.0f;
};

} // namespace sds110
