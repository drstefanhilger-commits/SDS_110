/*
 * TaskBase.cpp
 *
 * Mathematical / Physical Description
 * -----------------------------------
 * This file implements the runtime behavior of a deterministic periodic
 * task for STM32 systems using CMSIS‑RTOS2. The task behaves like a
 * discrete‑time dynamical system:
 *
 *      x[k+1] = f( x[k], u[k] )
 *
 * where:
 *   - k is the discrete time index (each iteration of runOnce())
 *   - f is the task‑specific update function implemented in runOnce()
 *   - u[k] represents external inputs (sensor data, events, messages)
 *
 * The execution period is defined by:
 *
 *      T = delayMs / 1000 seconds
 *
 * The thread lifecycle is:
 *   1. onStart()   — initialization hook
 *   2. runOnce()   — periodic update loop
 *   3. onExit()    — optional cleanup (rarely reached)
 *
 * The threadEntry() function acts as a trampoline converting the void*
 * argument from CMSIS‑RTOS2 into a TaskBase instance and executing the
 * deterministic lifecycle.
 *
 * Created on: Aug 6, 2026
 * Author: Stefan (310004)
 */

#include "TaskBase.hpp"

// ---------------------------------------------------------------------------
// Constructor: stores stack size, period, and priority.
// ---------------------------------------------------------------------------
TaskBase::TaskBase(uint32_t stackSize,
                   uint32_t delayMs,
                   osPriority_t prio)
    : stackSize_(stackSize),
      delayMs_(delayMs),
      priority_(prio)
{
    // No dynamic allocation, no side effects — MISRA‑friendly.
}

// ---------------------------------------------------------------------------
// Start the task (only once). Creates the CMSIS‑RTOS2 thread.
// ---------------------------------------------------------------------------
void TaskBase::start()
{
    if (taskHandle_ != nullptr) {
        // Task already started — ignore repeated calls.
        return;
    }

    // Thread attributes: stack size and priority.
    const osThreadAttr_t attr = {
        .stack_size = stackSize_,
        .priority   = priority_
    };

    // Create the thread. The argument "this" is passed to threadEntry().
    taskHandle_ = osThreadNew(threadEntry, this, &attr);
}

// ---------------------------------------------------------------------------
// Static trampoline function used by CMSIS‑RTOS2.
// Converts void* argument back into a TaskBase instance and executes:
//   onStart() → periodic runOnce() loop → onExit()
// ---------------------------------------------------------------------------
void TaskBase::threadEntry(void* argument)
{
    TaskBase* self = static_cast<TaskBase*>(argument);

    self->onStart();

    self->lastRunCycles_ = self->dwt.instance().cycles();

    while (true)
    {
    	// --- Get current cycle
        uint32_t t0 = self->dwt.instance().cycles();

        // --- Jitter ---
        self->jitterCycles_ = t0 - self->lastRunCycles_;
        self->lastRunCycles_ = t0;

        // --- runOnce() execution time ---
        self->runOnce();
        uint32_t t1 = self->dwt.instance().cycles();
        self->execTimeCycles_ = t1 - t0;

        // --- Stack usage ---
        self->freeStackBytes_ = osThreadGetStackSpace(self->taskHandle_);

        // --- Increment Loop Number ---
        self->loopNr_++;

        // --- Loop delay ---
        self->delay_(self->delayMs_);
    }

    self->onExit();
}


void TaskBase::delay_(uint32_t ms) {
	osDelay(ms);
}

