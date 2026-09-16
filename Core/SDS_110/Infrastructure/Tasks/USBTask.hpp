/*
 * USBTask.hpp  (Infrastructure/Tasks)
 *
 * Empfangsseite CDC (Kommandos vom PC-Monitor) und nicht-patentrelevante
 * Sendepfade (Logging, Rohdaten im READ-Modus).
 * Der Candidate Report (Patent) wird NICHT hier, sondern in
 * Output_Interface_130 gesendet.
 *
 * Anbindung in usbd_cdc_if.c, CDC_Receive_FS(), USER CODE 6:
 *     extern void USBTask_OnReceive(uint8_t* buf, uint32_t len);
 *     USBTask_OnReceive(Buf, *Len);
 * Nachrichten max. 63 Byte. Das Symbol usb_debug_counter (bisher in LCDTask)
 * wird hier definiert.
 *
 * Migration aus SDS/Tasks/USBTask: Ablauf unverändert; SDS_Data-API angepasst
 * (setMode(SDS_Mode), setTaskStats, setErrorBuffer statt getErrorBuffer()+memcpy).
 */
#pragma once
#include "FreeRTOS.h"
#include "queue.h"
#include "TaskBase.hpp"
#include "Infrastructure/Driver/USBDriver.hpp"

namespace sds110 {

class USBTask : public TaskBase {
public:
    static USBTask& instance() { static USBTask inst; return inst; }
    /// aus CDC_Receive_FS (ISR-Kontext)
    void onUsbReceive(const uint8_t* buf, uint32_t len);

protected:
    void onStart() override;
    void runOnce() override;

private:
    USBTask();
    void handleTimeSync(const uint8_t* rx);
    void handleModeChange(const uint8_t* rx);
    void handleSimulation(const uint8_t* rx);
    void handleError(const uint8_t* rx);
    static bool hasMagic(const uint8_t* rx);
    static uint32_t payloadLen(const uint8_t* rx) { return (rx[5] << 16) | (rx[6] << 8) | rx[7]; }
    static uint32_t payloadU32(const uint8_t* rx)  { return (rx[8] << 24) | (rx[9] << 16) | (rx[10] << 8) | rx[11]; }
    void resetCounters();

    static constexpr size_t MAX_LENGTH = 64;
    SDS_Data&     dm_ = SDS_Data::instance();
    QueueHandle_t rxQueue_ = nullptr;
};

} // namespace sds110

extern "C" {
void USBTask_OnReceive(uint8_t* buf, uint32_t len);
extern volatile uint32_t usb_debug_counter;
}
