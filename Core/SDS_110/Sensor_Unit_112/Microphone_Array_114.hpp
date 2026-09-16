/*
 * Microphone_Array_114.hpp
 * Mikrofonarray 114: M = 8 IM69D130 im regelmäßigen Oktagon.
 * Hält Geometrie und den aktuellen synchronen Frame.
 * Migration: Model/SDS_MicrophoneBuffer, SRPPhat::initGeometryFromSDS
 */
#pragma once
#include <cstdint>
#include "SDS_110_Config.hpp"

namespace sds110 {

struct Vec3 { float x, y, z; };

struct MicFrame {
    float    data[NUM_MICS][N_FFT];
    uint64_t time_utc_us;
    uint32_t frame_id;
};

class Microphone_Array_114 {
public:
    static Microphone_Array_114& instance();
    const Vec3& position(uint32_t mic) const { return pos_[mic]; }
    MicFrame*   acquireFrame();            // Puffer zum Schreiben (116)
    const MicFrame* latestFrame() const;   // für 118 / 122
private:
    Microphone_Array_114();
    Vec3 pos_[NUM_MICS];
};

} // namespace sds110
