/*
 * t_sdram_selftest – Logik des SDRAM-Selbsttests
 *
 * Bezug: Befund 14
 * Prüft/misst: Handle nicht READY -> kein Speicherzugriff; intakter Bereich -> bestanden, Prüfadressen, Cache-Flushes, kein Schreiben außerhalb
 * Aufruf: make check  bzw.  build/test_host/t_sdram_selftest
 * Beschreibung und Referenzergebnisse: doc/Host_Tests.md
 */
#include <cstdio>
#include <vector>
#include "Infrastructure/Driver/SDRAMSelfTest.hpp"
int g_flushes = 0;
using namespace sds110;
int main()
{
    int fail = 0;
    const uint32_t n = 599520 / 4;                        // Größe .sdram_data aus dem Build
    std::vector<uint32_t> mem(n + 4, 0x11111111u);         // +4 Wächterwörter hinter dem Bereich
    SDRAM_HandleTypeDef h{ HAL_SDRAM_STATE_RESET };
    bool r = sdramSelfTest(h, mem.data(), mem.data() + n);
    bool untouched = true; for (auto v : mem) if (v != 0x11111111u) untouched = false;
    std::printf("Handle nicht READY: Ergebnis %d, Speicher unberührt %d  %s\n", r, untouched, (!r && untouched) ? "OK" : "FEHLER"); fail += (r || !untouched);
    h.State = HAL_SDRAM_STATE_READY; g_flushes = 0;
    r = sdramSelfTest(h, mem.data(), mem.data() + n);
    uint32_t changed = 0; for (uint32_t i = 0; i < n; ++i) if (mem[i] != 0x11111111u) ++changed;
    bool guard = true; for (uint32_t i = n; i < n + 4; ++i) if (mem[i] != 0x11111111u) guard = false;
    std::printf("intakter Speicher: Ergebnis %d, geprüfte Adressen %u, Cache-Flushes %d, Wächter unberührt %d  %s\n",
                r, changed, g_flushes, guard, (r && guard && g_flushes == 2) ? "OK" : "FEHLER"); fail += !(r && guard && g_flushes == 2);
    r = sdramSelfTest(h, mem.data(), mem.data());
    std::printf("leerer Bereich: Ergebnis %d  %s\n", r, r ? "OK" : "FEHLER"); fail += !r;
    return fail;
}
