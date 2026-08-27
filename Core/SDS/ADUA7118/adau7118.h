/*
 * adau7118.h
 *
 * ADAU7118 8‑Channel PDM‑to‑TDM Microphone Converter Driver (C)
 */

#pragma once

#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_i2c.h"
#include "stm32f7xx_hal_i2s.h"

#ifdef __cplusplus
extern "C" {
#endif

// I2C address (7‑bit, shifted for HAL)
#define ADAU7118_I2C_ADDR        (0x3A << 1)   // ggf. anpassen

// Handles provided by main.c
extern I2C_HandleTypeDef hi2c1;
extern I2S_HandleTypeDef hi2s2;

// C‑bridge from SDS_MicrophoneBuffer
extern int32_t* SDS_GetRxBuffer(void);
extern uint32_t SDS_GetRxBufferSize(void);
extern int SDS_GetNumMics(void);

// Driver API
void ADAU7118_Init(void);
void ADAU7118_Start(void);
void ADAU7118_Stop(void);

void ADAU7118_ProcessRxBuffer(int32_t *buffer, uint32_t samplesPerChannel);

void ADAU7118_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s);
void ADAU7118_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s);

// Called into C++ side (implemented in SDS_MicrophoneBuffer.cpp)
void ADAU7118_OnSample(uint8_t ch, int32_t pcm24);

#ifdef __cplusplus
}
#endif
