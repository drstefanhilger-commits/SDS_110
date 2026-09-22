/*
 * HardwareTimerGeneric.hpp
 *
 *  Created on: Sep 22, 2026
 *      Author: 310004
 */

#pragma once
#include <functional>
#include <vector>
#include "stm32f7xx_hal.h"

class HardwareTimerGeneric
{
public:
    HardwareTimerGeneric(TIM_TypeDef* tim, IRQn_Type irq);

    void init(uint32_t timerClockHz, float rateHz);
    void start();
    void stop();

    void setCallback(std::function<void()> cb);
    void handleInterrupt();

    // Eine einzige ISR ruft diese Funktion auf
    static void dispatchAll();

private:
    TIM_TypeDef* tim;
    IRQn_Type irq;
    std::function<void()> callback;

    static std::vector<HardwareTimerGeneric*> allTimers;

    static void enableClock(TIM_TypeDef* tim);
};
