#pragma once
#include <cstdint>
#include <cstring>
#include "usbd_cdc_if.h"
#include "SDS_Structs.hpp"
#include "SDS_MicrophoneBuffer.hpp"
#include "crc32.hpp"


class SDS_USB_MicSender
{
public:
    SDS_USB_MicSender() : frameCounter(0) {}

	static constexpr int BUFFER_SIZE = 16;

    bool send(const UnifiedMicBuffer* rb)
    {
//    	if (!rb) return false;

    	struct SDS_MsgReadXXX {
    	    uint32_t  magic 	= 0xDEADBEEF;
    	    uint8_t	  id		= 2;					// READ_ID
    	    uint8_t   len[3]    = {0x00, 0x04, 0x18}; 	// 4 + 1 + 3 + 4 + 4 + 4 + 1024 + 4 + 4 = 1048
    	    uint32_t  timestamp = 0; 					// TimeStamp
    	    uint32_t  frameNumber = 0;
    	    uint32_t  micNumber = 0;					// MicNumber
    	    float 	  payload[BUFFER_SIZE];
    	    uint32_t  crc32;
    	};

    	SDS_MsgReadXXX msg;
    	msg.magic 		= 0xDEADBEEF;
    	msg.id 			= 2;
    	msg.len[0] 		= 0x00;
    	msg.len[1]		= 0x04;
    	msg.len[2]		= 0x18;
    	msg.timestamp	= 0;
        msg.frameNumber = 0;
        msg.micNumber   = 0;
        //        memcpy(msgXXX.payload, rb->data, sizeof(msg));
    	for (int i=0; i<BUFFER_SIZE; i++) { msg.payload[i] 	= (float)(i); }

        msg.crc32		= 0xabcdefff; //computeCRC32(const uint8_t* data, size_t len);
        uint8_t status =  CDC_Transmit_FS((uint8_t*)(&msg), sizeof(msg));

        return (status == USBD_OK);
    }



private:
//    SDS_MsgHeader1 msg;

    uint32_t frameCounter;

//    uint32_t computeCRC32(const uint8_t* data, size_t len)
//    {
//        uint32_t crc = 0xFFFFFFFF;
//        for (size_t i = 0; i < len; i++) {
//            crc ^= data[i];
//            for (int j = 0; j < 8; j++) {
//                uint32_t mask = -(crc & 1);
//                crc = (crc >> 1) ^ (0xEDB88320 & mask);
//            }
//        }
//        return ~crc;
//    }
};
