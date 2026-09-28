/*
 * t_frame_assembler – Analysefenster mit 50 % Überlappung
 *
 * Bezug: Befund 17
 * Prüft/misst: Füllen, Schieben um einen Hop, Zeitstempel = ältester Hop, Neubeginn bei Lücke in der Hop-Folge, reset()
 * Aufruf: make check  bzw.  build/test_host/t_frame_assembler
 * Beschreibung und Referenzergebnisse: doc/Host_Tests.md
 */
#include <cstdio>
#include "Sensor_Unit_112/Frame_Assembler.hpp"
#include "Sensor_Unit_112/Pre_Processor_118.hpp"
#include <cstring>
#include <cmath>
using namespace sds110;
static MicFrame hop;
static void make(uint32_t id) { hop.frame_id = id; hop.time_utc_us = 1000000ull + id * 32000ull;
    for (uint32_t ch = 0; ch < NUM_MICS; ++ch) for (uint32_t i = 0; i < HOP_SAMPLES; ++i) hop.data[ch][i] = id * 10000.0f + ch * 1000.0f + (i % 1000); }
static bool frameIs(const AnalysisFrame& f, uint32_t idA, uint32_t idB)   // erwartet [Hop idA | Hop idB]
{
    for (uint32_t ch = 0; ch < NUM_MICS; ++ch)
        for (uint32_t i = 0; i < FRAME_SAMPLES; ++i) {
            const uint32_t id = i < HOP_SAMPLES ? idA : idB, k = i % HOP_SAMPLES;
            if (f.sample(ch, i) != id * 10000.0f + ch * 1000.0f + (k % 1000)) return false;
        }
    return f.time_utc_us == 1000000ull + idA * 32000ull;
}
int main()
{
    static Frame_Assembler fa; int fail = 0;
    auto check = [&](const char* what, bool ok) { std::printf("%-48s %s\n", what, ok ? "OK" : "FEHLER"); fail += !ok; };
    make(5); check("Hop 5: Fenster noch nicht voll", !fa.push(hop));
    make(6); bool full = fa.push(hop); check("Hop 6: Frame [5|6], Zeit von Hop 5", full && frameIs(fa.frame(), 5, 6));
    const uint32_t id0 = fa.frame().frame_id;
    make(7); full = fa.push(hop); check("Hop 7: Frame [6|7] (um einen Hop geschoben)", full && frameIs(fa.frame(), 6, 7));
    check("frame_id fortlaufend", fa.frame().frame_id == id0 + 1);
    make(9); check("Hop 9 nach Lücke (8 fehlt): Neubeginn", !fa.push(hop));
    make(10); full = fa.push(hop); check("Hop 10: Frame [9|10], nichts aus Hop 7", full && frameIs(fa.frame(), 9, 10));
    fa.reset(); make(11); check("reset(): Hop 11 füllt erneut", !fa.push(hop));

    // Board-Pfad (118 schreibt direkt in den Slot, beginHop/commitHop) = Werkzeug-Pfad
    // (118 in-place, push kopiert): Frames bitgleich über 6 Hops, 118 mit Zustand
    static Pre_Processor_118 preA, preB; static Frame_Assembler faA, faB; static MicFrame h, hA;
    preA.init(); preB.init();
    bool same = true; int framesA = 0, framesB = 0;
    for (uint32_t k = 0; k < 6; ++k) {
        h.frame_id = 100 + k; h.time_utc_us = 5000000ull + k * 32000ull;
        for (uint32_t ch = 0; ch < NUM_MICS; ++ch)
            for (uint32_t i = 0; i < HOP_SAMPLES; ++i)
                h.data[ch][i] = 0.2f * std::sin(0.05f * (k * HOP_SAMPLES + i) + ch) + 0.01f * ((i * 7 + ch * 13 + k) % 11);
        std::memcpy(&hA, &h, sizeof(h));
        preA.process(hA); const bool fullA = faA.push(hA);
        float* dst[NUM_MICS]; faB.beginHop(h, dst); preB.process(h, dst); const bool fullB = faB.commitHop(h);
        framesA += fullA; framesB += fullB;
        if (fullA != fullB) same = false;
        if (fullA && fullB)
            for (uint32_t ch = 0; ch < NUM_MICS; ++ch)
                for (uint32_t i = 0; i < FRAME_SAMPLES; ++i)
                    if (faA.frame().sample(ch, i) != faB.frame().sample(ch, i)) same = false;
        same = same && preA.appliedGain(0) == preB.appliedGain(0);
    }
    check("Board-Pfad = Werkzeug-Pfad (bitgleich, 5 Frames)", same && framesA == 5 && framesB == 5);
    return fail;
}
