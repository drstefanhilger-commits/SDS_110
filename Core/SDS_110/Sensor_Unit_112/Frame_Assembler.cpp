/*
 * Frame_Assembler.cpp
 */
#include "Frame_Assembler.hpp"
#include <cstring>

namespace sds110 {

bool Frame_Assembler::push(const MicFrame& hop)
{
    if (haveLast_ && hop.frame_id != lastHopId_ + 1) fill_ = 0;   // Lücke: neu beginnen
    lastHopId_ = hop.frame_id; haveLast_ = true;

    constexpr uint32_t keep = FRAME_SAMPLES - HOP_SAMPLES;
    if (fill_ == HOPS_PER_FRAME) {                                // Fenster voll: um einen Hop schieben
        for (uint32_t ch = 0; ch < NUM_MICS; ++ch)
            std::memmove(frame_.data[ch], frame_.data[ch] + HOP_SAMPLES, sizeof(float) * keep);
        for (uint32_t k = 0; k + 1 < HOPS_PER_FRAME; ++k) hopTime_[k] = hopTime_[k + 1];
        --fill_;
    }
    // Hop an Position fill_ (bis zum Füllen) bzw. ans Ende
    const uint32_t off = fill_ * HOP_SAMPLES;
    for (uint32_t ch = 0; ch < NUM_MICS; ++ch)
        std::memcpy(frame_.data[ch] + off, hop.data[ch], sizeof(float) * HOP_SAMPLES);
    hopTime_[fill_] = hop.time_utc_us;
    ++fill_;

    if (fill_ < HOPS_PER_FRAME) return false;
    frame_.time_utc_us = hopTime_[0];
    frame_.frame_id    = nextFrameId_++;
    return true;
}

} // namespace sds110
