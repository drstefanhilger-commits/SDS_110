/*
 * SDS_MicrophoneBuffer.hpp
 *
 * Deterministic Triple‑Buffer System for 8‑Channel Microphone Arrays
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <atomic>
#include "cmsis_os2.h"
#include "SDS_Params.hpp"

// ---------------------------------------------------------------------------
// Buffer states
// ---------------------------------------------------------------------------
enum BufferState : uint8_t {
    BUF_FREE    = 0,
    BUF_WRITING = 1,
    BUF_READY   = 2,
    BUF_READING = 3
};

// ---------------------------------------------------------------------------
// Triple‑buffer (float)
// ---------------------------------------------------------------------------
struct MicBuffer {
    BufferState state;
    float data[SDS_NUM_MICS][SDS_BLOCK_SIZE];
    uint32_t writeIndex;
};

// ---------------------------------------------------------------------------
// Manager
// ---------------------------------------------------------------------------
class SDS_MicrophoneBuffer
{
public:
    static SDS_MicrophoneBuffer& instance();

    MicBuffer* getFreeBuffer();
    MicBuffer* getReadableBuffer();
    void markReadable(MicBuffer* b);
    void markFree(MicBuffer* b);

    // ISR‑safe entry point
    void pushSample(uint8_t ch, float sample);

    // DMA buffer for ADAU7118 (int32_t TDM frames)
    int32_t* rxBuffer();
    uint32_t rxBufferSize();

private:
    SDS_MicrophoneBuffer();

    MicBuffer buffers[3];
    osMutexId_t mutex;

    MicBuffer* activeWriteBuffer;

    // NEW: DMA buffer for ADAU7118
    int32_t dmaBuffer[SDS_NUM_MICS * SDS_BLOCK_SIZE];
};

// ---------------------------------------------------------------------------
// C‑bridge for ADAU7118 driver
// ---------------------------------------------------------------------------
extern "C" int32_t* SDS_GetRxBuffer();
extern "C" uint32_t SDS_GetRxBufferSize();
extern "C" int SDS_GetNumMics();

// Called from ADAU7118 driver
extern "C" void ADAU7118_OnSample(uint8_t ch, int32_t pcm24);
