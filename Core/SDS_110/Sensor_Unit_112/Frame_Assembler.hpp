/*
 * Frame_Assembler.hpp
 *
 * Analyse-Frames mit Überlappung (Patent, Abschnitt 2: 64 ms, 50 %):
 * Gleitendes Fenster über die letzten FRAME_SAMPLES / HOP_SAMPLES Hops aus 114, nach 118.
 * Jeder neue Hop schiebt das Fenster um HOP_SAMPLES weiter; ab dem zweiten Hop liegt
 * je Hop ein vollständiger Frame vor (Takt 32 ms).
 *
 * Fehlt ein Hop (frame_id nicht fortlaufend, z. B. verworfen in 114 oder Moduswechsel),
 * beginnt das Fenster neu – ein Frame enthält nie zeitlich getrennte Stücke.
 */
#pragma once
#include <cstdint>
#include "SDS_110_Config.hpp"
#include "Microphone_Array_114.hpp"

namespace sds110 {

struct AnalysisFrame {
    uint32_t frame_id;                        // fortlaufend je vollständigem Frame
    uint64_t time_utc_us;                     // Zeit des ersten Samples im Frame
    float    data[NUM_MICS][FRAME_SAMPLES];
};

class Frame_Assembler {
public:
    static constexpr uint32_t HOPS_PER_FRAME = FRAME_SAMPLES / HOP_SAMPLES;

    void reset() { fill_ = 0; haveLast_ = false; }

    /// Hop (nach 118) anhängen; true, wenn frame() einen vollständigen Frame enthält
    bool push(const MicFrame& hop);
    const AnalysisFrame& frame() const { return frame_; }

private:
    AnalysisFrame frame_{};
    uint64_t hopTime_[HOPS_PER_FRAME] = {};   // Startzeit der Hops im Fenster (ältester zuerst)
    uint32_t fill_ = 0;                       // Hops im Fenster (0 .. HOPS_PER_FRAME)
    uint32_t lastHopId_ = 0;
    bool     haveLast_ = false;
    uint32_t nextFrameId_ = 0;
};

} // namespace sds110
