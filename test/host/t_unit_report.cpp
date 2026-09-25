/*
 * t_unit_report – Serialisierung des UnitReport (Nachricht id 4)
 *
 * Bezug: Befund 10
 * Prüft/misst: Feld nsel = tatsächlich gesendete Bänder (max. 56), Bandliste innerhalb der 128-Byte-Nutzlast
 * Aufruf: make check  bzw.  build/test_host/t_unit_report
 * Beschreibung und Referenzergebnisse: doc/Host_Tests.md
 */
#include <cstdio>
#include <cstring>
#include "Processing_Module_120/Output_Interface_130/Output_Interface_130.hpp"
#include "Infrastructure/Model/SDS_Structs.hpp"
using namespace sds110;
namespace sds110 { extern ::MessageData g_lastMsg; extern uint32_t g_lastId; }
int main()
{
    Output_Interface_130 out; out.init(); int fail = 0;
    for (uint32_t nsel : { 0u, 3u, 8u, 56u, 57u, 64u }) {
        UnitReport r{}; r.unit_id = 0x1234; r.bearing_deg = 42.5f; r.valid_pairs = 27; r.level = 0.1f; r.num_selected = nsel;
        for (uint32_t i = 0; i < nsel; ++i) { r.band_index[i] = i; r.band_prob[i] = (i + 1) / 64.0f; }
        std::memset(&g_lastMsg, 0xEE, sizeof(g_lastMsg));
        out.send(r);
        const uint8_t* b = g_lastMsg.b; const uint32_t n = b[11];          // Kopf: 2+4+4+1 -> nsel an Offset 11
        const uint32_t expect = nsel > 56 ? 56 : nsel;
        bool ok = g_lastId == 4 && n == expect && 16 + 2 * n <= sizeof(::MessageData);
        for (uint32_t i = 0; i < n && ok; ++i) ok = b[16 + i] == i && b[16 + n + i] == (uint8_t)(((i + 1) / 64.0f) * 255.0f);
        std::printf("num_selected %2u -> nsel %2u, Bytes %3u/128 %s\n", nsel, n, 16 + 2 * n, ok ? "OK" : "FEHLER");
        fail += !ok;
    }
    return fail;
}
