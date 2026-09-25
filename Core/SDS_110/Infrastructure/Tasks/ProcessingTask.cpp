/*
 * ProcessingTask.cpp  (Infrastructure/Tasks)
 */
#include "ProcessingTask.hpp"
#include "stm32f7xx_hal.h"
#include "Infrastructure/Utils/TimeBase.hpp"

namespace sds110 {

void ProcessingTask::onStart()
{
    SimParams sp;                       // Grundwerte: SIM_* in SDS_110_Config.hpp
    sp.scenario = static_cast<SimScenario>(SIM_SCENARIO_ID);
    sp.f0_hz    = SIM_F0_HZ;
    sp.snr_db   = SIM_SNR_DB;
    sim_.init(sp);
}

void ProcessingTask::feedSimulation()
{
    // Simulation (USB-Kommando Typ 3, Standard 1): Generator statt SAI/DMA.
    // Ein Frame pro Aufruf; die Verarbeitung im selben Durchlauf hält 114 frei.
    const bool sim = dm_.getSimulation() != 0;
    if (sim && !simRunning_)  { proc_.unit().sampling().stop(); simRunning_ = true; }
    if (!sim && simRunning_)  { if (!proc_.start()) dm_.pushErrorMessage("116 start failed"); simRunning_ = false; }
    if (!sim) return;

    sim_.generateFrame(TimeBase::nowUs());   // gleiche Zeitbasis wie 116
    dm_.setDebugValue(2, sim_.trueAzimuth());     // LCD: "True Azimuth"
    dm_.setDebugValue(3, sim_.trueDistance());    // LCD: "True Distance"
}

void ProcessingTask::runOnce()
{
    feedSimulation();
    switch (dm_.getMode()) {
        case SDS_Mode::DETECT:
            while (proc_.processFrame()) {}
            break;
        case SDS_Mode::READ:
            proc_.streamFrame();
            break;
        case SDS_Mode::CALIBRATE:
        default:
            break;
    }
    reportStats(TaskId::Proc120);
}

} // namespace sds110
