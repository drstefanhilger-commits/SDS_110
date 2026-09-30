/*
 * SDS_110_Wrapper.cpp – C-Einstiegspunkte für main.c (STM32F746G-Discovery, ohne LCD).
 */
#include "SDS_110_Wrapper.hpp"
#include "Infrastructure/Tasks/USBTask.hpp"
#include "Infrastructure/Tasks/LoggerTask.hpp"
#include "Infrastructure/Tasks/ProcessingTask.hpp"
#include "Infrastructure/Model/SDS_Data.hpp"

#include "main.h"                      // LCD_DISP / LCD_BL_CTRL (CubeMX)

// CubeMX, main.c (Discovery): ADAU7118 über SAI2 Block A und I2C1
extern SAI_HandleTypeDef hsai_BlockA2;
extern I2C_HandleTypeDef hi2c1;

extern "C" {

void SDS110_Init(void)
{
    SDS_Data& dm = SDS_Data::instance();
    // Standard-Unit-ID: 96-Bit-UID des STM32 auf 16 Bit gefaltet (per USB-Kommando Typ 5 überschreibbar)
    const uint32_t uid = HAL_GetUIDw0() ^ HAL_GetUIDw1() ^ HAL_GetUIDw2();
    dm.setId(static_cast<uint16_t>((uid ^ (uid >> 16)) & 0xFFFF));

    // Kein LCDTask: Display und Hintergrundbeleuchtung aus (LTDC läuft weiter, zeigt nichts)
    HAL_GPIO_WritePin(LCD_DISP_GPIO_Port, LCD_DISP_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_RESET);

    sds110::Processing_Module_120::instance().init(&hsai_BlockA2, &hi2c1);
}

void SDS110_StartProcessingTask(void) { sds110::ProcessingTask::instance().start(); }
void SDS110_StartUSBTask(void)        { sds110::USBTask::instance().start(); }
void SDS110_StartLoggerTask(void)     { sds110::LoggerTask::instance().start(); }

} // extern "C"
