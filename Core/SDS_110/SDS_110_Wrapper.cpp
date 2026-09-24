/*
 * SDS_110_Wrapper.cpp – C-Einstiegspunkte für main.c.
 */
#include "SDS_110_Wrapper.hpp"
#include "Infrastructure/Tasks/USBTask.hpp"
#include "Infrastructure/Tasks/LCDTask.hpp"
#include "Infrastructure/Tasks/LoggerTask.hpp"
#include "Infrastructure/Tasks/ProcessingTask.hpp"
#include "Infrastructure/Model/SDS_Data.hpp"


extern SAI_HandleTypeDef hsai_BlockA2;     // CubeMX, main.c
extern I2C_HandleTypeDef hi2c1;

extern "C" {

void SDS110_Init(void)
{
    SDS_Data& dm = SDS_Data::instance();
    // Standard-Unit-ID: 96-Bit-UID des STM32 auf 16 Bit gefaltet (per USB-Kommando Typ 5 überschreibbar)
    const uint32_t uid = HAL_GetUIDw0() ^ HAL_GetUIDw1() ^ HAL_GetUIDw2();
    dm.setId(static_cast<uint16_t>((uid ^ (uid >> 16)) & 0xFFFF));
    sds110::Processing_Module_120::instance().init(&hsai_BlockA2, &hi2c1);
}

void SDS110_StartProcessingTask(void) { sds110::ProcessingTask::instance().start(); }
void SDS110_StartDisplayTask(void)    { sds110::LCDTask::instance().start(); }
void SDS110_StartUSBTask(void)        { sds110::USBTask::instance().start(); }
void SDS110_StartLoggerTask(void)     { sds110::LoggerTask::instance().start(); }

} // extern "C"
