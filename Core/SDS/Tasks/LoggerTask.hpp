/*
 * LoggerTask.h
 *
 *  Created on: Aug 30, 2026
 *      Author: 310004
 */


#pragma once
#include "TaskBase.hpp"
#include "USBDriver.hpp"
#include "Logger.hpp"

class LoggerTask : public TaskBase {
public:
    // Singleton instance — ensures only one microphone simulator runs.
    static LoggerTask& instance() { static LoggerTask inst; return inst; }

protected:
    // Periodic Task update — send Message from Buffer
    void runOnce() override;

    // Initialization hook — prepares buffers, timers, and DSP structures.
    void onStart() override;

private:
    // Constructor: initializes base task and simulation parameters.
    LoggerTask();

    Logger* pLogger = &Logger::instance();

private:
    uint8_t buf[128];

};

