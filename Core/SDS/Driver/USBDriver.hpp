/*
 * USBDriver.h
 *
 *  Created on: Aug 18, 2026
 *      Author: 310004
 */
#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include <stdbool.h>
#include "usbd_cdc_if.h"
#include "crc32.hpp"
#include "Model.hpp"

bool USB_SendDetection(uint32_t timestamp, uint32_t micId, float azimuth, float distance, float confidence);
bool USB_SendRead(uint32_t timestamp, uint32_t micNr, uint32_t frameNr, UnifiedMicBuffer* rb);
bool USB_SendLogging(uint32_t timestamp, uint8_t* dst, int maxLen);
bool SDS_SendMessage(uint32_t id, uint32_t timestamp, MessageData data, uint32_t len);

bool USB_SendRead_Test();

