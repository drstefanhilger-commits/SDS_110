/*
 * LoggerTask.cpp  (Infrastructure/Tasks)
 */
#include "LoggerTask.hpp"
#include "Infrastructure/Driver/USBDriver.hpp"
#include <cstring>

namespace sds110 {

void LoggerTask::runOnce()
{
    const int n = logger_.read(buf_, sizeof(buf_));
    if (n > 0) {
        MessageData data{};
        memcpy(data.b, buf_, n);
        USBDriver::sendMessage(99, 0, data);
    }
    reportStats(TaskId::Logger);
}

} // namespace sds110
