/*
 * SDS_110_Wrapper.cpp
 * Übergangsstand: USB-Task läuft, Display/Logger/Processing folgen mit ihrer Migration.
 */
#include "SDS_110_Wrapper.hpp"
#include "Infrastructure/Tasks/USBTask.hpp"

extern "C" {

void SDS110_Init(void)
{
    // TODO: Sensor_Unit_112::init(&hsai, &hi2c), Processing_Module_120::init()
}

void SDS110_StartProcessingTask(void)
{
    // TODO: Task um Processing_Module_120 (folgt mit 126/128/130)
}

void SDS110_StartDisplayTask(void)
{
    // TODO: LCDTask migrieren
}

void SDS110_StartUSBTask(void)
{
    sds110::USBTask::instance().start();
}

void SDS110_StartLoggerTask(void)
{
    // TODO: LoggerTask migrieren
}

} // extern "C"
