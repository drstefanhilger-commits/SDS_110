/*
 * SDS_SAI_A.h
 *
 *  Created on: Aug 30, 2026
 *      Author: 310004
 */

#ifndef SDS_ADUA7118_SDS_SAI_A_H_
#define SDS_ADUA7118_SDS_SAI_A_H_

#include "stm32f7xx_hal.h"
#include "sai.h"
#include "SDS_RingBuffer.h"
#include "TDM_Parser.h"

#define SDS_SAI_A_NUM_SLOTS      8
#define SDS_SAI_A_FRAME_WORDS    SDS_SAI_A_NUM_SLOTS
#define SDS_SAI_A_DMA_BUFFER_WORDS  (SDS_SAI_A_FRAME_WORDS * 2)

extern uint32_t g_saiA_dmaBuffer[SDS_SAI_A_DMA_BUFFER_WORDS];

void SDS_SAI_A_Init(void);
void SDS_SAI_A_Start(void);
void SDS_SAI_A_Stop(void);

void SDS_SAI_A_OnRxHalfComplete(void);
void SDS_SAI_A_OnRxComplete(void);
void SDS_SAI_A_OnError(void);

#endif /* SDS_ADUA7118_SDS_SAI_A_H_ */
