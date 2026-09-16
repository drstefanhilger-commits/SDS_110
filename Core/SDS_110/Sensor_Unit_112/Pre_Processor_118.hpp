/*
 * Pre_Processor_118.hpp
 * Vorverarbeitung 118: AGC, Bandpass 80 Hz – 8 kHz, adaptive Rauschunterdrückung.
 * Migration: Tasks/MicTask (Real-/Simulationspfad) – Filter sind neu.
 */
#pragma once
#include "Microphone_Array_114.hpp"

namespace sds110 {

class Pre_Processor_118 {
public:
    void init();
    /// in-place auf einem Frame aller M Mikrofone
    void process(MicFrame& frame);
private:
    void agc(float* x, uint32_t n);
    void bandpass(float* x, uint32_t n);
    void noiseSuppress(float* x, uint32_t n);
};

} // namespace sds110
