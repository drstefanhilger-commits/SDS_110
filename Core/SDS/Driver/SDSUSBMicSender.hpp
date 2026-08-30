#pragma once
#include <cstdint>
#include <cstring>
#include "usbd_cdc_if.h"
#include "SDS_Structs.hpp"
#include "SDS_MicrophoneBuffer.hpp"



class SDS_USB_MicSender
{
public:
    SDS_USB_MicSender() : frameCounter(0) {}

    bool send(const UnifiedMicBuffer* rb)
    {
    	struct SDS_MsgReadXXX {
    	    uint32_t  magic 	= 0xDEADBEEF;
    	    uint8_t	  id		= 2;					// READ_ID
    	    uint8_t   len[3]    = {0x00, 0x04, 0x18}; 	// 4 + 1 + 3 + 4 + 4 + 4 + 1024 + 4 + 4 = 1048
    	    uint32_t  timestamp = 0; 					// TimeStamp
    	    uint32_t  frameNumber = 0;
    	    uint32_t  micNumber = 0;					// MicNumber
    	    float 	  payload[256];
    	    uint32_t  crc32;
    	};

//    	struct SDS_MsgReadXXX {
//    	    uint32_t  magic 	= 0xDEADBEEF;
//    	    uint8_t	  id		= 2;					// READ_ID
//    	    uint8_t   len[3]    = {0x00, 0x00, 0x90}; 	// 8220
//    	    uint32_t  timestamp = 0; 					// TimeStamp
//    	    float 	  payload[32];
//    	    uint32_t  crc32;
//    	};
    	SDS_MsgReadXXX msg;

    	if (!rb) return false;


        // FrameNr
//        msg.header.frameIndex = frameCounter++;
        // Payload

//        memcpy(msgXXX.payload,
//               rb->data,
//               sizeof(128));
//        msgXXX.id = 2;
//        // CRC
//        msgXXX.crc32 = 0xabcdefff;
//        		computeCRC32(
//            reinterpret_cast<const uint8_t*>(&msgXXX),
//            sizeof(SDS_MsgRead)
//        );
//    	SDS_MsgDetect msg;
    	msg.magic 		= 0xDEADBEEF;
    	msg.id 			= 2;
    	msg.len[0] 		= 0x00;
    	msg.len[1]		= 0x04;
    	msg.len[2]		= 0x18;
    	msg.timestamp	= 0;
    	for (int i=0; i<256; i++) {
    		msg.payload[i] 	= (float)(i);
    	}
        msg.crc32		= 0xabcdefff;
        uint8_t status =  CDC_Transmit_FS((uint8_t*)(&msg), sizeof(msg));


        // USB transmit
//        auto status = CDC_Transmit_FS(
//            reinterpret_cast<uint8_t*>(&msgXXX),
//            sizeof(SDS_MsgRead)
//        );

        return (status == USBD_OK);
    }


//    bool send(const MicBuffer* rb)
//    {
//        if (!rb) return false;
//
//        SDS_MicFrameMsg msg;
//
//        // Header
//        msg.header.sof        = 0xA5;
//        msg.header.msgType    = static_cast<uint8_t>(SDS_MsgType::READ_MICS);
//        msg.header.version    = 1;
//        msg.header.frameIndex = frameCounter++;
//        msg.header.numMics    = SDS_NUM_MICS;
//        msg.header.frameLen   = SDS_BLOCK_SIZE;
//        msg.header.payloadLen = SDS_NUM_MICS * SDS_BLOCK_SIZE * sizeof(float);
//
//        // Payload
//        memcpy(msg.payload.micData,
//               rb->data,
//               sizeof(msg.payload.micData));
//
//        // CRC
//        msg.crc32 = computeCRC32(
//            reinterpret_cast<const uint8_t*>(&msg),
//            sizeof(SDS_MsgHeader) + sizeof(SDS_MicPayload)
//        );
//
//        // USB transmit
//        auto status = CDC_Transmit_FS(
//            reinterpret_cast<uint8_t*>(&msg),
//            sizeof(msg)
//        );
//
//        return (status == USBD_OK);
//    }

private:
//    SDS_MsgHeader1 msg;

    uint32_t frameCounter;

    uint32_t computeCRC32(const uint8_t* data, size_t len)
    {
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < len; i++) {
            crc ^= data[i];
            for (int j = 0; j < 8; j++) {
                uint32_t mask = -(crc & 1);
                crc = (crc >> 1) ^ (0xEDB88320 & mask);
            }
        }
        return ~crc;
    }
};
