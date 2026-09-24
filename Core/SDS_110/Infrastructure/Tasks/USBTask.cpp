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
    sds110::USBTask::onUsbReceiveISR(buf, len);
}

namespace sds110 {

USBTask* volatile USBTask::active_ = nullptr;

// delayMs wird nicht benutzt (waitForWork blockiert auf der Queue)
USBTask::USBTask() : TaskBase(4096, 0, osPriorityNormal)
{
    // Queue im Konstruktor (main, nach osKernelInitialize), nicht erst im Task:
    // so geht kein Kommando verloren, das vor dem ersten Task-Lauf eintrifft.
    rxQueue_ = xQueueCreate(8, MAX_LENGTH);
    configASSERT(rxQueue_ != nullptr);
    active_ = this;
}

void USBTask::onUsbReceiveISR(const uint8_t* buf, uint32_t len)
{
    USBTask* self = active_;
    if (self == nullptr) return;                     // USBTask nicht gestartet

    uint8_t local[MAX_LENGTH] = {};
    const size_t n = (len > MAX_LENGTH) ? MAX_LENGTH : len;
    memcpy(local, buf, n);
    BaseType_t hpw = pdFALSE;
    if (xQueueSendFromISR(self->rxQueue_, local, &hpw) != pdPASS)
        ++self->rxDropped_;
    portYIELD_FROM_ISR(hpw);
}

void USBTask::waitForWork()
{
    // schläft, bis ein Kommando kommt – keine CPU-Last im Leerlauf
    rxValid_ = (xQueueReceive(rxQueue_, rx_, portMAX_DELAY) == pdTRUE);
}

void USBTask::runOnce()
{
    if (rxValid_) handle(rx_);
    // weitere, inzwischen eingetroffene Kommandos gleich mit abarbeiten
    uint8_t rx[MAX_LENGTH];
    while (xQueueReceive(rxQueue_, rx, 0) == pdTRUE)
        handle(rx);
    reportStats(TaskId::Usb);
}

void USBTask::handle(const uint8_t* rx)
{
    if (!hasMagic(rx)) { handleError(rx); return; }
    switch (rx[4]) {
        case 1:  handleTimeSync(rx);   break;
        case 2:  handleModeChange(rx); break;
        case 3:  handleSimulation(rx); break;
        case 5:  handleSetUnitId(rx);  break;
        default: handleError(rx);      break;
    }
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
    // entfernt: dm_.setId(rx[4]) – überschrieb die Unit-ID mit dem Kommandotyp (1)
}

void USBTask::handleModeChange(const uint8_t* rx)
{
    if (payloadLen(rx) != 16) { handleError(rx); return; }
    const SDS_Mode mode = static_cast<SDS_Mode>(payloadU32(rx));
    if (dm_.getMode() != mode) {
        dm_.setMode(mode);
        resetCounters();
        // entfernt: dm_.setId(rx[0]) – überschrieb die Unit-ID mit 0xDE (Magic)
    }
}

void USBTask::handleSimulation(const uint8_t* rx)
{
    if (payloadLen(rx) != 16) { handleError(rx); return; }
    dm_.setSimulation(payloadU32(rx));
    resetCounters();
    // entfernt: dm_.setId(rx[0]) – überschrieb die Unit-ID mit 0xDE (Magic)
}

void USBTask::handleSetUnitId(const uint8_t* rx)
{
    if (payloadLen(rx) != 16) { handleError(rx); return; }
    dm_.setId(static_cast<uint16_t>(payloadU32(rx) & 0xFFFF));
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
