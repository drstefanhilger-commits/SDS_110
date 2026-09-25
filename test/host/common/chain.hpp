// Host-Tests: Simulator -> 114 (Hop) -> 118 -> Frame_Assembler, wie Sensor_Unit_112::nextFrame()
#pragma once
#include "Harness/Signal_Simulator.hpp"
#include "Sensor_Unit_112/Pre_Processor_118.hpp"
#include "Sensor_Unit_112/Frame_Assembler.hpp"
namespace sds110 {
static Frame_Assembler g_fa;
inline const AnalysisFrame& nextAnalysisFrame(Signal_Simulator& sim, Microphone_Array_114& arr, Pre_Processor_118& pre)
{
    for (;;) {
        sim.generateHop(0);
        MicFrame* h = arr.acquireReadable();
        pre.process(*h);
        const bool full = g_fa.push(*h);
        arr.release(h);
        if (full) return g_fa.frame();
    }
}
}
