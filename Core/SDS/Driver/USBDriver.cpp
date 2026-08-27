/*
 * USBDriver.c
 *
 *  Created on: Aug 18, 2026
 *      Author: 310004
 */

#include <USBDriver.hpp>

bool USB_SendDetection(uint32_t timestamp, uint32_t micId, float azimuth, float distance, float confidence)
{
//    struct DetectionMsg {
//        uint32_t magic;
//        uint32_t ts;
//        uint32_t mic;
//        float    azi;
//        float    distance;
//        float    conf;
//    } msg;
//
//    msg.magic 	 = 0xDEADBEEF;
	SDS_MsgDetect msg;
    msg.timestamp   = timestamp;
    msg.mic   	 	= micId;
    msg.azi   	 	= azimuth;
    msg.distance 	= distance;
    msg.conf  	 	= confidence;

    msg.crc32		= 0;

    uint8_t status =  CDC_Transmit_FS((uint8_t*)(&msg), sizeof(msg));
    return (status == USBD_OK);
}
