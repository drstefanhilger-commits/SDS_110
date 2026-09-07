/*
 * USBTask.hpp
 *
 *  Created on: Aug 17, 2026
 *      Author: 310004
 */

/*
 * Maximal messages length is 63 Byte
 *
 * In usbd_cdc_if.c in add the specific handler
 *   static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len)
 *   USER CODE BEGIN 6
 *     extern void USBTask_OnReceive(uint8_t* buf, uint32_t len);
 *     USBTask_OnReceive(Buf, *Len);
 * 	 USER CODE END 6
 */

#pragma once
#include <USBDriver.hpp>
#include "TaskBase.hpp"
#include "FreeRTOS.h"
#include "queue.h"

#include "Model.hpp"

class USBTask : public TaskBase
{
public:
    static USBTask& instance()
    {
        static USBTask inst;
        return inst;
    }

    void onUsbReceive(uint8_t* buf, uint32_t len);

protected:
    void onStart() override;
    void runOnce() override;

private:
    USBTask();   // Konstruktor privat
    void HandleStateChange(uint8_t* rxBuffer);
	void HandleUnixTimeSync(uint8_t* rxBuffer);
	void HandleError(uint8_t* rxBuffer);
	void HandleSetSimuation(uint8_t* rxBuffer);

	bool hasMagic(uint8_t* rxBuffer);

private:
    SDS_Data& dm = SDS_Data::instance();
    QueueHandle_t usbRxQueue = nullptr;
    size_t MAX_LENGTH = 64;


};
