#include "SDS_MicrophoneBuffer.hpp"

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------
SDS_MicrophoneBuffer& SDS_MicrophoneBuffer::instance()
{
    static SDS_MicrophoneBuffer inst;
    return inst;
}

// ---------------------------------------------------------------------------
// Konstruktor
// ---------------------------------------------------------------------------
SDS_MicrophoneBuffer::SDS_MicrophoneBuffer()
    : activeWriteBuffer(nullptr)
{
    // Mutex anlegen
    osMutexAttr_t attr{};
    attr.name = "SDS_MicBufferMutex";
    mutex = osMutexNew(&attr);

    // Buffer initialisieren
    for (auto &b : buffers) {
        b.state = BUF_FREE;
        b.writeIndex = 0;

        std::memset(b.raw,  0, sizeof(b.raw));
        std::memset(b.data, 0, sizeof(b.data));
    }

    // Ersten Buffer als aktiven Schreib‑Buffer setzen
    activeWriteBuffer = &buffers[0];
    activeWriteBuffer->state = BUF_WRITING;
}

// ---------------------------------------------------------------------------
// Buffer holen: FREE → WRITING
// ---------------------------------------------------------------------------
UnifiedMicBuffer* SDS_MicrophoneBuffer::getFreeBuffer()
{
    osMutexAcquire(mutex, osWaitForever);

    UnifiedMicBuffer* res = nullptr;

    for (auto &b : buffers) {
        if (b.state == BUF_FREE) {
            b.state = BUF_WRITING;
            b.writeIndex = 0;
            std::memset(b.raw,  0, sizeof(b.raw));
            std::memset(b.data, 0, sizeof(b.data));
            res = &b;
            break;
        }
    }

    osMutexRelease(mutex);
    return res;
}

// ---------------------------------------------------------------------------
// Buffer holen: READY → READING
// ---------------------------------------------------------------------------
UnifiedMicBuffer* SDS_MicrophoneBuffer::getReadableBuffer()
{
    osMutexAcquire(mutex, osWaitForever);

    UnifiedMicBuffer* res = nullptr;

    for (auto &b : buffers) {
        if (b.state == BUF_READY) {
            b.state = BUF_READING;
            res = &b;
            break;
        }
    }

    osMutexRelease(mutex);
    return res;
}

// ---------------------------------------------------------------------------
// Aktiven Schreib‑Buffer zurückgeben
// ---------------------------------------------------------------------------
UnifiedMicBuffer* SDS_MicrophoneBuffer::getActiveWriteBuffer()
{
    return activeWriteBuffer;
}

// ---------------------------------------------------------------------------
// WRITING → READY
// ---------------------------------------------------------------------------
void SDS_MicrophoneBuffer::markReadable(UnifiedMicBuffer* b)
{
    osMutexAcquire(mutex, osWaitForever);

    if (b) {
        b->state = BUF_READY;
    }

    osMutexRelease(mutex);
}

// ---------------------------------------------------------------------------
// READING → FREE
// ---------------------------------------------------------------------------
void SDS_MicrophoneBuffer::markFree(UnifiedMicBuffer* b)
{
    osMutexAcquire(mutex, osWaitForever);

    if (b) {
        b->state = BUF_FREE;
        b->writeIndex = 0;
    }

    osMutexRelease(mutex);
}

// ---------------------------------------------------------------------------
// DMA‑Buffer: Pointer auf raw[0][0] des aktiven Buffers
// ---------------------------------------------------------------------------
int32_t* SDS_MicrophoneBuffer::rxBuffer()
{
    return &activeWriteBuffer->raw[0][0];
}

uint32_t SDS_MicrophoneBuffer::rxBufferSize()
{
    return SDS_NUM_MICS * SDS_BLOCK_SIZE;
}

// ---------------------------------------------------------------------------
// ISR‑safe Sample‑Pfad (für ADAU7118_OnSample)
// ---------------------------------------------------------------------------
void SDS_MicrophoneBuffer::pushSample(uint8_t ch, int32_t pcm24)
{
    UnifiedMicBuffer* b = activeWriteBuffer;
    if (!b) return;

    if (ch >= SDS_NUM_MICS) return;
    if (b->writeIndex >= SDS_BLOCK_SIZE) return;

    const uint32_t idx = b->writeIndex;

    // Rohdaten speichern
    b->raw[ch][idx] = pcm24;

    // Float konvertieren (24‑bit signed → float)
    constexpr float scale = 1.0f / (1 << 23);
    b->data[ch][idx] = static_cast<float>(pcm24) * scale;

    // Wenn letzter Kanal dieses Frames → Index erhöhen
    if (ch == (SDS_NUM_MICS - 1)) {
        b->writeIndex++;

        // Block voll?
        if (b->writeIndex >= SDS_BLOCK_SIZE) {

            // Buffer READY setzen
            markReadable(b);

            // Neuen FREE‑Buffer holen
            UnifiedMicBuffer* next = getFreeBuffer();
            if (next) {
                activeWriteBuffer = next;
            }
        }
    }
}

void SDS_MicrophoneBuffer::onDmaTransferComplete()
{
    UnifiedMicBuffer* b = activeWriteBuffer;
    if (!b) return;

    // Buffer READY setzen
    markReadable(b);

    // neuen FREE-Buffer holen
    UnifiedMicBuffer* next = getFreeBuffer();
    if (next) {
        activeWriteBuffer = next;
        activeWriteBuffer->state = BUF_WRITING;
        activeWriteBuffer->writeIndex = SDS_BLOCK_SIZE;
    }
}

// ---------------------------------------------------------------------------
// C‑Bridge
// ---------------------------------------------------------------------------
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
    SDS_MicrophoneBuffer::instance().pushSample(ch, pcm24);
}

extern "C" void SDS_MicrophoneBuffer_onDmaTransferComplete(void)
{
    SDS_MicrophoneBuffer::instance().onDmaTransferComplete();
}
