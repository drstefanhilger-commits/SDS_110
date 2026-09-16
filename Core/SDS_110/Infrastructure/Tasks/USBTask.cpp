/*
 * USBTask.cpp  (Infrastructure/Tasks)
 */
#include "USBTask.hpp"
#include <cstring>
#include <initializer_list>

// von usbd_cdc_if.c referenziert
volatile uint32_t usb_debug_counter = 0;

extern "C" void USBTask_OnReceive(uint8_t* buf, uint32_t len)
{
    sds110::USBTask::instance().onUsbReceive(buf, len);
}

namespace sds110 {

USBTask::USBTask() : TaskBase(4096, 10, osPriorityNormal) {}

void USBTask::onStart()
{
    rxQueue_ = xQueueCreate(8, MAX_LENGTH);
}

void USBTask::onUsbReceive(const uint8_t* buf, uint32_t len)
{
    if (!rxQueue_) return;
    uint8_t local[MAX_LENGTH] = {};
    const size_t n = (len > MAX_LENGTH) ? MAX_LENGTH : len;
    memcpy(local, buf, n);
    BaseType_t hpw = pdFALSE;
    xQueueSendFromISR(rxQueue_, local, &hpw);
    portYIELD_FROM_ISR(hpw);
}

void USBTask::runOnce()
{
    uint8_t rx[MAX_LENGTH];
    while (xQueueReceive(rxQueue_, rx, 0) == pdTRUE) {
        if (!hasMagic(rx)) { handleError(rx); continue; }
        switch (rx[4]) {
            case 1:  handleTimeSync(rx);   break;
            case 2:  handleModeChange(rx); break;
            case 3:  handleSimulation(rx); break;
            default: handleError(rx);      break;
        }
    }
    reportStats(TaskId::Usb);
}

void USBTask::resetCounters()
{
    for (TaskId t : { TaskId::Proc120, TaskId::Lcd, TaskId::Mic })
        dm_.setTaskStats(t, 0, 0.0f, 0);
}

void USBTask::handleTimeSync(const uint8_t* rx)
{
    if (payloadLen(rx) != 16) { handleError(rx); return; }
    dm_.setSyncTimeDifference(payloadU32(rx));
    dm_.setId(rx[4]);
}

void USBTask::handleModeChange(const uint8_t* rx)
{
    if (payloadLen(rx) != 16) { handleError(rx); return; }
    const SDS_Mode mode = static_cast<SDS_Mode>(payloadU32(rx));
    if (dm_.getMode() != mode) {
        dm_.setMode(mode);
        resetCounters();
        dm_.setId(rx[0]);
    }
}

void USBTask::handleSimulation(const uint8_t* rx)
{
    if (payloadLen(rx) != 16) { handleError(rx); return; }
    dm_.setSimulation(payloadU32(rx));
    resetCounters();
    dm_.setId(rx[0]);
}

void USBTask::handleError(const uint8_t* rx)
{
    dm_.setErrorFlag(1);
    dm_.setErrorBuffer(rx, 16);
    resetCounters();
    dm_.setErrorCount(30);   // ~10 s Anzeige
}

bool USBTask::hasMagic(const uint8_t* rx)
{
    return rx[0] == 0xDE && rx[1] == 0xAD && rx[2] == 0xBE && rx[3] == 0xEF;
}

} // namespace sds110
