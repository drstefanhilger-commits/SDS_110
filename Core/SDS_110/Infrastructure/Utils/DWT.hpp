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
    float cyclesToUs(uint32_t c) const { return static_cast<float>(c) / cpuMHz; }
    float cyclesToNs(uint32_t c) const { return (static_cast<float>(c) * 1000.0f) / cpuMHz; }
    void setCpuMHz(float mhz) { cpuMHz = mhz; }

private:
    float cpuMHz = 216.0f;
    DWTTimer()
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
};

} // namespace sds110
