/*
 * sds_wrapper.hpp
 *
 * System Integration Description
 * ------------------------------
 * This header provides a C‑compatible interface for initializing and
 * starting the SDS (Sensor‑DSP‑System) runtime components. It acts as
 * the bridge between C‑based startup code (main.c) and the C++ task
 * architecture (TaskBase, MicTask, SRPTask, LCDTask).
 *
 * The wrapper exposes:
 *
 *   - SDS_Init()
 *       Initializes global models, buffers, DSP modules, and hardware
 *       subsystems required before any task is started.
 *
 *   - SDS_StartDisplayManagerTask()
 *       Starts the LCDTask responsible for rendering DSP results.
 *
 *   - SDS_StartSRPPhatTask()
 *       Starts the SRPTask responsible for acoustic localization.
 *
 *   - SDS_StartMicTask()
 *       Starts the MicTask responsible for generating or acquiring
 *       microphone frames (real or synthetic).
 *
 *   - SDS_RunSRPUnittest()
 *       Executes SRP‑PHAT unit tests for validation and debugging.
 *
 * All functions are declared inside an extern "C" block to ensure
 * compatibility with STM32 HAL startup code and linker behavior.
 *
 * Created on: Jun 18, 2026
 * Author: Stefan (310004)
 */


//ToDo Send SDS Status

#pragma once

#include "SDRAMDriver.h"
#include "PrintfDriver.h"
#include "MPUDriver.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize SDS system (models, buffers, DSP modules)
void SDS_Init(void);

// Start LCD rendering task
void SDS_StartDisplayManagerTask(void);

// Start SRP‑PHAT DSP task
void SDS_StartSRPPhatTask(void);

// Start microphone acquisition / simulation task
void SDS_StartMicTask(void);

// Start USB task
void SDS_StartUSBTask(void);


// Start USB task
void SDS_StartLoggerTask(void);


// Execute SRP‑PHAT unit tests
void SDS_RunSRPUnittest(void);



#ifdef __cplusplus
}
#endif
