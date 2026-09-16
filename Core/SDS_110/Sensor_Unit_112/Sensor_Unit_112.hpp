/*
 * Sensor_Unit_112.hpp
 * Akustische Sensoreinheit 112-n = 114 + 116 + 118.
 */
#pragma once
#include "stm32f7xx_hal.h"
#include "Microphone_Array_114.hpp"
#include "Sampling_Circuitry_116.hpp"
#include "Pre_Processor_118.hpp"

namespace sds110 {

class Sensor_Unit_112 {
public:
    explicit Sensor_Unit_112(uint32_t id) : id_(id) {}

    bool init(SAI_HandleTypeDef* hsai, I2C_HandleTypeDef* hi2c)
    {
        pre_.init();
        return sampling_.init(hsai, hi2c);
    }
    bool start() { return sampling_.start(); }

    /// nächsten vorverarbeiteten Frame holen (nullptr = noch keiner fertig).
    /// Aufrufer gibt ihn mit releaseFrame() zurück.
    MicFrame* nextFrame()
    {
        MicFrame* f = array_.acquireReadable();
        if (f) pre_.process(*f);
        return f;
    }
    void releaseFrame(MicFrame* f) { array_.release(f); }

    uint32_t id() const { return id_; }
    const Microphone_Array_114& array() const { return array_; }

private:
    uint32_t id_;
    Microphone_Array_114&   array_    = Microphone_Array_114::instance();
    Sampling_Circuitry_116& sampling_ = Sampling_Circuitry_116::instance();
    Pre_Processor_118       pre_;
};

} // namespace sds110
