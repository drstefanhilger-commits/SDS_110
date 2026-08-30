/*
 * Logger.hpp
 *
 *  Created on: Aug 30, 2026
 *      Author: 310004
 */

#pragma once
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <atomic>
#include "cmsis_os2.h"

class Logger
{
public:
    static constexpr uint32_t LOG_BUFFER_SIZE = 4096;

    static Logger& instance();

    void write(const char* fmt, ...);
    int read(uint8_t* dst, int maxLen);

private:
    Logger();
    Logger(const Logger&) = delete;

private:
    uint8_t buffer[LOG_BUFFER_SIZE];
    std::atomic<uint32_t> head;
    std::atomic<uint32_t> tail;
};
