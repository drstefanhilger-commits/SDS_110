/*
 * USBDriver.cpp  (Infrastructure/Driver)
 */
#include "USBDriver.hpp"
#include "Infrastructure/Utils/crc32.hpp"
#include "usbd_cdc_if.h"
#include <cstring>

namespace sds110 {

bool USBDriver::transmit(const void* buf, uint16_t len)
{
    return CDC_Transmit_FS(reinterpret_cast<uint8_t*>(const_cast<void*>(buf)), len) == USBD_OK;
}

bool USBDriver::sendDetection(uint32_t timestamp, uint32_t micId, float azimuth, float distance, float confidence)
{
    SDS_MsgDetect msg;
    msg.timestamp = timestamp;
    msg.mic       = micId;
    msg.azi       = azimuth;
    msg.distance  = distance;
    msg.conf      = confidence;
    msg.crc32     = CRC32::compute_no_crc(msg);
    return transmit(&msg, sizeof(msg));
}

bool USBDriver::sendRead(uint32_t timestamp, uint32_t micNr, uint32_t frameNr, const MicFrame* frame)
{
    SDS_MsgRead msg;
    msg.timestamp = timestamp;
    msg.micNr     = static_cast<uint16_t>(micNr);
    msg.frameNr   = static_cast<uint16_t>(frameNr);
    const uint32_t offset = frameNr * SDS_MSG_BUFFER_SIZE;
    constexpr float toPcm24 = static_cast<float>(1 << 23);
    for (uint32_t i = 0; i < SDS_MSG_BUFFER_SIZE; ++i) {
        const uint32_t idx = offset + i;
        if (frame && micNr < NUM_MICS && idx < FRAME_SAMPLES)
            msg.data[i] = static_cast<uint32_t>(static_cast<int32_t>(frame->data[micNr][idx] * toPcm24));
        else
            msg.data[i] = 0;
    }
    msg.crc32 = CRC32::compute_no_crc(msg);
    return transmit(&msg, sizeof(msg));
}

bool USBDriver::sendLogging(uint32_t timestamp, const uint8_t* src, int len)
{
#pragma pack(push, 1)
    struct SDS_MsgLog {
        uint32_t magic     = 0xDEADBEEF;
        uint8_t  id        = 3;
        uint8_t  len[3]    = {0x30, 0x00, 0x00};   // 48 Byte
        uint32_t timestamp = 0;
        uint8_t  data[32]  = {};
        uint32_t crc32     = 0;
    };
#pragma pack(pop)
    static_assert(sizeof(SDS_MsgLog) == 48, "SDS_MsgLog must be 48 bytes");
    SDS_MsgLog msg;
    msg.timestamp = timestamp;
    const int n = (len < 32) ? len : 32;
    if (n > 0) memcpy(msg.data, src, n);
    msg.crc32 = CRC32::compute_no_crc(msg);
    return transmit(&msg, sizeof(msg));
}

bool USBDriver::sendMessage(uint32_t id, uint32_t timestamp, const MessageData& data)
{
    Message msg;
    msg.len_id    = ((sizeof(Message) & 0x00FFFFFF) | (id << 24));
    msg.timestamp = timestamp;
    msg.data      = data;
    msg.crc32     = CRC32::compute_no_crc(msg);
    return transmit(&msg, sizeof(msg));
}

} // namespace sds110
