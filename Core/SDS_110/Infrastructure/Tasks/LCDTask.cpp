/*
 * LCDTask.cpp  (Infrastructure/Tasks)
 */
#include "LCDTask.hpp"
#include <cmath>
#include <cstdio>

namespace sds110 {

LCDTask::LCDTask() : TaskBase(2048, 50, osPriorityNormal)
{
    gfx_->init(gfx_->pStartFrameBuffer, 480, 272);
}

void LCDTask::runOnce()
{
    gfx_->clear(Color::Black);
    switch (dm_.getMode()) {
        case SDS_Mode::DETECT:
            showRadar(); showSystemData(); showDetection(); break;
        case SDS_Mode::CALIBRATE:
            showSystemData(); break;
        case SDS_Mode::READ:
            showSystemData(); showAcousticState(); break;
        default: break;
    }
    showError();
    gfx_->activateFrameBuffer();
    reportStats(TaskId::Lcd);
}

void LCDTask::showRadar()
{
    gfx_->text8x12(420, 10, "1.10", Color::Green);              // Version
    snprintf(buf_, sizeof(buf_), "Unit %04X", dm_.getId());
    gfx_->text8x12(330, 10, buf_, Color::Green);
    gfx_->line(235, 10, 235, 242, Color::Red);
    gfx_->circle(x0_, y0_, R_, Color::White);
    gfx_->circle(x0_, y0_, dm_.getDebugValue(3) * distFac_, Color::Yellow);   // wahre Distanz (Sim)

    const float angle = dm_.getAzimuth() * deg2rad_;
    const float r1    = dm_.getDistance() * distFac_;
    const int x1 = x0_ + static_cast<int>(r1 * cosf(angle));
    const int y1 = y0_ + static_cast<int>(r1 * sinf(angle));
    gfx_->line(x0_, y0_, x1, y1, dm_.getDetected() ? Color::Red : Color::DarkGray);
    gfx_->circle(x1, y1, 3, Color::Green);
}

void LCDTask::taskLine(int y, const char* name, TaskId id)
{
    const TaskStats s = dm_.getTaskStats(id);
    snprintf(buf_, sizeof(buf_), "%s [%7.3f %4lu %6lu]", name,
             static_cast<double>(s.loopTime), static_cast<unsigned long>(s.loopCounter),
             static_cast<unsigned long>(s.freeStack));
    gfx_->text8x12(10, y, buf_, Color::White);
}

void LCDTask::showSystemData()
{
    const char* modeTxt = "Mode: ERROR";
    switch (dm_.getMode()) {
        case SDS_Mode::DETECT:    modeTxt = "Mode: DETECT"; break;
        case SDS_Mode::CALIBRATE: modeTxt = "Mode: CALI";   break;
        case SDS_Mode::READ:      modeTxt = "Mode: READ";   break;
        default: break;
    }
    gfx_->text8x12(10, 170, modeTxt, Color::White);
    gfx_->text8x12(145, 170, dm_.getSimulation() == 0 ? "Real" : "Simulated", Color::White);

    taskLine(120, "120", TaskId::Proc120);
    taskLine(130, "LCD", TaskId::Lcd);
    taskLine(140, "USB", TaskId::Usb);
    taskLine(150, "Log", TaskId::Logger);

    snprintf(buf_, sizeof(buf_), "Reports        %lu", static_cast<unsigned long>(dm_.getReportCount()));
    gfx_->text8x12(10, 190, buf_, Color::White);
    snprintf(buf_, sizeof(buf_), "Sel. bands     %lu", static_cast<unsigned long>(dm_.getSelectedBands()));
    gfx_->text8x12(10, 200, buf_, Color::White);
    snprintf(buf_, sizeof(buf_), "USB timeSync   %lu", static_cast<unsigned long>(usb_debug_counter));
    gfx_->text8x12(10, 230, buf_, Color::White);
}

void LCDTask::showDetection()
{
    const float trueAz   = dm_.getDebugValue(2);
    const float trueDist = dm_.getDebugValue(3);
    const float difAz    = fabsf(dm_.getAzimuth() - trueAz);
    const float difDist  = fabsf(dm_.getDistance() - trueDist);
    const float percent  = (trueDist > 0.0f) ? 100.0f * difDist / trueDist : 0.0f;
    const Color colorAz  = (difAz < errorAz_)    ? Color::Green : Color::Red;
    const Color colorDis = (percent < errorDist_) ? Color::Green : Color::Red;

    snprintf(buf_, sizeof(buf_), "Azimuth        %.3f", static_cast<double>(dm_.getAzimuth()));
    gfx_->text8x12(10, 10, buf_, colorAz);
    snprintf(buf_, sizeof(buf_), "Distance       %.1f", static_cast<double>(dm_.getDistance()));
    gfx_->text8x12(10, 20, buf_, colorDis);
    snprintf(buf_, sizeof(buf_), "Confidence     %.2f", static_cast<double>(dm_.getConfidence()));
    gfx_->text8x12(10, 30, buf_, Color::White);

    if (dm_.getSimulation() == 1) {
        snprintf(buf_, sizeof(buf_), "True Azimuth   %.3f", static_cast<double>(trueAz));
        gfx_->text8x12(10, 40, buf_, Color::White);
        snprintf(buf_, sizeof(buf_), "True Distance  %.3f", static_cast<double>(trueDist));
        gfx_->text8x12(10, 50, buf_, Color::White);
        snprintf(buf_, sizeof(buf_), "Dif Azimuth    %.3f", static_cast<double>(difAz));
        gfx_->text8x12(10, 70, buf_, colorAz);
        snprintf(buf_, sizeof(buf_), "Dif Distance   %.3f", static_cast<double>(difDist));
        gfx_->text8x12(10, 80, buf_, colorDis);
    }
}

void LCDTask::showAcousticState()
{
    // 64 Bänder als Balken: x = 240..(240+64*3), Höhe bis 100 px
    AcousticState s;
    dm_.getAcousticState(s);
    constexpr int baseY = 110, maxH = 100, x0 = 240, w = 3;
    gfx_->line(x0, baseY, x0 + NUM_BANDS * w, baseY, Color::Gray);
    for (uint32_t b = 0; b < NUM_BANDS; ++b) {
        const int h = static_cast<int>(s.p[b] * maxH);
        const Color c = (s.p[b] > THETA_SEL) ? Color::Green : Color::DarkGray;
        gfx_->line(x0 + b * w, baseY, x0 + b * w, baseY - h, c);
    }
    snprintf(buf_, sizeof(buf_), "s(t) frame %lu", static_cast<unsigned long>(s.frame_id));
    gfx_->text8x12(240, 115, buf_, Color::White);

    gfx_->text8x12(10, 50, dm_.getMlInitError() ? "ML Init Error" : "ML Init OK",
                   dm_.getMlInitError() ? Color::Red : Color::Green);
    gfx_->text8x12(10, 60, dm_.getMlRunError() ? "ML Run Error" : "ML Run OK",
                   dm_.getMlRunError() ? Color::Red : Color::Green);
}

void LCDTask::showError()
{
    SDS_ErrorMessage msg;
    if (dm_.popErrorMessage(msg))
        gfx_->text8x12(10, 80, msg.text, Color::White);

    if (dm_.getErrorFlag()) {
        const uint32_t count = dm_.getErrorCount();
        if (count > 0) {
            dm_.setErrorCount(count - 1);
            uint8_t e[16] = {};
            dm_.getErrorBuffer(e, sizeof(e));
            snprintf(buf_, sizeof(buf_), "%x %x %x %x   %x %x %x %x   %x %x %x %x ",
                     e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7], e[8], e[9], e[10], e[11]);
            gfx_->text8x12(10, 250, buf_, Color::White);
        } else {
            dm_.setErrorFlag(0);
        }
    }
}

} // namespace sds110
