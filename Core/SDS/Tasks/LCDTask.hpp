/*
 * LCDTask.hpp
 *
 * Mathematical / Physical Description
 * -----------------------------------
 * This task renders real‑time DSP results (azimuth, distance, confidence,
 * SRP grid metrics, etc.) onto the LCD using the LCDDriver and PixelEngine.
 *
 * The visualization is based on polar geometry:
 *
 *   - Azimuth θ is mapped to an angle on the display:
 *
 *         θ_rad = θ * (π / 180)
 *
 *   - Distance r is mapped to a radial coordinate:
 *
 *         r_px = distFac * r
 *
 *   where:
 *       distFac = scaling factor (pixels per meter)
 *       R       = maximum radius (visualization boundary)
 *
 * The LCDTask periodically:
 *   1. Reads DSP results from SDS_Data
 *   2. Computes geometric transformations:
 *
 *         x = x0 + r_px * cos(θ_rad)
 *         y = y0 + r_px * sin(θ_rad)
 *
 *   3. Renders:
 *       - source position
 *       - error bounds (errorAz, errorDist)
 *       - confidence indicators
 *       - unit test overlays
 *
 * Timing Model:
 * -------------
 * The task runs periodically with sampling period:
 *
 *      T = delayMs / 1000 seconds
 *
 * ensuring deterministic frame updates and stable visualization.
 *
 * Created on: Aug 10, 2026
 * Author: Stefan (310004)
 */

#pragma once
#include "TaskBase.hpp"

#include "Model.hpp"
#include "Algorithm.hpp"

#include "UnitTest.hpp"
#include "DWTTimer.hpp"

class LCDTask : public TaskBase {
public:
    // Singleton instance — ensures only one LCD renderer runs.
    static LCDTask& instance() { static LCDTask dm; return dm; }

protected:
    // Periodic update — renders DSP results to the LCD.
    void runOnce() override;

    // Initialization hook — prepares LCD driver and buffers.
    void onStart() override;



private:
    // Constructor: initializes base task and visualization parameters.
    LCDTask();

    // Show Radar
    void showRadar();

    // Show System Data
    void showSystemData();

    // Show Detections
    void showDetecktion();

    // Show Errors;
    void showError();

private:
    // Global data model containing DSP results.
    SDS_Data& dm = SDS_Data::instance();

    // High‑resolution cycle timer (DWT).
    DWTTimer& dwt = DWTTimer::instance();

    // Graphics driver for LCD rendering.
    LCDDriver* pGfx = &LCDDriver::instance();

    // Unit test overlay renderer.
    UnitTest unitTest;

    // Frame counter.
    int frame_nr = 0;

    // DSP results and visualization parameters.
    float bestAz    = 0.0f;
    float difAz     = 0.0f;
    float bestDist  = 0.0f;
    float difDist   = 0.0f;
    float percent   = 0.0f;

    // Error thresholds for visualization.
    float errorAz   = 3.0f;     // ±3 degrees
    float errorDist = 15.0f;    // ±15%

    // Polar visualization geometry.
    int   x0       = 359;       // center X
    int   y0       = 136;       // center Y
    float R        = 120.0f;    // maximum radius
    float deg2rad  = 3.1415f / 180.0f;
    float distFac  = 120.0f / 100.0f; // pixel scaling for max 100m

    char buf[128];
};
