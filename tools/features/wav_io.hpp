/*
 * wav_io.hpp – minimaler WAV-Leser für das Merkmalswerkzeug (PCM 16/24/32 Bit, float32,
 * WAVE_FORMAT_EXTENSIBLE). Liefert je Kanal die Samples als 24-bit-PCM-Werte (−2^23 … 2^23−1),
 * so wie sie der ADAU7118 liefert; float wird auf [−1, 1) begrenzt und auf 24 Bit gerundet.
 */
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>

struct WavData {
    uint32_t sampleRate = 0;
    uint16_t channels = 0;
    uint16_t bits = 0;
    bool     isFloat = false;
    std::vector<std::vector<int32_t>> pcm24;   // [Kanal][Sample]
    std::string error;
};

inline WavData readWav(const std::string& path)
{
    WavData w;
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) { w.error = "Datei nicht lesbar"; return w; }
    std::vector<uint8_t> buf;
    { uint8_t tmp[65536]; size_t n; while ((n = std::fread(tmp, 1, sizeof(tmp), f)) > 0) buf.insert(buf.end(), tmp, tmp + n); }
    std::fclose(f);
    auto u16 = [&](size_t o) { return uint16_t(buf[o] | (buf[o + 1] << 8)); };
    auto u32 = [&](size_t o) { return uint32_t(buf[o] | (buf[o + 1] << 8) | (buf[o + 2] << 16) | (uint32_t(buf[o + 3]) << 24)); };
    if (buf.size() < 12 || std::memcmp(&buf[0], "RIFF", 4) || std::memcmp(&buf[8], "WAVE", 4)) { w.error = "kein RIFF/WAVE"; return w; }
    size_t pos = 12, dataOff = 0, dataLen = 0; uint16_t fmt = 0; bool haveFmt = false;
    while (pos + 8 <= buf.size()) {
        const uint32_t len = u32(pos + 4); const size_t body = pos + 8;
        if (!std::memcmp(&buf[pos], "fmt ", 4) && body + 16 <= buf.size()) {
            fmt = u16(body); w.channels = u16(body + 2); w.sampleRate = u32(body + 4); w.bits = u16(body + 14);
            if (fmt == 0xFFFE && len >= 40) fmt = u16(body + 24);          // Subformat-GUID: erste 2 Byte
            haveFmt = true;
        } else if (!std::memcmp(&buf[pos], "data", 4)) {
            dataOff = body; dataLen = std::min<size_t>(len, buf.size() - body); break;
        }
        pos = body + len + (len & 1);
    }
    if (!haveFmt || !dataOff) { w.error = "fmt- oder data-Chunk fehlt"; return w; }
    w.isFloat = (fmt == 3);
    if (!((fmt == 1 && (w.bits == 16 || w.bits == 24 || w.bits == 32)) || (fmt == 3 && w.bits == 32))) {
        w.error = "Format nicht unterstützt (fmt " + std::to_string(fmt) + ", " + std::to_string(w.bits) + " Bit)"; return w;
    }
    const size_t bps = w.bits / 8, frames = dataLen / (bps * w.channels);
    w.pcm24.assign(w.channels, std::vector<int32_t>(frames));
    for (size_t i = 0; i < frames; ++i)
        for (uint16_t c = 0; c < w.channels; ++c) {
            const size_t o = dataOff + (i * w.channels + c) * bps; int32_t v;
            if (w.isFloat) {
                float x; std::memcpy(&x, &buf[o], 4);
                double d = std::round(double(x) * 8388608.0);
                v = int32_t(std::fmin(std::fmax(d, -8388608.0), 8388607.0));
            } else if (w.bits == 16) v = int32_t(int16_t(u16(o))) * 256;                              // 16 -> 24 Bit
            else if (w.bits == 24) { v = int32_t(buf[o] | (buf[o + 1] << 8) | (buf[o + 2] << 16)); if (v & 0x800000) v -= 0x1000000; }
            else v = int32_t(u32(o)) >> 8;                                                            // 32 -> 24 Bit
            w.pcm24[c][i] = v;
        }
    return w;
}
