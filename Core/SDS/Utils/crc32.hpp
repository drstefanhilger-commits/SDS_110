/*
 * crc32.hpp
 *
 * Deterministic STM32-compatible CRC32 (reflected polynomial 0xEDB88320)
 * Bit-identical to Python SDSParser.crc32_stm32()
 *
 * Created on: Aug 31, 2026
 * Author: Stefan (310004)
 */

#ifndef SDS_UTILS_CRC32_HPP_
#define SDS_UTILS_CRC32_HPP_

#include <cstdint>
#include <cstddef>

class CRC32 {
public:

    // ------------------------------------------------------------
    // Deterministic CRC32 (reflected)
    // ------------------------------------------------------------
    static uint32_t computeCRC32(const uint8_t* data, size_t len)
    {
        // reflected polynomial (same as Python)
        constexpr uint32_t poly = 0xEDB88320;

        uint32_t crc = 0xFFFFFFFF;

        for (size_t i = 0; i < len; i++) {
            crc ^= data[i];
            for (int bit = 0; bit < 8; bit++) {
                if (crc & 1) {
                    crc = (crc >> 1) ^ poly;
                } else {
                    crc >>= 1;
                }
            }
        }

        return crc ^ 0xFFFFFFFF;
    }

    // ------------------------------------------------------------
    // Convenience wrapper for structs
    // ------------------------------------------------------------
    template<typename T>
    static uint32_t compute(const T& msg)
    {
        return computeCRC32(reinterpret_cast<const uint8_t*>(&msg),
                            sizeof(T));
    }

    template<typename T>
    static uint32_t compute_no_crc(const T& msg)
    {
        return computeCRC32(reinterpret_cast<const uint8_t*>(&msg),
                            sizeof(T) - sizeof(uint32_t));
    }
};

#endif /* SDS_UTILS_CRC32_HPP_ */
