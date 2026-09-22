/*
 * LoggerTask1.hpp
 *
 *  Created on: Sep 22, 2026
 *      Author: 310004
 */

#pragma once
#include "Infrastructure/Timer/TaskTimerBase.hpp"
#include "Infrastructure/Utils/DWT.hpp"

namespace sds110 {

class LoggerTask1 : public TaskTimerBase
{
public:
    static LoggerTask1& instance()
    {
        static LoggerTask1 inst;
        return inst;
    }

protected:
    void onTask() override;

private:
    LoggerTask1();
    sds110::DWTTimer& dwt = sds110::DWTTimer::instance();
};

} // namespace sds110
