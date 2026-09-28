/*
 * GeoPosition.hpp  (Infrastructure/Utils)
 *
 * Standort der Sensoreinheit (WGS84): Breite, Länge, Höhe (doc/ICD_SDS_PC_Monitor.md 4.5, 5.5).
 * FSL9 §1 (A7): Positionen der Einheiten vermessen und gespeichert – Grundlage der Lokalisation
 * mit mehreren Einheiten und der Umrechnung der Spur in geografische Koordinaten.
 *
 * Quellen:
 *   PC   – USB-Kommando Id 10 vom PC-Monitor (Eingabe oder gespeicherter Wert), Hardware-Version 1
 *   GNSS – GPS-Modul, setzt die Position beim Start (Hardware-Version 2)
 * Vorrang: eine gültige GNSS-Position wird von Id 10 nicht überschrieben.
 *
 * Kommando Id 10 (28 Byte, big-endian):
 *   [0..7]   DE AD BE EF 0A 00 00 1C
 *   [8..11]  Breite  i32, 1e-7° (Nord positiv), |φ| ≤ 90°
 *   [12..15] Länge   i32, 1e-7° (Ost positiv),  |λ| ≤ 180°
 *   [16..19] Höhe    i32, mm über NN (MSL), −1000 … +10000 m
 *   [20]     Flags   Bit 0 = gültig (0 = Position löschen)
 *   [21..23] reserviert
 *   [24..27] CRC32 BE (noch nicht geprüft, Befund 12)
 * Nachricht Id 6 (SDS -> PC, 144 Byte, Nutzlast little-endian): siehe encodeReport().
 *
 * Ganzzahlen statt float: float hat bei 180° nur ~1,7 m Auflösung.
 * Reine Logik ohne Hardware (Host-Test t_geo_position).
 */
#pragma once
#include <cstdint>
#include <cstring>

namespace sds110 {

enum class PositionSource : uint8_t { None = 0, Pc = 1, Gnss = 2 };

struct GeoPosition {
    int32_t latE7 = 0;               // 1e-7°
    int32_t lonE7 = 0;               // 1e-7°
    int32_t altMm = 0;               // mm über NN
    PositionSource source = PositionSource::None;
    bool    valid = false;

    double latDeg() const { return latE7 * 1e-7; }
    double lonDeg() const { return lonE7 * 1e-7; }
    float  altM()   const { return static_cast<float>(altMm) * 0.001f; }
};

class GeoPositionCodec {
public:
    static constexpr uint32_t CMD_LENGTH = 28;
    static constexpr uint8_t  FLAG_VALID = 0x01;
    static constexpr int32_t  LAT_LIMIT_E7 = 900000000, LON_LIMIT_E7 = 1800000000;
    static constexpr int32_t  ALT_MIN_MM = -1000000, ALT_MAX_MM = 10000000;
    static constexpr uint32_t REPORT_PAYLOAD = 16;          // genutzte Bytes der 128-Byte-Nutzlast

    /// Kommando Id 10 (ab Magic) -> Position (Quelle PC); false bei falscher Länge oder Werten außerhalb
    static bool decode(const uint8_t* rx, uint32_t len, GeoPosition& out)
    {
        if (len != CMD_LENGTH) return false;
        GeoPosition p;
        p.latE7 = be32(rx + 8);
        p.lonE7 = be32(rx + 12);
        p.altMm = be32(rx + 16);
        p.valid = (rx[20] & FLAG_VALID) != 0;
        p.source = p.valid ? PositionSource::Pc : PositionSource::None;
        if (p.valid && (p.latE7 < -LAT_LIMIT_E7 || p.latE7 > LAT_LIMIT_E7 ||
                        p.lonE7 < -LON_LIMIT_E7 || p.lonE7 > LON_LIMIT_E7 ||
                        p.altMm < ALT_MIN_MM || p.altMm > ALT_MAX_MM)) return false;
        if (!p.valid) p = GeoPosition{};
        out = p;
        return true;
    }

    /// Vorrang: eine gültige GNSS-Position bleibt, bis GNSS selbst sie ändert
    static bool accept(const GeoPosition& current, const GeoPosition& incoming)
    {
        return !(current.valid && current.source == PositionSource::Gnss &&
                 incoming.source != PositionSource::Gnss);
    }

    /// Nutzlast der Nachricht Id 6 (little-endian):
    ///   [0..1] Unit-ID u16  [2] Quelle (0 keine, 1 PC, 2 GNSS)  [3] Flags Bit 0 = gültig
    ///   [4..7] Breite i32 1e-7°  [8..11] Länge i32 1e-7°  [12..15] Höhe i32 mm
    static void encodeReport(const GeoPosition& p, uint16_t unit, uint8_t* out)
    {
        std::memset(out, 0, REPORT_PAYLOAD);
        std::memcpy(out, &unit, 2);
        out[2] = static_cast<uint8_t>(p.valid ? p.source : PositionSource::None);
        out[3] = p.valid ? FLAG_VALID : 0;
        const int32_t v[3] = { p.valid ? p.latE7 : 0, p.valid ? p.lonE7 : 0, p.valid ? p.altMm : 0 };
        std::memcpy(out + 4, v, sizeof(v));
    }

private:
    static int32_t be32(const uint8_t* b)
    {
        return static_cast<int32_t>((static_cast<uint32_t>(b[0]) << 24) | (static_cast<uint32_t>(b[1]) << 16) |
                                    (static_cast<uint32_t>(b[2]) << 8) | b[3]);
    }
};

} // namespace sds110
