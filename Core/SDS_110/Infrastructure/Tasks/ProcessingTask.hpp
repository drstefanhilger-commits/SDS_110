/*
 * ProcessingTask.hpp  (Infrastructure/Tasks)
 * RTOS-Task um Processing_Module_120: DETECT -> processFrame(), READ -> streamFrame().
 * Ersetzt SDS/Tasks/SRPTask. Periode 10 ms, verarbeitet alle fertigen Frames (~15/s bei 64 ms).
 */
#pragma once
#include "TaskBase.hpp"
#include "Processing_Module_120/Processing_Module_120.hpp"
#include "Harness/Signal_Simulator.hpp"

namespace sds110 {

class ProcessingTask : public TaskBase {
public:
    static ProcessingTask& instance() { static ProcessingTask inst; return inst; }
protected:
    void onStart() override;
    void runOnce() override;
private:
    ProcessingTask() : TaskBase(16384, 40, osPriorityAboveNormal) {}   // 40 ms: ~1 Sim-Frame (64 ms) je 1,5 Aufrufe
    void feedSimulation();
    Processing_Module_120& proc_ = Processing_Module_120::instance();
    Signal_Simulator&      sim_  = Signal_Simulator::instance();
    bool simRunning_ = false;
    SDS_Data& dm_ = SDS_Data::instance();
};

} // namespace sds110
