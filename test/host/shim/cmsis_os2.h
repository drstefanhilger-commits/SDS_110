// Host-Shim: CMSIS-RTOS2-Stubs (einfädig)
#pragma once
#include <cstdint>
typedef void* osMutexId_t;
struct osMutexAttr_t { const char* name; uint32_t attr_bits; void* cb_mem; uint32_t cb_size; };
typedef int osStatus_t;
constexpr osStatus_t osOK = 0;
constexpr uint32_t osWaitForever = 0xFFFFFFFFu;
inline osMutexId_t osMutexNew(const osMutexAttr_t*) { static int m; return &m; }
inline osStatus_t osMutexAcquire(osMutexId_t, uint32_t) { return osOK; }
inline osStatus_t osMutexRelease(osMutexId_t) { return osOK; }
inline uint32_t osKernelGetTickCount() { return 0; }
typedef void* osMessageQueueId_t;
struct osMessageQueueAttr_t { const char* name; uint32_t attr_bits; void* cb_mem; uint32_t cb_size; void* mq_mem; uint32_t mq_size; };
