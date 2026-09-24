/*
 * LCDTask.hpp  (Infrastructure/Tasks)
 *
 * Rendert den Zustand aus SDS_Data auf das 480x272-LCD (Polar-Plot,
 * Systemdaten, Fehler). Im READ-Modus statt der vier AI-Klassen jetzt
 * die 64 Bandwahrscheinlichkeiten s(t) als Balkenspektrum.
 *
 * Getaktet durch TIM4 über TaskTimerBase (20 Hz), vorher TaskBase mit osDelay(50).
 *
 * Migration aus SDS/Tasks/LCDTask:
 *  - neue SDS_Data-API (getTaskStats, getAcousticState, setCandidate-Felder)
 *  - usb_debug_counter liegt jetzt in USBTask.cpp
 *  - Algorithm.hpp entfällt (keine SRP-Abhängigkeit mehr)
 */
#pragma once
#include "Infrastructure/Timer/TaskTimerBase.hpp"
#include "Infrastructure/Model/Model.hpp"
#include "Infrastructure/Driver/LCDDriver.hpp"

extern "C" volatile uint32_t usb_debug_counter;

namespace sds110 {

class LCDTask : public TaskTimerBase {
public:
    static LCDTask& instance() { static LCDTask inst; return inst; }

    static constexpr float kRateHz = 20.0f;   // 50 ms, wie bisher osDelay(50)

protected:
    void onTask() override;

private:
    LCDTask();
    LCDTask(const LCDTask&) = delete;
    LCDTask& operator=(const LCDTask&) = delete;
    void showDetection();
    void showAcousticState();
    void showRadar();
    void showSystemData();
    void showError();
    void taskLine(int y, const char* name, TaskId id);

    SDS_Data&  dm_   = SDS_Data::instance();
    LCDDriver* gfx_  = &LCDDriver::instance();

    static constexpr float errorAz_   = 3.0f;    // ±3°
    static constexpr float errorDist_ = 15.0f;   // ±15 %
    static constexpr int   x0_ = 359, y0_ = 136; // Polar-Zentrum
    static constexpr float R_  = 120.0f;         // max. Radius px
    static constexpr float distFac_ = 120.0f / 100.0f; // px pro m (100 m Vollausschlag)
    static constexpr float deg2rad_ = 3.14159265f / 180.0f;
    char buf_[128];
};

} // namespace sds110
