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
 *
 * Speicher: HOPS_PER_FRAME Hop-Slots im Ring; der Frame zeigt auf die Slots in zeitlicher
 * Reihenfolge. Ein neuer Hop überschreibt den ältesten Slot, es wird nichts verschoben.
 */
#pragma once
#include <cstdint>
#include "SDS_110_Config.hpp"
#include "Microphone_Array_114.hpp"

namespace sds110 {

/// Analyse-Frame als Sicht auf zwei Hop-Slots (älterer, neuerer Hop, je HOP_SAMPLES Samples).
/// Kein zusammenhängender Puffer mehr: das Schieben um einen Hop (memmove) und die Kopie jedes Hops
/// kosteten am Board 2,6 ms je Hop im SDRAM. 122 fenstert die beiden Teile getrennt (bitgleich).
struct AnalysisFrame {
    static constexpr uint32_t PARTS = FRAME_SAMPLES / HOP_SAMPLES;
    uint32_t     frame_id;                    // fortlaufend je vollständigem Frame
    uint64_t     time_utc_us;                 // Zeit des ersten Samples im Frame
    const float* part[NUM_MICS][PARTS];       // part[ch][0] älterer Hop, part[ch][1] neuerer
    float sample(uint32_t ch, uint32_t i) const { return part[ch][i / HOP_SAMPLES][i % HOP_SAMPLES]; }
};

class Frame_Assembler {
public:
    static constexpr uint32_t HOPS_PER_FRAME = AnalysisFrame::PARTS;
    static_assert(HOPS_PER_FRAME * HOP_SAMPLES == FRAME_SAMPLES, "Frame = ganze Zahl von Hops");

    void reset() { fill_ = 0; haveLast_ = false; }

    /// Hop (nach 118) anhängen: Samples in den nächsten Slot kopieren; true, wenn frame() einen
    /// vollständigen Frame enthält
    bool push(const MicFrame& hop);

    /// Ohne Kopie (Board, Sensor_Unit_112): beginHop() liefert je Kanal den Zielpuffer im nächsten
    /// Slot, 118 schreibt dort hinein, commitHop() schließt den Hop ab (Rückgabe wie push()).
    void beginHop(const MicFrame& hop, float* dst[NUM_MICS]);
    bool commitHop(const MicFrame& hop);

    const AnalysisFrame& frame() const { return frame_; }

private:
    // Slots mit Zeilenfüllung wie 114 (Befund 42: Kanäle nicht im selben D-Cache-Satz)
    float    slot_[HOPS_PER_FRAME][NUM_MICS][HOP_SAMPLES + MIC_ROW_PAD];
    uint32_t order_[HOPS_PER_FRAME];          // Slot-Indizes, ältester Hop zuerst
    uint32_t cur_ = 0;                        // Slot des Hops zwischen beginHop und commitHop
    AnalysisFrame frame_{};
    uint64_t hopTime_[HOPS_PER_FRAME] = {};   // Startzeit der Hops im Fenster (ältester zuerst)
    uint32_t fill_ = 0;                       // Hops im Fenster (0 .. HOPS_PER_FRAME)
    uint32_t lastHopId_ = 0;
    bool     haveLast_ = false;
    uint32_t nextFrameId_ = 0;
public:
    Frame_Assembler() { for (uint32_t k = 0; k < HOPS_PER_FRAME; ++k) order_[k] = k; }
};

} // namespace sds110
