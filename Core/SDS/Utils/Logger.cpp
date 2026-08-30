/*
 * Logger.cpp
 *
 *  Created on: Aug 30, 2026
 *      Author: 310004
 */

#include "Logger.hpp"

Logger& Logger::instance()
{
    static Logger inst;
    return inst;
}

Logger::Logger()
{
    head = 0;
    tail = 0;
}

void Logger::write(const char* fmt, ...)
{
    char temp[256];

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(temp, sizeof(temp), fmt, args);
    va_end(args);

    if (len <= 0) return;
    if (len > 256) len = 256;

    uint32_t h = head.load(std::memory_order_relaxed);

    for (int i = 0; i < len; i++)
        buffer[(h + i) % LOG_BUFFER_SIZE] = temp[i];

    head.store((h + len) % LOG_BUFFER_SIZE, std::memory_order_release);
}

int Logger::read(uint8_t* dst, int maxLen)
{
    uint32_t t = tail.load(std::memory_order_relaxed);
    uint32_t h = head.load(std::memory_order_acquire);

    if (t == h)
        return 0;

    int count = 0;

    while (t != h && count < maxLen)
    {
        dst[count++] = buffer[t];
        t = (t + 1) % LOG_BUFFER_SIZE;
    }

    tail.store(t, std::memory_order_release);
    return count;
}

