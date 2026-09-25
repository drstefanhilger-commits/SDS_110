/*
 * SDRAMSelfTest.cpp  (Infrastructure/Driver)
 */
#include "SDRAMSelfTest.hpp"

namespace sds110 {

static uint32_t pattern(uint32_t offset) { return 0xA5A5F00Fu ^ (offset * 0x9E3779B1u); }

// Prüfadressen: 0, 1, 2, 4, ... 2^k < n, sowie n-1
template <typename F>
static void forEachOffset(uint32_t n, F f)
{
    f(0u);
    for (uint32_t off = 1; off < n; off <<= 1) f(off);
    if (n > 1) f(n - 1);
}

static bool pass(volatile uint32_t* p, uint32_t n, uint32_t invert)
{
    forEachOffset(n, [&](uint32_t off) { p[off] = pattern(off) ^ invert; });
    SCB_CleanInvalidateDCache();                      // zurück ins SDRAM, Cache verwerfen
    bool ok = true;
    forEachOffset(n, [&](uint32_t off) { if (p[off] != (pattern(off) ^ invert)) ok = false; });
    return ok;
}

bool sdramSelfTest(const SDRAM_HandleTypeDef& hsdram, uint32_t* begin, uint32_t* end)
{
    if (hsdram.State != HAL_SDRAM_STATE_READY) return false;
    if (end <= begin) return true;                    // nichts im SDRAM abgelegt
    volatile uint32_t* p = begin;
    const uint32_t n = static_cast<uint32_t>(end - begin);
    return pass(p, n, 0u) && pass(p, n, 0xFFFFFFFFu);
}

} // namespace sds110
