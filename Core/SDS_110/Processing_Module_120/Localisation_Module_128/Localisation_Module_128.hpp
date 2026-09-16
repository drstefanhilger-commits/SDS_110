/*
 * Localisation_Module_128.hpp
 * Lokalisationsmodul 128 (Patent, Abschnitt 6, FIG. 3 rechts, FIG. 5):
 * Hyperbolische Gleichungen |x-u_i| - |x-u_j| = c·τ_ij aus >= 3 Paaren,
 * gewichtete kleinste Quadrate -> x, ausgedrückt als Azimut φ und Distanz r
 * relativ zum Referenzpunkt (Zentroid der Unit-Positionen).
 * Migration: Algorithm/DistanceEstimator, SRP_DAS_Distance, SRPPhat (Azimut-Scan)
 */
#pragma once
#include "SDS_110_Config.hpp"
#include "Sensor_Unit_112/Microphone_Array_114.hpp"
#include "Processing_Module_120/Correlation_Processing_Module_126/Correlation_Processing_Module_126.hpp"

namespace sds110 {

struct CandidateLocation {
    float    azimuth_deg;
    float    distance_m;
    uint8_t  accepted_pairs;
    float    ls_residual;
    bool     valid;
};

class Localisation_Module_128 {
public:
    void init(const Vec3* unitPositions, uint32_t numUnits);
    bool solve(const TdoaMeasurement* tdoa, uint32_t count, CandidateLocation& out);
private:
    Vec3     units_[8];
    uint32_t numUnits_ = 0;
    Vec3     reference_{};        // Zentroid
};

} // namespace sds110
