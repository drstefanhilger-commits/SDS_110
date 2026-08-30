/*
 * LoggerTask.cpp
 *
 *  Created on: Aug 30, 2026
 *      Author: 310004
 */

#include <LoggerTask.hpp>

// ---------------------------------------------------------------------------
// Constructor: initializes base task with stack size and period.
// ---------------------------------------------------------------------------
LoggerTask::LoggerTask()
    : TaskBase(1024, 5, osPriorityLow)
{
    // No dynamic allocation — deterministic initialization.
}

// ---------------------------------------------------------------------------
// Initialization hook — executed once before periodic processing.
// ---------------------------------------------------------------------------
void LoggerTask::onStart()
{
    // Simulation state could be initialized here if needed.
}

// ---------------------------------------------------------------------------
// Periodic update — generates synthetic microphone signals.
// ---------------------------------------------------------------------------
void LoggerTask::runOnce()
{
	int n = pLogger->read(buf, sizeof(buf));
	//anderes Message Format
	if ( n > 0) {
		uint32_t timestamp = 0;
		USB_SendLogging(timestamp, buf, sizeof(buf));
//		 CDC_Transmit_FS(buf, n);
	}
}
