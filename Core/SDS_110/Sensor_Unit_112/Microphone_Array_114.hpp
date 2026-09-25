/*
 * Microphone_Array_114.hpp
 *
 * Mikrofonarray 114 (Patent, Abschnitt 1, FIG. 1/2):
 *  - Geometrie: M = 8 IM69D130 im regelmäßigen Oktagon, Radius MIC_RADIUS_M
 *  - Frame-Puffer: Triple-Buffering (FREE -> WRITING -> READY -> READING),
 *    Frames von FRAME_SAMPLES pro Mikrofon als normalisierte float.
 *
 * Migration aus SDS:
 *  - Model/SDS_Params.hpp      : SDS_MIC_POSITIONS, SDS_MIC_RADIUS
 *  - Model/SDS_MicrophoneBuffer: UnifiedMicBuffer, Zustandsautomat, Mutex
 *  Geändert: Blockweise Übernahme aus dem DMA-Puffer (pushBlock) statt
 *  Sample-für-Sample (pushSample); raw-int32-Kopie entfällt (nur noch im
 *  DMA-Ping-Pong-Puffer von 116). Frames liegen im SDRAM.
 */
#pragma once
#include <cstdint>
#include "cmsis_os2.h"
#include "SDS_110_Config.hpp"

namespace sds110 {

struct Vec3 { float x, y, z; };

enum class FrameState : uint8_t { Free = 0, Writing, Ready, Reading };

struct MicFrame {
    FrameState state;
    uint32_t   frame_id;
    uint64_t   time_utc_us;                     // Zeitreferenz des ersten Samples
    uint32_t   writeIndex;                      // 0 .. FRAME_SAMPLES
    float      data[NUM_MICS][FRAME_SAMPLES];   // normalisiert [-1, 1)
};

class Microphone_Array_114 {
public:
    static Microphone_Array_114& instance();

    // --- Geometrie ---------------------------------------------------
    const Vec3& position(uint32_t mic) const { return pos_[mic]; }
    const Vec3* positions() const { return pos_; }

    // --- Schreibseite (116, ISR-Kontext) -----------------------------
    /// Block von DMA_BLOCK_SAMPLES Samples je Mikrofon, interleaved (s*M + ch),
    /// 24-bit PCM linksbündig in int32 (pcm24 << 8, siehe PCM_RAW_FULL_SCALE).
    /// Schließt bei vollem Frame ab und wechselt Puffer.
    void pushBlock(const int32_t* interleaved, uint32_t samplesPerMic, uint64_t time_utc_us);

    // --- Leseseite (118/122, Task-Kontext) ---------------------------
    MicFrame* acquireReadable();     // READY -> READING, nullptr wenn keiner
    void      release(MicFrame* f);  // READING -> FREE
    const MicFrame* latestFrame() const { return latest_; }

    // --- Statistik ---------------------------------------------------
    uint32_t droppedFrames() const { return dropped_; }

private:
    Microphone_Array_114();
    MicFrame* acquireFree();         // FREE -> WRITING (ISR-sicher: ohne Mutex)

    Vec3       pos_[NUM_MICS];
    MicFrame*  active_   = nullptr;  // aktueller Schreibpuffer
    MicFrame*  latest_   = nullptr;  // zuletzt fertiggestellter Frame
    uint32_t   nextId_   = 0;
    uint32_t   dropped_  = 0;
    osMutexId_t mutex_   = nullptr;  // schützt Leseseite

    static MicFrame frames_[NUM_MIC_FRAMES];
};

} // namespace sds110
