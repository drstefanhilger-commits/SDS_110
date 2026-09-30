/*
 * SDS_110_Wrapper.hpp
 *
 * C-Schnittstelle für main.c (STM32F746G-Discovery, Branch ohne LCD).
 * Zieht die Header-only-Treiber ein, die main.c direkt aufruft:
 *   SDRAMDriver.h  -> SDRAM_InitSequence() (USER CODE FMC_Init 2; SDRAM nur noch für den
 *                     LTDC-Framebuffer der CubeMX-Konfiguration, SDS-Puffer liegen intern)
 *   PrintfDriver.h -> ITM/SWO für printf (USER CODE BEGIN SysInit)
 *   MPUDriver.h    -> SDS110_MPU_Config() (USER CODE BEGIN 1, vor Cache-Enable und HAL_Init)
 *
 * Gegenüber dem bisherigen Stand entfallen LCDTask, LCDDriver, Font und SDRAM-Selbsttest.
 */
#pragma once
#include "SDS_110_Board.h"             // SDS110_SAI_ENABLED (auch für main.c)

// Nur für main.c (C): Header-only-Treiber mit nicht-inline Definitionen.
// Aus C++-Dateien NICHT einziehen, sonst doppelte Definition beim Linken.
#ifndef __cplusplus
#include "Infrastructure/Driver/SDRAMDriver.h"
#include "Infrastructure/Driver/PrintfDriver.h"
#include "Infrastructure/Driver/MPUDriver.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void SDS110_Init(void);                  // Sensor Unit 112 + Processing Module 120, LCD aus
void SDS110_StartProcessingTask(void);   // Task um Processing_Module_120
void SDS110_StartUSBTask(void);
void SDS110_StartLoggerTask(void);       // Logger (USB Id 99) + LED1-Herzschlag

#ifdef __cplusplus
}
#endif
