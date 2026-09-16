/*
 * Microphone_Array_114.cpp
 */
#include "Microphone_Array_114.hpp"
#include <cmath>
#include <cstring>

namespace sds110 {

SDS110_SDRAM_SECTION MicFrame Microphone_Array_114::frames_[NUM_MIC_FRAMES];

Microphone_Array_114& Microphone_Array_114::instance()
{
    static Microphone_Array_114 inst;
    return inst;
}

Microphone_Array_114::Microphone_Array_114()
{
    // Oktagon: Mic m bei Winkel m * 45°, gegen den Uhrzeigersinn ab +x
    for (uint32_t m = 0; m < NUM_MICS; ++m) {
        const float a = static_cast<float>(m) * (2.0f * 3.14159265f / NUM_MICS);
        pos_[m] = { MIC_RADIUS_M * std::cos(a), MIC_RADIUS_M * std::sin(a), 0.0f };
    }

    osMutexAttr_t attr{};
    attr.name = "Mic114Mutex";
    mutex_ = osMutexNew(&attr);

    for (auto& f : frames_) {
        f.state = FrameState::Free;
        f.writeIndex = 0;
        f.frame_id = 0;
        f.time_utc_us = 0;
        std::memset(f.data, 0, sizeof(f.data));
    }
    active_ = acquireFree();
}

MicFrame* Microphone_Array_114::acquireFree()
{
    for (auto& f : frames_) {
        if (f.state == FrameState::Free) {
            f.state = FrameState::Writing;
            f.writeIndex = 0;
            f.frame_id = nextId_++;
            return &f;
        }
    }
    return nullptr;
}

void Microphone_Array_114::pushBlock(const int32_t* interleaved, uint32_t samplesPerMic, uint64_t time_utc_us)
{
    MicFrame* f = active_;
    if (!f) {
        // Kein freier Puffer: Block verwerfen, später erneut versuchen
        active_ = acquireFree();
        ++dropped_;
        return;
    }
    if (f->writeIndex == 0) f->time_utc_us = time_utc_us;

    constexpr float scale = 1.0f / static_cast<float>(1 << 23);   // 24-bit signed -> float
    uint32_t idx = f->writeIndex;

    for (uint32_t s = 0; s < samplesPerMic && idx < FRAME_SAMPLES; ++s, ++idx) {
        const int32_t* row = interleaved + s * NUM_MICS;
        for (uint32_t ch = 0; ch < NUM_MICS; ++ch)
            f->data[ch][idx] = static_cast<float>(row[ch]) * scale;
    }
    f->writeIndex = idx;

    if (idx >= FRAME_SAMPLES) {
        f->state = FrameState::Ready;
        latest_  = f;
        active_  = acquireFree();          // nullptr -> nächster Block wird verworfen
    }
}

MicFrame* Microphone_Array_114::acquireReadable()
{
    osMutexAcquire(mutex_, osWaitForever);
    MicFrame* res = nullptr;
    // ältesten READY-Frame nehmen
    for (auto& f : frames_) {
        if (f.state == FrameState::Ready && (!res || f.frame_id < res->frame_id))
            res = &f;
    }
    if (res) res->state = FrameState::Reading;
    osMutexRelease(mutex_);
    return res;
}

void Microphone_Array_114::release(MicFrame* f)
{
    if (!f) return;
    osMutexAcquire(mutex_, osWaitForever);
    f->state = FrameState::Free;
    f->writeIndex = 0;
    osMutexRelease(mutex_);
}

} // namespace sds110
