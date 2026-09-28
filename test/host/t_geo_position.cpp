/*
 * t_geo_position – Standort der Einheit (FSL9 §1, A7), USB Id 10 / Nachricht Id 6
 *
 * Prüft (Infrastructure/Utils/GeoPosition.hpp):
 *   1. Id 10: Breite/Länge in 1e-7°, Höhe in mm, Vorzeichen (Süd/West, unter NN); Bytes des
 *      PC-Monitors (app/model/SDSUSBModel.py build_position_message) gleich
 *   2. Grenzen: |φ| > 90°, |λ| > 180°, Höhe außerhalb −1000 … +10000 m, falsche Länge -> verworfen
 *   3. Flags = 0 -> Position gelöscht (ungültig, Quelle keine)
 *   4. Vorrang: gültige GNSS-Position wird von Id 10 nicht überschrieben; PC über PC ja
 *   5. Nachricht Id 6: Nutzlast little-endian (Unit, Quelle, Flags, φ, λ, h)
 * Aufruf: build/test_host/t_geo_position
 */
#include <cstdio>
#include <cstring>
#include <cmath>
#include "Infrastructure/Utils/GeoPosition.hpp"
using namespace sds110;

static int g_fail = 0;
static void check(bool ok, const char* what) { std::printf("%s %s\n", ok ? "ok  " : "FAIL", what); if (!ok) g_fail = 1; }

static void put32(uint8_t* b, int32_t v) { const uint32_t u = static_cast<uint32_t>(v); b[0] = u >> 24; b[1] = u >> 16; b[2] = u >> 8; b[3] = u; }
static void cmd(uint8_t* m, int32_t lat, int32_t lon, int32_t alt, uint8_t flags)
{
    const uint8_t h[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0x0A, 0x00, 0x00, 0x1C};
    std::memcpy(m, h, 8); put32(m + 8, lat); put32(m + 12, lon); put32(m + 16, alt);
    m[20] = flags; m[21] = m[22] = m[23] = 0; put32(m + 24, 0);
}

int main()
{
    uint8_t m[28];
    GeoPosition p;
    // 1) München, 519,5 m
    cmd(m, 481371540, 115754900, 519500, 1);
    check(GeoPositionCodec::decode(m, 28, p) && p.valid && p.source == PositionSource::Pc &&
          p.latE7 == 481371540 && p.lonE7 == 115754900 && p.altMm == 519500 &&
          std::fabs(p.latDeg() - 48.137154) < 1e-9 && std::fabs(p.altM() - 519.5f) < 1e-3f, "Id 10 dekodiert");
    cmd(m, -338688000, -1512093000, -12000, 1);
    check(GeoPositionCodec::decode(m, 28, p) && p.latE7 == -338688000 && p.lonE7 == -1512093000 && p.altMm == -12000,
          "Süd, West, unter NN");
    // Bytes vom PC-Monitor: build_position_message(48.137154, 11.57549, 519.5)
    const char* pcHex = "deadbeef0a00001c1cb1259406e647940007ed4c01000000";
    uint8_t pc[24];
    for (int i = 0; i < 24; ++i) { unsigned v; std::sscanf(pcHex + 2 * i, "%2x", &v); pc[i] = static_cast<uint8_t>(v); }
    cmd(m, 481371540, 115754900, 519500, 1);
    check(std::memcmp(pc, m, 24) == 0, "Bytes des PC-Monitors gleich");
    // 2) Grenzen
    GeoPosition q; q.latE7 = 7;
    cmd(m, 900000001, 0, 0, 1);  bool r1 = GeoPositionCodec::decode(m, 28, q);
    cmd(m, 0, -1800000001, 0, 1); bool r2 = GeoPositionCodec::decode(m, 28, q);
    cmd(m, 0, 0, 10000001, 1);   bool r3 = GeoPositionCodec::decode(m, 28, q);
    cmd(m, 0, 0, 0, 1);          bool r4 = GeoPositionCodec::decode(m, 24, q);
    check(!r1 && !r2 && !r3 && !r4 && q.latE7 == 7, "außerhalb der Grenzen / falsche Länge verworfen");
    cmd(m, 900000000, 1800000000, -1000000, 1);
    check(GeoPositionCodec::decode(m, 28, q) && q.valid, "Grenzwerte ±90°, 180°, −1000 m gültig");
    // 3) Löschen
    cmd(m, 123, 456, 789, 0);
    check(GeoPositionCodec::decode(m, 28, q) && !q.valid && q.source == PositionSource::None && q.latE7 == 0,
          "Flags 0 -> Position gelöscht");
    // 4) Vorrang
    GeoPosition gnss; gnss.valid = true; gnss.source = PositionSource::Gnss; gnss.latE7 = 1;
    GeoPosition pcPos; pcPos.valid = true; pcPos.source = PositionSource::Pc;
    GeoPosition none;
    check(!GeoPositionCodec::accept(gnss, pcPos) && !GeoPositionCodec::accept(gnss, none) &&
          GeoPositionCodec::accept(pcPos, pcPos) && GeoPositionCodec::accept(none, pcPos) &&
          GeoPositionCodec::accept(pcPos, gnss), "GNSS hat Vorrang vor Id 10");
    // 5) Nachricht Id 6
    uint8_t out[16];
    cmd(m, 481371540, 115754900, 519500, 1); GeoPositionCodec::decode(m, 28, p);
    GeoPositionCodec::encodeReport(p, 4660, out);
    int32_t v[3]; std::memcpy(v, out + 4, 12);
    const uint16_t unit = static_cast<uint16_t>(out[0] | (out[1] << 8));
    check(unit == 4660 && out[2] == 1 && out[3] == 1 && v[0] == 481371540 && v[1] == 115754900 && v[2] == 519500,
          "Id 6: Unit, Quelle PC, gültig, φ, λ, h (little-endian)");
    GeoPositionCodec::encodeReport(GeoPosition{}, 1, out);
    check(out[2] == 0 && out[3] == 0 && out[4] == 0, "Id 6 ohne Standort: Quelle keine, ungültig");
    return g_fail;
}
