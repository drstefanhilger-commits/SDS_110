/*
 * sds_wrapper.cpp
 *
 * System Integration Description
 * ------------------------------
 * This file implements the C‑compatible wrapper functions used to
 * initialize and start the SDS (Sensor‑DSP‑System) runtime components.
 * It provides a stable interface for STM32 HAL startup code (main.c)
 * and ensures that the C++ task architecture is accessible from C.
 *
 * Architectural Role:
 * -------------------
 * The SDS wrapper defines the deterministic startup sequence:
 *
 *   1. SDS_Init()
 *        Initializes global models, buffers, DSP modules, timers,
 *        and any hardware subsystems required before tasks begin.
 *
 *   2. SDS_StartDisplayManagerTask()
 *        Starts the LCDTask responsible for rendering DSP results.
 *
 *   3. SDS_StartSRPPhatTask()
 *        Starts the SRPTask responsible for acoustic localization
 *        using SRP‑PHAT and DAS‑based distance estimation.
 *
 *   4. SDS_StartMicTask()
 *        Starts the MicTask responsible for microphone acquisition
 *        or synthetic test signal generation.
 *
 *   5. SDS_StartUSBTask()
 *        Starts the USBTask responsible for Sending and receiving
 *        USB Messages..
 *
 *   6. SDS_RunSRPUnittest()
 *        Executes SRP‑PHAT unit tests for validation and debugging.
 *
 * All functions are wrapped in extern "C" to ensure correct linkage
 * with C‑based startup code and to avoid name mangling.
 *
 * Created on: Jun 18, 2026
 * Author: Stefan (310004)
 */

#include "LCDTask.hpp"
#include "SRPTask.hpp"
#include "MicTask.hpp"
#include "USBTask.hpp"
#include "LoggerTask.hpp"

#include "UnitTest.hpp"

extern "C" {

    // -----------------------------------------------------------------------
    // Initialize SDS system (models, buffers, DSP modules)
    // -----------------------------------------------------------------------
    void SDS_Init(void)
    {
        // Initialization logic can be added here when required.
        // Typical responsibilities:
        //   - SDS_Data::instance().init();
        //   - SDS_MicrophoneBuffer::instance().init();
        //   - Hardware initialization (if not done in main.c)
        //   - DSP module pre‑initialization
    }

    // -----------------------------------------------------------------------
    // Start LCD rendering task
    // -----------------------------------------------------------------------
    void SDS_StartDisplayManagerTask(void)
    {
        LCDTask::instance().start();
    }

    // -----------------------------------------------------------------------
    // Start SRP‑PHAT DSP task
    // -----------------------------------------------------------------------
    void SDS_StartSRPPhatTask(void)
    {
        SRPTask::instance().start();
    }

    // -----------------------------------------------------------------------
    // Start microphone acquisition / simulation task
    // -----------------------------------------------------------------------
    void SDS_StartMicTask(void)
    {
        MicTask::instance().start();
    }

    // -----------------------------------------------------------------------
    // Start USB sending / receiving task
    // -----------------------------------------------------------------------
    void SDS_StartUSBTask(void)
    {
        USBTask::instance().start();
    }


    void SDS_StartLoggerTask(void)
    {
    	LoggerTask::instance().start();
    }

    // -----------------------------------------------------------------------
    // Execute SRP‑PHAT unit tests
    // -----------------------------------------------------------------------
    void SDS_RunSRPUnittest(void)
    {
        // Unit tests can be triggered here:
        // UnitTest::runSRPTests();
    }

}
