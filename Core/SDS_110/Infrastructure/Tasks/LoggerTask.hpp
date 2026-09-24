/*
 * LoggerTask.hpp  (Infrastructure/Tasks)
 * Ersetzt LoggerTask: leert den Logger-Ringpuffer und sendet ihn als
 * Message id 99 über USB. Getaktet durch TIM7 über TaskTimerBase.
 */
#pragma once
#include <cstdint>
#include "Infrastructure/Timer/TaskTimerBase.hpp"
#include "Infrastructure/Utils/Logger.hpp"

namespace sds110 {

class LoggerTask : public TaskTimerBase
{
public:
    static LoggerTask& instance()
    {
        static LoggerTask inst;
        return inst;
    }

    static constexpr float    kRateHz        = 10.0f;  // Takt des Loggers
    static constexpr uint32_t kMaxMsgPerTick = 8;      // max. USB-Pakete je Takt (8 x 128 B)
    static constexpr uint32_t kMsgId         = 99;     // wie bisher

protected:
    void onTask() override;

private:
    LoggerTask();
    LoggerTask(const LoggerTask&) = delete;
    LoggerTask& operator=(const LoggerTask&) = delete;

    Logger&  logger_ = Logger::instance();
    uint8_t  buf_[128];
    int      pending_ = 0;       // Bytes in buf_, die noch nicht gesendet wurden
    uint32_t droppedTicks_ = 0;  // Takte mit Overrun (für Diagnose)
};

} // namespace sds110
