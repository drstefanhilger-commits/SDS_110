/*
 * Frame_Assembler.cpp
 */
#include "Frame_Assembler.hpp"
#include <cstring>

namespace sds110 {

void Frame_Assembler::beginHop(const MicFrame& hop, float* dst[NUM_MICS])
{
    if (haveLast_ && hop.frame_id != lastHopId_ + 1) fill_ = 0;   // Lücke: neu beginnen
    if (fill_ == HOPS_PER_FRAME) {                                // Fenster voll: ältester Slot wird frei
        const uint32_t oldest = order_[0];
        for (uint32_t k = 0; k + 1 < HOPS_PER_FRAME; ++k) { order_[k] = order_[k + 1]; hopTime_[k] = hopTime_[k + 1]; }
        order_[HOPS_PER_FRAME - 1] = oldest;
        --fill_;
    }
    cur_ = order_[fill_];
    for (uint32_t ch = 0; ch < NUM_MICS; ++ch) dst[ch] = slot_[cur_][ch];
}

bool Frame_Assembler::commitHop(const MicFrame& hop)
{
    lastHopId_ = hop.frame_id; haveLast_ = true;
    hopTime_[fill_] = hop.time_utc_us;
    ++fill_;
    if (fill_ < HOPS_PER_FRAME) return false;
    for (uint32_t ch = 0; ch < NUM_MICS; ++ch)
        for (uint32_t k = 0; k < HOPS_PER_FRAME; ++k) frame_.part[ch][k] = slot_[order_[k]][ch];
    frame_.time_utc_us = hopTime_[0];
    frame_.frame_id    = nextFrameId_++;
    return true;
}

bool Frame_Assembler::push(const MicFrame& hop)
{
    float* dst[NUM_MICS];
    beginHop(hop, dst);
    for (uint32_t ch = 0; ch < NUM_MICS; ++ch)
        std::memcpy(dst[ch], hop.data[ch], sizeof(float) * HOP_SAMPLES);
    return commitHop(hop);
}

} // namespace sds110
