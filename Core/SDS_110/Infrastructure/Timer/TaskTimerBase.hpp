/*
 * TaskTimerBase.hpp
 *
 *  Created on: Sep 22, 2026
 *      Author: 310004
 */

#pragma once
#include <atomic>
#include "FreeRTOS.h"
#include "task.h"
#include "HardwareTimer.hpp"

class TaskTimerBase
{
public:
    void attachTimer(HardwareTimer* timer);
    void start();
    void stop();

    bool isOverrun() const { return overrunFlag.load(); }
    void clearOverrun() { overrunFlag.store(false); }
    float getAllowedTimeUs() const { return allowedTimeUs; }
    void setAllowedTimeUs(float us) { allowedTimeUs = us; }


protected:
    TaskTimerBase(const char* taskName, uint16_t stackSize, UBaseType_t priority);

    virtual void onTask() = 0;

    std::atomic<bool> overrunFlag;
    std::atomic<bool> taskRunning;

private:
    static void taskEntry(void* arg);
    void taskLoop();
    void onTimerISR();

    HardwareTimer* timer;
    TaskHandle_t taskHandle;
    float allowedTimeUs = 0.0f;

};
