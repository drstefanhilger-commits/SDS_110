#pragma once
#include <functional>
#include "stm32f7xx_hal.h"

class HardwareTimer
{
public:
    HardwareTimer(TIM_TypeDef* tim, IRQn_Type irq);

    void init(uint32_t timerClockHz, float rateHz);
    void start();
    void stop();

    void setCallback(std::function<void()> cb);

    void handleInterrupt();
    float getRateHz() { return ( rateHz_); };

private:
    float rateHz_;
    TIM_TypeDef* tim;
    IRQn_Type irq;
    std::function<void()> callback;
};
