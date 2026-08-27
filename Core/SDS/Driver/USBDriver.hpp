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
#include "Model.hpp"

bool USB_SendDetection(uint32_t timestamp, uint32_t micId, float azimuth, float distance, float confidence);

//void onUsbReceive2(uint8_t* buf, uint32_t len);
