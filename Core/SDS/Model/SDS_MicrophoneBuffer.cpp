#include "SDS_MicrophoneBuffer.hpp"

SDS_MicrophoneBuffer& SDS_MicrophoneBuffer::instance()
{
    static SDS_MicrophoneBuffer inst;
    return inst;
}

SDS_MicrophoneBuffer::SDS_MicrophoneBuffer()
{
    mutex = osMutexNew(nullptr);

    for (int i = 0; i < 3; i++) {
        buffers[i].state = BUF_FREE;
        buffers[i].writeIndex = 0;
        memset(buffers[i].data, 0, sizeof(buffers[i].data));
    }

    memset(dmaBuffer, 0, sizeof(dmaBuffer));

    activeWriteBuffer = &buffers[0];
}

MicBuffer* SDS_MicrophoneBuffer::getFreeBuffer()
{
    for (int i = 0; i < 3; i++) {
        if (buffers[i].state == BUF_FREE) {
            buffers[i].state = BUF_WRITING;
            buffers[i].writeIndex = 0;
            return &buffers[i];
        }
    }
    return nullptr;
}

MicBuffer* SDS_MicrophoneBuffer::getReadableBuffer()
{
    for (int i = 0; i < 3; i++) {
        if (buffers[i].state == BUF_READY) {
            buffers[i].state = BUF_READING;
            return &buffers[i];
        }
    }
    return nullptr;
}

void SDS_MicrophoneBuffer::markReadable(MicBuffer* b)
{
    osMutexAcquire(mutex, osWaitForever);
    b->state = BUF_READY;
    osMutexRelease(mutex);
}

void SDS_MicrophoneBuffer::markFree(MicBuffer* b)
{
    osMutexAcquire(mutex, osWaitForever);
    b->state = BUF_FREE;
    osMutexRelease(mutex);
}

void SDS_MicrophoneBuffer::pushSample(uint8_t ch, float sample)
{
    MicBuffer* b = activeWriteBuffer;

    if (b->writeIndex < SDS_BLOCK_SIZE) {
        b->data[ch][b->writeIndex] = sample;

        if (ch == SDS_NUM_MICS - 1) {
            b->writeIndex++;

            if (b->writeIndex >= SDS_BLOCK_SIZE) {
                markReadable(b);
                activeWriteBuffer = getFreeBuffer();
            }
        }
    }
}

// DMA buffer access
int32_t* SDS_MicrophoneBuffer::rxBuffer()
{
    return dmaBuffer;
}

uint32_t SDS_MicrophoneBuffer::rxBufferSize()
{
    return SDS_NUM_MICS * SDS_BLOCK_SIZE;
}

// C‑bridge
extern "C" int32_t* SDS_GetRxBuffer()
{
    return SDS_MicrophoneBuffer::instance().rxBuffer();
}

extern "C" uint32_t SDS_GetRxBufferSize()
{
    return SDS_MicrophoneBuffer::instance().rxBufferSize();
}

extern "C" int SDS_GetNumMics()
{
    return SDS_NUM_MICS;
}

extern "C" void ADAU7118_OnSample(uint8_t ch, int32_t pcm24)
{
    float f = static_cast<float>(pcm24) / 8388608.0f; // 24‑bit normalization
    SDS_MicrophoneBuffer::instance().pushSample(ch, f);
}
