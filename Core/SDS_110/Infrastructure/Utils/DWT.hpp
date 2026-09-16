/*
 * DWT.hpp  (Infrastructure/Utils) – unverändert aus SDS/Utils/DWT.hpp, Namespace sds110
 */
#pragma once
#include "stm32f7xx_hal.h"
#include <cstdint>

namespace sds110 {

class DWTTimer {
public:
    static DWTTimer& instance() { static DWTTimer inst; return inst; }
    uint32_t cycles() const { return DWT->CYCCNT; }
    void reset() { DWT->CYCCNT = 0; }
private:
    DWTTimer()
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
};

} // namespace sds110
