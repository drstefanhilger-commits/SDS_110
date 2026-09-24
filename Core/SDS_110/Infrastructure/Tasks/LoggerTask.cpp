/*
 * LoggerTask.cpp  (Infrastructure/Tasks)
 */
#include "LoggerTask.hpp"
#include "Infrastructure/Timer/HardwareTimer.hpp"
#include "Infrastructure/Driver/USBDriver.hpp"
#include "Infrastructure/Model/SDS_Data.hpp"
#include "cmsis_os2.h"
#include <cstring>

namespace {
// TIM7: Basic-Timer auf APB1 (108 MHz bei 216 MHz SYSCLK / APB1 DIV4)
sds110::HardwareTimer loggerTimer(TIM7, TIM7_IRQn, 6);
}

extern "C" void TIM7_IRQHandler(void)
{
    loggerTimer.handleInterrupt();
}

namespace sds110 {

LoggerTask::LoggerTask()
    : TaskTimerBase("LoggerTask", 1024 /*Bytes*/, static_cast<UBaseType_t>(osPriorityLow))
{
    const bool ok = loggerTimer.init(kRateHz);   // Timer-Takt aus RCC
    configASSERT(ok);
    attachTimer(&loggerTimer);
}

void LoggerTask::onTask()
{
    // Ringpuffer leeren: mehrere Pakete je Takt, damit 10 Hz denselben
    // Durchsatz schaffen wie der alte 5-ms-LoggerTask (128 B je Aufruf).
    for (uint32_t i = 0; i < kMaxMsgPerTick; ++i)
    {
        if (pending_ == 0) {
            pending_ = logger_.read(buf_, sizeof(buf_));
            if (pending_ <= 0) { pending_ = 0; break; }   // Puffer leer
        }

        MessageData data{};
        memcpy(data.b, buf_, static_cast<size_t>(pending_));
        if (!USBDriver::sendMessage(kMsgId, 0, data))
            break;                  // USB belegt -> Paket bleibt in buf_, nächster Takt
        pending_ = 0;
    }

    if (isOverrun()) {
        ++droppedTicks_;
        clearOverrun();
    }

    SDS_Data::instance().setTaskStats(TaskId::Logger, freeStackBytes(),
                                      maxExecUs() / 1000.0f, loopCount());
}

} // namespace sds110
