/*
 * LoggerTask.hpp  (Infrastructure/Tasks)
 * Leert den Logger-Ringpuffer und sendet ihn als Message id 99 über USB.
 * Migration aus SDS/Tasks/LoggerTask: USBDriver::sendMessage statt SDS_SendMessage.
 */
#pragma once
#include "TaskBase.hpp"
#include "Infrastructure/Utils/Logger.hpp"

namespace sds110 {

class LoggerTask : public TaskBase {
public:
    static LoggerTask& instance() { static LoggerTask inst; return inst; }
protected:
    void runOnce() override;
private:
    LoggerTask() : TaskBase(1024, 5, osPriorityLow) {}
    Logger& logger_ = Logger::instance();
    uint8_t buf_[128];
};

} // namespace sds110

