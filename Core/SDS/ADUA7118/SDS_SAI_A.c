/*
 * SDS_SAI_A.c
 *
 *  Created on: Aug 30, 2026
 *      Author: 310004
 */

#include "SDS_SAI_A.h"

uint32_t g_saiA_dmaBuffer[SDS_SAI_A_DMA_BUFFER_WORDS];
static SDS_RingBuffer_t g_saiA_ringBuffer;

void SDS_SAI_A_Init(void)
{
    SDS_RingBuffer_Init(&g_saiA_ringBuffer);
}

void SDS_SAI_A_Start(void)
{
    if (HAL_SAI_Receive_DMA(&hsai_BlockA1,
                            (uint8_t *)g_saiA_dmaBuffer,
                            SDS_SAI_A_DMA_BUFFER_WORDS) != HAL_OK) {
//        Error_Handler();
    }
}

void SDS_SAI_A_Stop(void)
{
    HAL_SAI_DMAStop(&hsai_BlockA1);
}

void SDS_SAI_A_OnRxHalfComplete(void)
{
    TDM_Frame_t frame;
    TDM_ParseFrame(&g_saiA_dmaBuffer[0], &frame);
    SDS_RingBuffer_PushFrame(&g_saiA_ringBuffer, &frame);
}

void SDS_SAI_A_OnRxComplete(void)
{
    TDM_Frame_t frame;
    TDM_ParseFrame(&g_saiA_dmaBuffer[SDS_SAI_A_FRAME_WORDS], &frame);
    SDS_RingBuffer_PushFrame(&g_saiA_ringBuffer, &frame);
}

void SDS_SAI_A_OnError(void)
{
    // Optional: Error logging
}
