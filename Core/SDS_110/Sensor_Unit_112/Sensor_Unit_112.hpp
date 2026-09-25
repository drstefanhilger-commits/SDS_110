/*
 * Sensor_Unit_112.hpp
 * Akustische Sensoreinheit 112-n = 114 + 116 + 118.
 */
#pragma once
#include "stm32f7xx_hal.h"
#include "Microphone_Array_114.hpp"
#include "Sampling_Circuitry_116.hpp"
#include "Pre_Processor_118.hpp"
#include "Frame_Assembler.hpp"

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

    /// Nächsten Hop aus 114 holen, mit 118 verarbeiten und ins Analysefenster schieben.
    /// Rückgabe false: kein Hop bereit. Sonst true; frame zeigt auf einen vollständigen
    /// Analyse-Frame (64 ms, 50 % Überlappung) oder ist nullptr, solange das Fenster füllt.
    bool nextFrame(const AnalysisFrame*& frame)
    {
        frame = nullptr;
        MicFrame* h = array_.acquireReadable();
        if (!h) return false;
        pre_.process(*h);
        const bool full = asm_.push(*h);
        array_.release(h);
        if (full) frame = &asm_.frame();
        return true;
    }

    /// READ-Modus: nächster vorverarbeiteter Hop ohne Überlappung (nullptr = keiner bereit).
    /// Aufrufer gibt ihn mit releaseHop() zurück. Das Analysefenster beginnt danach neu.
    MicFrame* nextHop()
    {
        MicFrame* h = array_.acquireReadable();
        if (h) { pre_.process(*h); asm_.reset(); }
        return h;
    }
    void releaseHop(MicFrame* h) { array_.release(h); }

    uint32_t id() const { return id_; }
    const Microphone_Array_114& array() const { return array_; }
    const Pre_Processor_118& preprocessor() const { return pre_; }
    const Sampling_Circuitry_116& sampling() const { return sampling_; }
    Sampling_Circuitry_116& sampling() { return sampling_; }

private:
    uint32_t id_;
    Microphone_Array_114&   array_    = Microphone_Array_114::instance();
    Sampling_Circuitry_116& sampling_ = Sampling_Circuitry_116::instance();
    Pre_Processor_118       pre_;
    Frame_Assembler         asm_;   // liegt mit 120 im SDRAM (~98 kB)
};

} // namespace sds110
