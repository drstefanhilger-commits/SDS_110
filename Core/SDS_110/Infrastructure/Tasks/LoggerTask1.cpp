/*
 * LoggerTask1.cpp
 *
 *  Created on: Sep 22, 2026
 *      Author: 310004
 */

#include "LoggerTask1.hpp"
#include "Infrastructure/Timer/HardwareTimer.hpp"


// Timer-Instanz für LoggerTask1
static HardwareTimer LoggerTimer1(TIM7, TIM7_IRQn);

extern "C" void TIM7_IRQHandler()
{
    LoggerTimer1.handleInterrupt();
}

namespace sds110 {

LoggerTask1::LoggerTask1()
    : TaskTimerBase("LoggerTask1", 2048, tskIDLE_PRIORITY + 1)
{
	// Timer an Task binden
    attachTimer(&LoggerTimer1);

    // Logger läuft mit 10 Hz
    LoggerTimer1.init(216000000, 10.0f);
}

void LoggerTask1::onTask()
{
    // --- Logging Code ---
    if (isOverrun())
    {
    	for (int i = 0; i<100; ++i) {

    	}
    } else {
    	clearOverrun();
    	//ToDo Error
    }

}

}
