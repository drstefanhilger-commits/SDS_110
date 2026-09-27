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
    clock_.start(osKernelGetTickCount(), osKernelGetTickFreq());
    proc_.unit().sampling().setHopReadyHook(&ProcessingTask::onHopReadyISR, this);
}

void ProcessingTask::onHopReadyISR(void* ctx)
{
    osThreadFlagsSet(static_cast<ProcessingTask*>(ctx)->handle(), FLAG_HOP);
}

void ProcessingTask::waitForWork()
{
    if (runEndTick_ - runStartTick_ >= HOP_SAMPLES * osKernelGetTickFreq() / SAMPLE_RATE_HZ) osDelay(MIN_IDLE_MS);
    if (simOn_) {
        osDelayUntil(clock_.nextTick());         // bereits fällig: kehrt sofort zurück
        return;
    }
    const uint32_t r = osThreadFlagsWait(FLAG_HOP, osFlagsWaitAny, HW_TIMEOUT_MS);
    const bool timeout = (r & osFlagsError) != 0;
    if (timeout && proc_.unit().sampling().running() && !hwTimeout_) dm_.pushErrorMessage("116: keine Hops");
    hwTimeout_ = timeout;
}

void ProcessingTask::updateSource()
{
    // Simulation (USB-Kommando Typ 3, Standard 1): Generator statt SAI/DMA.
    // Sperr-Timeout: letzten Wert behalten, nicht auf Hardware umschalten (Befund 29)
    uint32_t simFlag = 0;
    if (dm_.tryGetSimulation(simFlag)) simOn_ = simFlag != 0;
    if (simOn_ && !simRunning_) {
        proc_.unit().sampling().stop();
        clock_.start(osKernelGetTickCount(), osKernelGetTickFreq());   // Takt neu ab jetzt
        simRunning_ = true;
    }
    if (!simOn_ && simRunning_) {
        simRunning_ = false;
        hwTimeout_  = false;
        osThreadFlagsClear(FLAG_HOP);
        if (!proc_.unit().sampling().dmaReady()) dm_.pushErrorMessage("116: SAI ohne DMA");
        else if (!proc_.start())                 dm_.pushErrorMessage("116 start failed");
    }
}

void ProcessingTask::process()
{
    switch (dm_.getMode()) {
        case SDS_Mode::DETECT:
            while (proc_.processFrame()) {}
            break;
        case SDS_Mode::READ:
            while (proc_.streamFrame()) {}
            break;
        case SDS_Mode::CALIBRATE:
        default:
            break;
    }
}

void ProcessingTask::runOnce()
{
    runStartTick_ = osKernelGetTickCount();
    updateSource();
    if (simOn_) {
        // alle fälligen Hops, je Hop sofort verarbeiten (114 hat nur NUM_MIC_FRAMES Puffer)
        const uint32_t n = clock_.due(osKernelGetTickCount(), SIM_MAX_CATCH_UP);
        for (uint32_t i = 0; i < n; ++i) {
            sim_.generateHop(TimeBase::nowUs());     // gleiche Zeitbasis wie 116
            process();
        }
        dm_.setDebugValue(2, sim_.trueAzimuth());     // LCD: "True Azimuth"
        dm_.setDebugValue(3, sim_.trueDistance());    // LCD: "True Distance"
    } else {
        process();
    }
    reportStats(TaskId::Proc120);
    runEndTick_ = osKernelGetTickCount();
}

} // namespace sds110
