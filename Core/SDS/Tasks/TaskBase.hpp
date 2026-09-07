/*
 * TaskBase.hpp
 *
 * Mathematical / Physical Description
 * -----------------------------------
 * This class models a deterministic periodic task for STM32 systems using
 * CMSIS‑RTOS2. Each task behaves like a discrete‑time dynamical system:
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
 * This creates a uniform sampling grid for deterministic state evolution.
 *
 * Lifecycle:
 *   1. onStart()   — initialization hook
 *   2. runOnce()   — periodic update loop
 *   3. onExit()    — optional cleanup (rarely reached)
 *
 * The class encapsulates:
 *   - Thread creation (osThreadNew)
 *   - Stack size configuration
 *   - Priority assignment
 *   - Periodic scheduling via osDelay()
 *
 * This abstraction ensures:
 *   - deterministic timing
 *   - uniform task structure
 *   - safe startup sequencing
 *   - consistent FreeRTOS/CMSIS‑RTOS2 integration
 *
 * Created on: Aug 6, 2026
 * Author: Stefan (310004)
 */

#pragma once

#include "cmsis_os2.h"
#include <cstdint>

class TaskBase {
public:
    // Starts the task (only once). Creates the CMSIS‑RTOS2 thread.
    void start();

    // Returns the thread handle (for monitoring or signaling).
    osThreadId_t handle() const { return taskHandle_; }

protected:
    // Base constructor: defines stack size, period, and priority.
    // Derived classes typically call this from their own constructors.
    TaskBase(uint32_t stackSize = 4096,
             uint32_t delayMs   = 50,
             osPriority_t prio  = osPriorityNormal);

    // Must be implemented by derived classes.
    // This function is executed periodically every delayMs milliseconds.
    virtual void runOnce() = 0;

    // Optional hook executed once before the periodic loop starts.
    virtual void onStart() {}

    // Optional hook executed when the thread exits (rarely reached).
    virtual void onExit() {}

    void delay(uint32_t ms);

private:
    // Static trampoline function used by CMSIS‑RTOS2.
    // Converts the void* argument back into a TaskBase instance and
    // executes the lifecycle: onStart() → runOnce() loop → onExit().
    static void threadEntry(void* argument);

protected:
    osThreadId_t taskHandle_ = nullptr;  // RTOS thread handle
    uint32_t     stackSize_;             // Stack size in bytes
    uint32_t     delayMs_;               // Periodic delay in milliseconds
    osPriority_t priority_;              // Thread priority
};
