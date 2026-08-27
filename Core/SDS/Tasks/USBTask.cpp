/*
 * USBTask.cpp
 *
 *  Created on: Aug 17, 2026
 *      Author: 310004
 */

#include "USBTask.hpp"
#include "Model.hpp"
#include <cstring>

extern "C" void USBTask_OnReceive(uint8_t* buf, uint32_t len)
{
    USBTask::instance().onUsbReceive(buf, len);
}

USBTask::USBTask()
    : TaskBase(4096, 10, osPriorityNormal)   // 10 ms periodisch
{

}

void USBTask::onStart()
{
    usbRxQueue = xQueueCreate(8, MAX_LENGTH);
}

void USBTask::onUsbReceive(uint8_t* buf, uint32_t len)
{
    uint8_t localBuf[MAX_LENGTH];
    size_t copyLen = (len > MAX_LENGTH) ? MAX_LENGTH : len;
    memcpy(localBuf, buf, copyLen);

    BaseType_t hpw = pdFALSE;
    xQueueSendFromISR(usbRxQueue, localBuf, &hpw);
    portYIELD_FROM_ISR(hpw);
}

void USBTask::runOnce()
{
    uint8_t rxBuffer[MAX_LENGTH];

    if (xQueueReceive(usbRxQueue, rxBuffer, 0) == pdTRUE)
    {
    	uint8_t MsgType = rxBuffer[0];
		switch(MsgType)
		{
			case 1:  HandleUnixTimeSync(rxBuffer); break;
			case 2:  HandleStateChange(rxBuffer); break;
			case 3:  HandleSetSimuation(rxBuffer); break;
			default: HandleError(rxBuffer); break;
		}
    }
}

void USBTask::HandleStateChange(uint8_t* rxBuffer) {
	uint32_t len = (rxBuffer[1] << 16) | (rxBuffer[2] << 8) | rxBuffer[3];
	if (len == 12) {
		uint32_t mode = (rxBuffer[4] << 24) |(rxBuffer[5] << 16) | (rxBuffer[6] << 8) | rxBuffer[7];
		if (dm.getMode() != mode) {
			dm.setMode(mode);
			dm.setMicLoopCounter(0);
			dm.setLcdLoopCounter(0);
			dm.setSrpLoopCounter(0);
			dm.setId(rxBuffer[0]);	//Debug
		}
	} else {
		HandleError(rxBuffer);
	}
}

void USBTask::HandleUnixTimeSync(uint8_t* rxBuffer) {
	uint32_t len = (rxBuffer[1] << 16) | (rxBuffer[2] << 8) | rxBuffer[3];
	if (len == 12) {
		uint32_t time = (rxBuffer[4] << 24) |(rxBuffer[5] << 16) | (rxBuffer[6] << 8) | rxBuffer[7];
		dm.setSyncTimeDifference(time);
	    dm.setId(rxBuffer[0]);	//Debug
	} else {
		HandleError(rxBuffer);
	}
}

void USBTask::HandleSetSimuation(uint8_t* rxBuffer) {
	uint32_t len = (rxBuffer[1] << 16) | (rxBuffer[2] << 8) | rxBuffer[3];
	if (len == 12) {
		uint32_t doSimulation = (rxBuffer[4] << 24) |(rxBuffer[5] << 16) | (rxBuffer[6] << 8) | rxBuffer[7];
		dm.setSimulation(doSimulation);
		dm.setMicLoopCounter(0);
		dm.setLcdLoopCounter(0);
		dm.setSrpLoopCounter(0);
	    dm.setId(rxBuffer[0]);	//Debug
	} else {
		HandleError(rxBuffer);
	}
}

void USBTask::HandleError(uint8_t* rxBuffer) {
	dm.setErrorFlag(1);
	dm.setErrorLen(12);
	memcpy(dm.getErrorBuffer(), rxBuffer, 12);
	dm.setLcdLoopCounter(0);
	dm.setSrpLoopCounter(0);
	dm.setErrorCount(30);	// ~10 sec
}

