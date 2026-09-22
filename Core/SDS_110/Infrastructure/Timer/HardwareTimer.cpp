#include "HardwareTimer.hpp"

HardwareTimer::HardwareTimer(TIM_TypeDef* t, IRQn_Type i)
    : tim(t), irq(i)
{
}

void HardwareTimer::init(uint32_t timerClockHz, float rateHz)
{
	rateHz_ = rateHz;

    // Clock enable
    if (tim == TIM2) __HAL_RCC_TIM2_CLK_ENABLE();
    else if (tim == TIM3) __HAL_RCC_TIM3_CLK_ENABLE();
    else if (tim == TIM4) __HAL_RCC_TIM4_CLK_ENABLE();
    else if (tim == TIM5) __HAL_RCC_TIM5_CLK_ENABLE();
    else if (tim == TIM6) __HAL_RCC_TIM6_CLK_ENABLE();
    else if (tim == TIM7) __HAL_RCC_TIM7_CLK_ENABLE();
    else if (tim == TIM8) __HAL_RCC_TIM8_CLK_ENABLE();

    // ARR berechnen
    uint32_t arr = static_cast<uint32_t>(timerClockHz / rateHz);

    tim->PSC = 0;
    tim->ARR = arr - 1;

    // Update Interrupt aktivieren
    tim->DIER |= TIM_DIER_UIE;

    // NVIC konfigurieren
    NVIC_SetPriority(irq, 5);
    NVIC_EnableIRQ(irq);
}

void HardwareTimer::start()
{
    tim->CR1 |= TIM_CR1_CEN;
}

void HardwareTimer::stop()
{
    tim->CR1 &= ~TIM_CR1_CEN;
}

void HardwareTimer::setCallback(std::function<void()> cb)
{
    callback = cb;
}

void HardwareTimer::handleInterrupt()
{
    if (callback)
        callback();

    // UIF löschen
    tim->SR &= ~TIM_SR_UIF;
}
