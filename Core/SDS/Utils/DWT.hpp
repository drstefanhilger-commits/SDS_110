/*
 * DWT.hpp
 *
 *  Created on: Sep 9, 2026
 *      Author: 310004
 */

#pragma once
//#include "core_cm7.h"
#include "stm32f7xx_hal.h"

class DWTTimer {
public:
    static DWTTimer& instance()
    {
        static DWTTimer inst;
        return inst;
    }

    inline uint32_t cycles() const
    {
        return DWT->CYCCNT;
    }

    inline void reset()
    {
        DWT->CYCCNT = 0;
    }

private:
    DWTTimer()
    {
		CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;        // enable trace unit
		*((volatile uint32_t*)0xE0001FB0) = 0xC5ACCE55;        // unlock DWT
		DWT->CYCCNT = 0;                                       // reset counter
		DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;                   // enable counter
    }
};
