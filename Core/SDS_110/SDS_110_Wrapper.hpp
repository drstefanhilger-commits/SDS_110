/*
 * SDS_110_Wrapper.hpp
 *
 * C-Schnittstelle für main.c (Ersatz für SDS_Wrapper.hpp).
 * Zieht wie bisher die Header-only-Treiber ein, die main.c direkt aufruft:
 *   SDRAMDriver.h  -> SDRAM-Init
 *   PrintfDriver.h -> ITM/SWO für printf
 *   MPUDriver.h    -> SDS110_MPU_Config() – in main.c, USER CODE BEGIN 1 aufrufen
 * Diese liegen bis zur Migration weiter in Core/SDS/Driver (Include-Pfad
 * beibehalten) und ziehen später nach Infrastructure/Driver um.
 *
 * Übergangsweise werden die alten Funktionsnamen (SDS_Init, SDS_Start*)
 * auf die neuen (SDS110_*) gemappt, damit main.c nicht angefasst werden muss.
 */
#pragma once

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

// --- Neue API -----------------------------------------------------------
void SDS110_Init(void);                  // Sensor Unit 112 + Processing Module 120
void SDS110_StartProcessingTask(void);   // Task um Processing_Module_120
void SDS110_StartDisplayTask(void);
void SDS110_StartUSBTask(void);
void SDS110_StartLoggerTask(void);


#ifdef __cplusplus
}
#endif
