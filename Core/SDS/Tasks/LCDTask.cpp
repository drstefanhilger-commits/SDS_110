/*
 * LCDTask.cpp
 *
 * Mathematical / Physical Description
 * -----------------------------------
 * This task renders real‑time DSP results (azimuth, distance, confidence,
 * timing metrics, etc.) onto the LCD using the LCDDriver and PixelEngine.
 *
 * The visualization is based on polar geometry:
 *
 *   Azimuth θ (degrees) is converted to radians:
 *
 *       θ_rad = θ * (π / 180)
 *
 *   Distance d (meters) is mapped to a radial pixel coordinate:
 *
 *       r_px = distFac * d
 *
 * The source position is drawn using:
 *
 *       x = x0 + r_px * cos(θ_rad)
 *       y = y0 + r_px * sin(θ_rad)
 *
 * where (x0, y0) is the center of the polar plot.
 *
 * The task also visualizes:
 *   - error bounds (errorAz, errorDist)
 *   - true vs. estimated values
 *   - frame counter
 *   - diagnostic timing values
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

#include "LCDTask.hpp"

// ---------------------------------------------------------------------------
// Constructor: initializes base task and LCD graphics engine.
// ---------------------------------------------------------------------------
LCDTask::LCDTask()
    : TaskBase(2024, 50, osPriorityNormal)
{
    // Initialize LCD graphics engine with framebuffer and resolution.
    pGfx->init(pGfx->pStartFrameBuffer, 480, 272);
}

// ---------------------------------------------------------------------------
// Initialization hook — executed once before periodic processing.
// ---------------------------------------------------------------------------
void LCDTask::onStart()
{
    // LCD initialization or splash screen could be placed here.
}

volatile uint32_t usb_debug_counter = 0;
//extern "C" volatile uint32_t usb_debug_counter;
//extern "C" volatile uint32_t usb_sds_counter;
//extern "C" volatile uint32_t usb_sds_counter2;

//extern "C" {
//    extern volatile uint32_t usb_debug_counter;
//    extern volatile uint32_t usb_debug_counter2;
//}

// ---------------------------------------------------------------------------
// Periodic update — renders DSP results to the LCD.
// ---------------------------------------------------------------------------
void LCDTask::runOnce()
{
    // Clear screen
    pGfx->clear(Color::Black);
    switch (dm.getMode()) {
    case 1:
        showRadar();
        showSystemData();
    	showDetecktion();
    	break;
    case 2:
        showSystemData();
    	break;
    case 3:
        showSystemData();
        showAI();
    	break;
    default:
    	break;
    }

    showError();
    // Swap framebuffer (double buffering)
    pGfx->activateFrameBuffer();

    dm.setLcdLoopTime(execTimeCycles_);
    dm.setLcdTaskFreeStack(freeStackBytes_);
    dm.setLcdLoopCounter(loopNr_);
}

// Show Radar
void LCDTask::showRadar() {
    // Version
    pGfx->text8x12(420, 10, "1.09", Color::Green);

    // Draw reference geometry
    pGfx->line(235, 10, 235, 242, Color::Red);     							// vertical reference line
    pGfx->circle(x0, y0, R,   Color::White);       							// outer circle
    pGfx->circle(x0, y0, dm.getDebugValue3()*distFac, Color::Yellow);       // inner circle (trueDist)

    if (dm.getMode() == 1) {
		// Convert DSP azimuth to radians
		float angle = dm.getAzimuth() * deg2rad;

		// Convert DSP distance to pixel radius
		float r1 = dm.getDistance() * distFac;

		// Compute source position in display coordinates
		int x1 = x0 + static_cast<int>(r1 * cosf(angle));
		int y1 = y0 + static_cast<int>(r1 * sinf(angle));


		// Draw line from center to estimated source position
		pGfx->line(x0, y0, x1, y1, Color::Red);
		pGfx->circle(x1, y1, 3, Color::Green);
    }

}

// Show System Data
void LCDTask::showSystemData() {
    // -----------------------------------------------------------------------
    // Debug values (temporary placeholders)
    // -----------------------------------------------------------------------
    switch (dm.getMode()) {
    case 1: 	pGfx->text8x12(10, 170, "Mode: DETECT", Color::White); break;
    case 2: 	pGfx->text8x12(10, 170, "Mode: READ", Color::White); break;
    case 3: 	pGfx->text8x12(10, 170, "Mode: CALI", Color::White); break;
    default: 	pGfx->text8x12(10, 170, "Mode: ERROR", Color::White); break;
    }
    snprintf(buf, sizeof(buf), "%s", (dm.getSimulation()== 0 ? "Real" : "Simulated"));
    pGfx->text8x12(145, 170, buf, Color::White);

    snprintf(buf, sizeof(buf), "SRP [%7.3f %4lu %6lu]", dm.getSrpLoopTime()/216000.0f, dm.getSrpLoopCounter(), dm.getSrpTaskFreeStack());
    pGfx->text8x12(10, 120, buf, Color::White);

    snprintf(buf, sizeof(buf), "LCD [%7.3f %4lu %6lu]", dm.getLcdLoopTime()/216000.0f, dm.getLcdLoopCounter(), dm.getLcdTaskFreeStack());
    pGfx->text8x12(10, 130, buf, Color::White);

    snprintf(buf, sizeof(buf), "Mic [%7.3f %4lu %6lu]", dm.getMicLoopTime()/216000.0f, dm.getMicLoopCounter(), dm.getMicTaskFreeStack());
    pGfx->text8x12(10, 140, buf, Color::White);


    snprintf(buf, sizeof(buf), "Loop Srp       %ld", dm.getSrpLoopCounter());
    pGfx->text8x12(10, 190, buf, Color::White);

    snprintf(buf, sizeof(buf), "Loop Mic       %ld", dm.getMicLoopCounter());
    pGfx->text8x12(10, 200, buf, Color::White);

    snprintf(buf, sizeof(buf), "Loop LCD       %ld", dm.getLcdLoopCounter());
    pGfx->text8x12(10, 210, buf, Color::White);

    uint32_t srpLoop = dm.getSrpLoopCounter();
    uint32_t micLoop = dm.getMicLoopCounter();
    if ((srpLoop != 0) && (micLoop != 0))  {
    	snprintf(buf, sizeof(buf), "               %.2f", (float)micLoop/(float)srpLoop);
        pGfx->text8x12(10, 220, buf, Color::White);
    }

    snprintf(buf, sizeof(buf), "USB timeSync   %ld", usb_debug_counter);
    pGfx->text8x12(10, 230, buf, Color::White);
}

// Show Detections();
void LCDTask::showDetecktion() {

    difAz = fabs(dm.getAzimuth() - dm.getDebugValue2());
    Color colorAz = (difAz < errorAz ? Color::Green : Color::Red);

    snprintf(buf, sizeof(buf), "Azimuth        %.3f", dm.getAzimuth());
    pGfx->text8x12(10, 10, buf, colorAz);

    float difDist = fabsf(dm.getDistance() - dm.getDebugValue3());
    percent = 100.0f * difDist / dm.getDebugValue3();
    Color colorDis = (percent < errorDist ? Color::Green : Color::Red);

    snprintf(buf, sizeof(buf), "Distance       %.1f", dm.getDistance());
    pGfx->text8x12(10, 20, buf, colorDis);

    if (dm.getSimulation()==1) {

		snprintf(buf, sizeof(buf), "Dif Azimuth    %.3f", difAz);
		pGfx->text8x12(10, 70, buf, colorAz);

		snprintf(buf, sizeof(buf), "Dif Distance   %.3f", difDist);
		pGfx->text8x12(10, 80, buf, colorDis);

		snprintf(buf, sizeof(buf), "True Azimuth   %.3f", dm.getDebugValue2());
		pGfx->text8x12(10, 40, buf, Color::White);

		snprintf(buf, sizeof(buf), "True Distance  %.3f", dm.getDebugValue3());
		pGfx->text8x12(10, 50, buf, Color::White);
    }
}

void LCDTask::showAI() {
    snprintf(buf, sizeof(buf), "Drone          %.3f", dm.getAiDrone());
    pGfx->text8x12(10, 10, buf, Color::White);

    snprintf(buf, sizeof(buf), "Wind           %.3f", dm.getAiWind());
    pGfx->text8x12(10, 20, buf, Color::White);

    snprintf(buf, sizeof(buf), "Human          %.3f", dm.getAiHuman());
    pGfx->text8x12(10, 30, buf, Color::White);

    snprintf(buf, sizeof(buf), "Background     %.3f", dm.getAiBackground());
    pGfx->text8x12(10, 40, buf, Color::White);

    if (dm.getAiInitError()) {
    	snprintf(buf, sizeof(buf), "AI Init Error");
    } else {
    	snprintf(buf, sizeof(buf), "No AI Init Error");
    }
    pGfx->text8x12(10, 50, buf, (dm.getAiInitError() ? Color::Red : Color::Green));

    if (dm.getAiRunError()) {
    	snprintf(buf, sizeof(buf), "AI Run Error");
    } else {
    	snprintf(buf, sizeof(buf), "No AI Run Error");
    }
    pGfx->text8x12(10, 60, buf, (dm.getAiRunError() ? Color::Red : Color::Green));
}

void LCDTask::showError() {

	SDS_ErrorMessage msg;
	if (dm.popErrorMessage(msg)) {
		pGfx->text8x12(10, 80, msg.text, Color::White);
	}

	if (dm.getErrorFlag()) {
		uint32_t count = dm.getErrorCount();
		if (count > 0) {
			dm.setErrorCount(count - 1);
			uint8_t* ebuf = dm.getErrorBuffer();
		    snprintf(buf, sizeof(buf), "%x %x %x %x   %x %x %x %x   %x %x %x %x ",
		    							ebuf[0], ebuf[1], ebuf[2], ebuf[3], ebuf[4], ebuf[5],
										ebuf[6], ebuf[7], ebuf[8], ebuf[9], ebuf[10], ebuf[11]);
		    pGfx->text8x12(10, 250, buf, Color::White);
		} else {
			dm.setErrorFlag(0);
			dm.setErrorCount(0);
		}
	}


}
