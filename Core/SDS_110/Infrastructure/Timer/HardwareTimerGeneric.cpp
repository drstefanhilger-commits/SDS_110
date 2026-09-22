/*
 * HardwareTimerGeneric.cpp
 *
 *  Created on: Sep 22, 2026
 *      Author: 310004
 */

#include "HardwareTimerGeneric.hpp"

std::vector<HardwareTimerGeneric*> HardwareTimerGeneric::allTimers;

// Automatische Liste aller Timer-Basen
static TIM_TypeDef* const allTimBases[] =
{
    TIM1, TIM2, TIM3, TIM4, TIM5, TIM6, TIM7
};

HardwareTimerGeneric::HardwareTimerGeneric(TIM_TypeDef* t, IRQn_Type i)
    : tim(t), irq(i)
{
    allTimers.push_back(this);
}

void HardwareTimerGeneric::enableClock(TIM_TypeDef* tim)
{
    uint32_t base = reinterpret_cast<uint32_t>(tim);

    if (base == TIM1_BASE) __HAL_RCC_TIM1_CLK_ENABLE();
    else if (base == TIM2_BASE) __HAL_RCC_TIM2_CLK_ENABLE();
    else if (base == TIM3_BASE) __HAL_RCC_TIM3_CLK_ENABLE();
    else if (base == TIM4_BASE) __HAL_RCC_TIM4_CLK_ENABLE();
    else if (base == TIM5_BASE) __HAL_RCC_TIM5_CLK_ENABLE();
    else if (base == TIM6_BASE) __HAL_RCC_TIM6_CLK_ENABLE();
    else if (base == TIM7_BASE) __HAL_RCC_TIM7_CLK_ENABLE();
}

void HardwareTimerGeneric::init(uint32_t timerClockHz, float rateHz)
{
    enableClock(tim);

    uint32_t arr = static_cast<uint32_t>(timerClockHz / rateHz);

    tim->PSC = 0;
    tim->ARR = arr - 1;
    tim->DIER |= TIM_DIER_UIE;

    NVIC_SetPriority(irq, 5);
    NVIC_EnableIRQ(irq);
}

void HardwareTimerGeneric::start()
{
    tim->CR1 |= TIM_CR1_CEN;
}

void HardwareTimerGeneric::stop()
{
    tim->CR1 &= ~TIM_CR1_CEN;
}

void HardwareTimerGeneric::setCallback(std::function<void()> cb)
{
    callback = cb;
}

void HardwareTimerGeneric::handleInterrupt()
{
    if (callback)
        callback();

    tim->SR &= ~TIM_SR_UIF;
}

void HardwareTimerGeneric::dispatchAll()
{
    for (auto* timer : allTimers)
    {
        if (timer->tim->SR & TIM_SR_UIF)
        {
            timer->handleInterrupt();
        }
    }
}
