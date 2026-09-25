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
 * Ereignisgetrieben: waitForWork() blockiert auf rxQueue_, bis ein Kommando
 * kommt (kein Polling, keine Latenz). Stats "USB": Bearbeitungszeit je
 * Aufwachen, Zähler = Anzahl Aufwachvorgänge.
 *
 * Migration aus SDS/Tasks/USBTask: SDS_Data-API angepasst
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

    /// aus CDC_Receive_FS (ISR-Kontext). Konstruiert NICHT die Instanz:
    /// solange der Task nicht angelegt ist, wird verworfen.
    static void onUsbReceiveISR(const uint8_t* buf, uint32_t len);

    uint32_t rxDropped() const { return rxDropped_; }

protected:
    void waitForWork() override;
    void runOnce() override;

private:
    USBTask();
    void handle(const uint8_t* rx);
    void handleTimeSync(const uint8_t* rx);
    void handleModeChange(const uint8_t* rx);
    void handleSimulation(const uint8_t* rx);
    void handleSetUnitId(const uint8_t* rx);     // Typ 5: Payload u32 = neue Unit-ID
    void handleError(const uint8_t* rx);
    static bool hasMagic(const uint8_t* rx);
    /// Längenfeld: Gesamtlänge der Nachricht (SDS_CMD_LENGTH), nicht nur der Nutzdaten
    static uint32_t msgLen(const uint8_t* rx)     { return (rx[5] << 16) | (rx[6] << 8) | rx[7]; }
    static uint32_t payloadU32(const uint8_t* rx)  { return (rx[8] << 24) | (rx[9] << 16) | (rx[10] << 8) | rx[11]; }
    void resetCounters();

    static constexpr size_t MAX_LENGTH = 64;
    SDS_Data&     dm_ = SDS_Data::instance();
    QueueHandle_t rxQueue_ = nullptr;
    uint8_t       rx_[MAX_LENGTH] = {};          // von waitForWork() empfangen
    bool          rxValid_ = false;
    volatile uint32_t rxDropped_ = 0;            // Queue voll / Task nicht bereit

    static USBTask* volatile active_;            // gesetzt, sobald Queue existiert
};

} // namespace sds110

extern "C" {
void USBTask_OnReceive(uint8_t* buf, uint32_t len);
extern volatile uint32_t usb_debug_counter;
}
