/*
 * ProcessingTask.cpp  (Infrastructure/Tasks)
 */
#include "ProcessingTask.hpp"

namespace sds110 {

void ProcessingTask::onStart()
{
    if (!proc_.start()) dm_.pushErrorMessage("116 start failed");
}

void ProcessingTask::runOnce()
{
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
