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
#include "Infrastructure/Utils/DWT.hpp"

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

    /// Nächsten Hop aus 114 holen, mit 118 in den nächsten Slot des Analysefensters verarbeiten.
    /// Rückgabe false: kein Hop bereit. Sonst true; frame zeigt auf einen vollständigen
    /// Analyse-Frame (64 ms, 50 % Überlappung) oder ist nullptr, solange das Fenster füllt.
    bool nextFrame(const AnalysisFrame*& frame)
    {
        frame = nullptr;
        MicFrame* h = array_.acquireReadable();
        if (!h) return false;
        // 118 schreibt direkt in den nächsten Slot des Frame_Assemblers (keine Kopie, kein memmove)
        const uint32_t c0 = DWTTimer::instance().cycles();
        float* dst[NUM_MICS];
        asm_.beginHop(*h, dst);
        const uint32_t c1 = DWTTimer::instance().cycles();
        pre_.process(*h, dst);
        const uint32_t c2 = DWTTimer::instance().cycles();
        const bool full = asm_.commitHop(*h);
        lastPreCycles_ = c2 - c1;                      // Diagnose Rechenlast: 118 und Fenster getrennt
        lastAsmCycles_ = (c1 - c0) + (DWTTimer::instance().cycles() - c2);
        array_.release(h);
        if (full) frame = &asm_.frame();
        return true;
    }

    /// READ-Modus: nächster Hop als Rohdaten, also ohne 118 (Bandpass, NS, AGC), ohne Überlappung
    /// (nullptr = keiner bereit). Befund 35: Aufnahmen für Training und Endabnahme (AP 8) brauchen
    /// die Mikrofonsignale; tools/features/sds_features wendet 118 selbst an. Aufrufer gibt den
    /// Hop mit releaseHop() zurück. Das Analysefenster beginnt danach neu; 118 läuft beim Wechsel
    /// zurück nach DETECT mit dem alten Zustand weiter (einige Frames Einschwingen).
    MicFrame* nextHop()
    {
        MicFrame* h = array_.acquireReadable();
        if (h) asm_.reset();
        return h;
    }
    void releaseHop(MicFrame* h) { array_.release(h); }

    uint32_t id() const { return id_; }
    uint32_t lastPreCycles() const { return lastPreCycles_; }   ///< 118 des letzten Hops (DWT-Zyklen)
    uint32_t lastAsmCycles() const { return lastAsmCycles_; }   ///< Frame_Assembler des letzten Hops
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
    uint32_t lastPreCycles_ = 0, lastAsmCycles_ = 0;
};

} // namespace sds110
