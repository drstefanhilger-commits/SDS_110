/*
 * Sampling_Circuitry_116.hpp
 * Synchrone Abtastung 116: ADAU7118 (PDM->TDM) über SAI + DMA.
 * Migration: ADUA7118/sai.c, dma.c, TDM_Parser.c, adau7118.c, adua7118Driver.c
 */
#pragma once
#include "Microphone_Array_114.hpp"

namespace sds110 {

class Sampling_Circuitry_116 {
public:
    static Sampling_Circuitry_116& instance();
    bool init();
    bool start();
    void stop();
    /// vom DMA-Callback aufgerufen; TDM->Frame, Zeitstempel setzen
    void onDmaComplete(const int32_t* tdm, uint32_t samples);
private:
    Sampling_Circuitry_116() = default;
    Microphone_Array_114& array_ = Microphone_Array_114::instance();
};

} // namespace sds110
