/*
 * Sensor_Unit_112.hpp
 * Akustische Sensoreinheit 112-n = 114 + 116 + 118.
 */
#pragma once
#include "Microphone_Array_114.hpp"
#include "Sampling_Circuitry_116.hpp"
#include "Pre_Processor_118.hpp"

namespace sds110 {

class Sensor_Unit_112 {
public:
    explicit Sensor_Unit_112(uint32_t id) : id_(id) {}
    bool init();
    /// liefert den nächsten vorverarbeiteten Frame oder nullptr
    const MicFrame* nextFrame();
    uint32_t id() const { return id_; }
private:
    uint32_t id_;
    Sampling_Circuitry_116& sampling_ = Sampling_Circuitry_116::instance();
    Pre_Processor_118 pre_;
};

} // namespace sds110
