#pragma once

#include <cstdint>
#include <cstring>
#include "cmsis_os2.h"
#include "SDS_Params.hpp"

// ---------------------------------------------------------------------------
// Buffer states (deterministic triple-buffering)
// ---------------------------------------------------------------------------
enum BufferState : uint8_t {
    BUF_FREE    = 0,   // buffer available for writing
    BUF_WRITING = 1,   // acquisition task is filling the buffer
    BUF_READY   = 2,   // buffer contains a complete frame
    BUF_READING = 3    // DSP task is processing the buffer
};

// ---------------------------------------------------------------------------
// Unified microphone buffer:
// - raw  : int32_t TDM frames from ADAU7118
// - data : float samples for DSP (normalized)
// ---------------------------------------------------------------------------
struct UnifiedMicBuffer
{
    BufferState state;

    int32_t raw[SDS_NUM_MICS][SDS_BLOCK_SIZE];   // raw PCM24 samples
    float   data[SDS_NUM_MICS][SDS_BLOCK_SIZE];  // normalized float samples

    uint32_t writeIndex; // current write position (0..SDS_BLOCK_SIZE-1)
};

// ---------------------------------------------------------------------------
// Triple-buffer manager
// ---------------------------------------------------------------------------
class SDS_MicrophoneBuffer
{
public:
    // Singleton instance
    static SDS_MicrophoneBuffer& instance();

    // Acquire a free buffer for writing (MicTask / DMA completion)
    UnifiedMicBuffer* getFreeBuffer();

    // Acquire a ready buffer for reading (DSPTask / SRPTask)
    UnifiedMicBuffer* getReadableBuffer();

    // Get the currently active write buffer
    UnifiedMicBuffer* getActiveWriteBuffer();

    // Mark buffer as ready after writing
    void markReadable(UnifiedMicBuffer* b);

    // Mark buffer as free after DSP processing
    void markFree(UnifiedMicBuffer* b);

    // ISR-safe sample push (called from ADAU7118_OnSample)
    void pushSample(uint8_t ch, int32_t pcm24);

    // Wird aus DMA-Completion-Callback aufgerufen (Full Transfer Complete)
    void onDmaTransferComplete();

    // DMA buffer interface (pointer to raw[0][0] of active buffer)
    int32_t* rxBuffer();
    uint32_t rxBufferSize();

private:
    SDS_MicrophoneBuffer(); // constructor

    UnifiedMicBuffer buffers[3];       // triple-buffer array
    UnifiedMicBuffer* activeWriteBuffer;

    osMutexId_t mutex;                 // RTOS mutex for thread-safe access
};

// ---------------------------------------------------------------------------
// C-compatible interface for ADAU7118 driver
// ---------------------------------------------------------------------------
extern "C" int32_t* SDS_GetRxBuffer();
extern "C" uint32_t SDS_GetRxBufferSize();
extern "C" int SDS_GetNumMics();

// Called from ADAU7118 driver (if not using DMA completion)
extern "C" void ADAU7118_OnSample(uint8_t ch, int32_t pcm24);

// HAL-Driver interface
extern "C" void SDS_MicrophoneBuffer_onDmaTransferComplete(void);
