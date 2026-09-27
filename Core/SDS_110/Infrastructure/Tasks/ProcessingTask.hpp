/*
 * ProcessingTask.hpp  (Infrastructure/Tasks)
 * RTOS-Task um Processing_Module_120: DETECT -> processFrame(), READ -> streamFrame().
 * Ersetzt SDS/Tasks/SRPTask.
 *
 * Takt (Befund 28), waitForWork():
 *  - Simulation: fester Hop-Takt über HopClock (32 ms, osDelayUntil auf absolute Ticks);
 *    je Durchlauf alle fälligen Hops erzeugen und verarbeiten (höchstens SIM_MAX_CATCH_UP,
 *    der Rest wird übersprungen und gezählt).
 *  - Hardware: Warten auf den fertigen Hop (Thread-Flag aus dem DMA-Interrupt, 116-Hook);
 *    die SAI gibt den Takt vor. Timeout HW_TIMEOUT_MS -> Meldung "116: keine Hops".
 *  - Überlast (Durchlauf länger als ein Hop): vorher MIN_IDLE_MS schlafen.
 * Stats "120": Bearbeitungszeit je Aufwachen (alle Hops dieses Durchlaufs).
 */
#pragma once
#include "TaskBase.hpp"
#include "Processing_Module_120/Processing_Module_120.hpp"
#include "Harness/Signal_Simulator.hpp"
#include "Infrastructure/Utils/HopClock.hpp"

namespace sds110 {

class ProcessingTask : public TaskBase {
public:
    static ProcessingTask& instance() { static ProcessingTask inst; return inst; }
    uint32_t simSkippedHops() const { return clock_.skipped(); }
protected:
    void onStart() override;
    void waitForWork() override;
    void runOnce() override;
private:
    static constexpr uint32_t FLAG_HOP         = 0x1;
    static constexpr uint32_t SIM_MAX_CATCH_UP = 4;     // Hops je Durchlauf (128 ms)
    static constexpr uint32_t HW_TIMEOUT_MS    = 100;   // > 3 Hops ohne Daten
    // Überlast: dauerte ein Durchlauf länger als ein Hop, vor dem nächsten MIN_IDLE_MS
    // schlafen – sonst liefe der Task (AboveNormal) ohne Pause, und LCD/USB/Logger kämen nicht dran
    static constexpr uint32_t MIN_IDLE_MS      = 5;

    ProcessingTask() : TaskBase(16384, 0, osPriorityAboveNormal) {}   // delayMs unbenutzt (waitForWork)
    void updateSource();                 // Simulation <-> Hardware umschalten
    void process();                      // alle bereiten Hops/Frames je Modus
    static void onHopReadyISR(void* ctx);

    Processing_Module_120& proc_ = Processing_Module_120::instance();
    Signal_Simulator&      sim_  = Signal_Simulator::instance();
    HopClock clock_;
    bool simRunning_ = false;
    bool simOn_      = true;       // zuletzt gelesener Wert von SDS_Data::simulation (Standard 1)
    bool hwTimeout_  = false;      // Meldung "keine Hops" nur einmal je Ausfall
    uint32_t runStartTick_ = 0, runEndTick_ = 0;
    SDS_Data& dm_ = SDS_Data::instance();
};

} // namespace sds110
