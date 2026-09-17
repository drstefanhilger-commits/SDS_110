/*
 * MPUDriver.h  (Infrastructure/Driver)
 *
 * MPU-Konfiguration für SDS_110. Aufruf aus main.c, USER CODE BEGIN 1
 * (vor SCB_EnableICache/DCache und HAL_Init):
 *
 *     SDS110_MPU_Config();
 *
 * Die CubeMX-Funktion MPU_Config() bleibt unbenutzt (leer oder auskommentiert).
 * Nur aus main.c (C) einbinden – Header-only-Definition.
 *
 * Regionen (höhere Nummer gewinnt bei Überlappung):
 *   0  AXI-SRAM 0x20010000, 256 kB  – uncached (DMA-Puffer SAI/USB ohne Cache-Pflege)
 *   1  SDRAM    0xC0000000, 8 MB    – Write-Back-Cache: DSP-Puffer (.sdram_data ab 0xC0200000)
 *   2  SDRAM    0xC0000000, 2 MB    – uncached: LTDC-Framebuffer (überlagert Region 1)
 */
#pragma once
#include "stm32f7xx_hal.h"

static inline void SDS110_MPU_ConfigRegion(uint8_t number, uint32_t base, uint8_t size,
                                           uint8_t cacheable, uint8_t bufferable, uint8_t tex)
{
    MPU_Region_InitTypeDef r = {0};
    r.Enable           = MPU_REGION_ENABLE;
    r.Number           = number;
    r.BaseAddress      = base;
    r.Size             = size;
    r.SubRegionDisable = 0x00;
    r.TypeExtField     = tex;
    r.AccessPermission = MPU_REGION_FULL_ACCESS;
    r.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;
    r.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
    r.IsCacheable      = cacheable;
    r.IsBufferable     = bufferable;
    HAL_MPU_ConfigRegion(&r);
}

static inline void SDS110_MPU_Config(void)
{
    HAL_MPU_Disable();

    /* 0: interner AXI-SRAM uncached (wie bisher; DMA-Ziel von SAI und USB) */
    SDS110_MPU_ConfigRegion(MPU_REGION_NUMBER0, 0x20010000, MPU_REGION_SIZE_256KB,
                            MPU_ACCESS_NOT_CACHEABLE, MPU_ACCESS_NOT_BUFFERABLE, MPU_TEX_LEVEL0);

    /* 1: gesamtes SDRAM Write-Back, Write-Allocate (TEX=1, C=1, B=1) – DSP-Puffer */
    SDS110_MPU_ConfigRegion(MPU_REGION_NUMBER1, 0xC0000000, MPU_REGION_SIZE_8MB,
                            MPU_ACCESS_CACHEABLE, MPU_ACCESS_BUFFERABLE, MPU_TEX_LEVEL1);

    /* 2: erste 2 MB SDRAM uncached – LTDC-Framebuffer (LCDDriver: 2 x 480x272x4 ab 0xC0000000) */
    SDS110_MPU_ConfigRegion(MPU_REGION_NUMBER2, 0xC0000000, MPU_REGION_SIZE_2MB,
                            MPU_ACCESS_NOT_CACHEABLE, MPU_ACCESS_BUFFERABLE, MPU_TEX_LEVEL0);

    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}
