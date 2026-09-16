/*
 * USBDriver.hpp  (Infrastructure/Driver)
 *
 * Sendeseite CDC: Wire-Format aus SDS_Structs.hpp + CRC32 -> CDC_Transmit_FS.
 * Wird von USBTask (Logging, READ-Streaming) und von Output_Interface_130
 * (Candidate Report) genutzt.
 *
 * Migration aus SDS/Driver/USBDriver.{hpp,cpp}:
 *  - USB_SendRead: liest jetzt float-Frames aus Microphone_Array_114 (kein raw[] mehr);
 *    Werte werden auf int32 (24-bit) zurückskaliert, Wire-Format bleibt 532 Byte.
 *  - USB_SendRead_Test entfällt; SDSUSBMicSender (Prototyp) entfällt.
 */
#pragma once
#include <cstdint>
#include "Infrastructure/Model/SDS_Structs.hpp"
#include "Sensor_Unit_112/Microphone_Array_114.hpp"

namespace sds110 {

class USBDriver {
public:
    /// 32 Byte, id 1 – Azimut/Distanz/Konfidenz (Legacy-Format des PC-Monitors)
    static bool sendDetection(uint32_t timestamp, uint32_t micId, float azimuth, float distance, float confidence);
    /// 532 Byte, id 2 – 128 Rohsamples eines Mikrofons ab frameNr*128
    static bool sendRead(uint32_t timestamp, uint32_t micNr, uint32_t frameNr, const MicFrame* frame);
    /// 48 Byte, id 3 – 32 Byte Logtext
    static bool sendLogging(uint32_t timestamp, const uint8_t* src, int len);
    /// generische Nachricht (128 Byte Payload)
    static bool sendMessage(uint32_t id, uint32_t timestamp, const MessageData& data);

private:
    static bool transmit(const void* buf, uint16_t len);
};

} // namespace sds110
