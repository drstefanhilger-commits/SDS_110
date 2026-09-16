/*
 * USBDriver.cpp
 *
 *  Created on: Aug 18, 2026
 *      Author: 310004
 */

#include "USBDriver.hpp"
#include "crc32.hpp"
#include <cstring>

// ------------------------------------------------------------
// SEND DETECTION (32 bytes)
// ------------------------------------------------------------
bool USB_SendDetection(uint32_t timestamp,
                       uint32_t micId,
                       float azimuth,
                       float distance,
                       float confidence)
{
    static_assert(sizeof(SDS_MsgDetect) == 32,
                  "SDS_MsgDetect must be 32 bytes");

    SDS_MsgDetect msg;

    msg.timestamp = timestamp;
    msg.mic       = micId;
    msg.azi       = azimuth;
    msg.distance  = distance;
    msg.conf      = confidence;

    // CRC over all bytes except crc32
    msg.crc32 = CRC32::computeCRC32(
                    reinterpret_cast<const uint8_t*>(&msg),
                    sizeof(SDS_MsgDetect) - sizeof(uint32_t));

    uint8_t status = CDC_Transmit_FS(
                        reinterpret_cast<uint8_t*>(&msg),
                        sizeof(msg));

    return (status == USBD_OK);
}


// ------------------------------------------------------------
// SEND READ (532 bytes)
// ------------------------------------------------------------
bool USB_SendRead(uint32_t timestamp,
                  uint32_t micNr,
                  uint32_t frameNr,
                  UnifiedMicBuffer* rb)
{
    static_assert(sizeof(SDS_MsgRead) == 532,
                  "SDS_MsgRead must be 532 bytes");

    SDS_MsgRead msg;

    int offset = frameNr*SDS_MSG_BUFFER_SIZE;
	for (int i = 0; i < SDS_MSG_BUFFER_SIZE; i++) {
		if (rb && micNr < SDS_NUM_MICS) {
			msg.data[i] = static_cast<uint32_t>(rb->raw[micNr][i+offset]);
		} else {
			msg.data[i] = 0;
		}
	}

    msg.crc32 = CRC32::computeCRC32(
                    reinterpret_cast<const uint8_t*>(&msg),
                    sizeof(SDS_MsgRead) - sizeof(uint32_t));

    uint8_t status = CDC_Transmit_FS(
                        reinterpret_cast<uint8_t*>(&msg),
                        sizeof(msg));

    return (status == USBD_OK);
}

bool USB_SendRead_Test()
{
    SDS_MsgRead msg;

    msg.magic = 0xDEADBEEF;

    msg.timestamp = 12345678;
    msg.micNr = 0;
    msg.frameNr = 0;

    // Payload = 128 × uint32_t = 0
    memset(msg.data, 0, sizeof(msg.data));

    // CRC32 über alles außer das CRC-Feld
    msg.crc32 = CRC32::computeCRC32(
        reinterpret_cast<uint8_t*>(&msg),
        sizeof(SDS_MsgRead) - sizeof(uint32_t)
    );

    uint8_t status = CDC_Transmit_FS(reinterpret_cast<uint8_t*>(&msg), sizeof(msg));
    return (status == USBD_OK);
}


// ------------------------------------------------------------
// SEND LOGGING (48 bytes)
// ------------------------------------------------------------
bool USB_SendLogging(uint32_t timestamp, uint8_t* dst, int maxLen)
{
    struct SDS_MsgUSB {
        uint32_t magic     = 0xDEADBEEF;
        uint8_t  id        = 3;                 // Logging
        uint8_t  len[3]    = {0x30, 0x00, 0x00}; // 48 bytes
        uint32_t timestamp = 0;
        uint8_t  data[32];
        uint32_t crc32;
    };

    static_assert(sizeof(SDS_MsgUSB) == 48,
                  "SDS_MsgUSB must be 48 bytes");

    SDS_MsgUSB msg;

    msg.timestamp = timestamp;

    int copyLen = (maxLen < 32 ? maxLen : 32);
    memcpy(msg.data, dst, copyLen);

    // CRC over all bytes except crc32
    msg.crc32 = CRC32::computeCRC32(
                    reinterpret_cast<const uint8_t*>(&msg),
                    sizeof(SDS_MsgUSB) - sizeof(uint32_t));

    uint8_t status = CDC_Transmit_FS(
                        reinterpret_cast<uint8_t*>(&msg),
                        sizeof(msg));

    return (status == USBD_OK);
}


// ------------------------------------------------------------
// SEND LOGGING with union data
// ------------------------------------------------------------
bool SDS_SendMessage(uint32_t id, uint32_t timestamp, MessageData data, uint32_t len) {
	Message		msg;
    msg.timestamp   = timestamp;
    msg.data  		= data;
    msg.crc32		= 0;

	auto status = CDC_Transmit_FS(reinterpret_cast<uint8_t*>(&msg), sizeof(msg));
	return (status == USBD_OK);
}


