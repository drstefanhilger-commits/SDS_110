/*
 * sai.c
 *
 *  Created on: Aug 30, 2026
 *      Author: 310004
 */


#include "sai.h"

SAI_HandleTypeDef hsai_BlockA1;

static void MX_SAI1_MspInit(SAI_HandleTypeDef *hsai)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_SAI1_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    // PE4: FS, PE5: SCK, PE6: SD
    GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SAI1;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}

void MX_SAI1_Init(void)
{
    hsai_BlockA1.Instance = SAI1_Block_A;

    hsai_BlockA1.Init.AudioMode      = SAI_MODEMASTER_RX;
    hsai_BlockA1.Init.Synchro        = SAI_ASYNCHRONOUS;
    hsai_BlockA1.Init.OutputDrive    = SAI_OUTPUTDRIVE_DISABLE;
    hsai_BlockA1.Init.NoDivider      = SAI_MASTERDIVIDER_ENABLE;
    hsai_BlockA1.Init.FIFOThreshold  = SAI_FIFOTHRESHOLD_1QF;
    hsai_BlockA1.Init.Protocol       = SAI_PCM_LONG;
    hsai_BlockA1.Init.DataSize       = SAI_PROTOCOL_DATASIZE_32BIT;
    hsai_BlockA1.Init.FirstBit       = SAI_FIRSTBIT_MSB;
    hsai_BlockA1.Init.ClockStrobing  = SAI_CLOCKSTROBING_FALLINGEDGE;

    hsai_BlockA1.FrameInit.FrameLength       = 256;
    hsai_BlockA1.FrameInit.ActiveFrameLength = 32;
    hsai_BlockA1.FrameInit.FSDefinition      = SAI_FS_STARTFRAME;
    hsai_BlockA1.FrameInit.FSPolarity        = SAI_FS_ACTIVE_HIGH;
    hsai_BlockA1.FrameInit.FSOffset          = SAI_FS_FIRSTBIT;

    hsai_BlockA1.SlotInit.FirstBitOffset = 0;
    hsai_BlockA1.SlotInit.SlotSize       = SAI_SLOTSIZE_32B;
    hsai_BlockA1.SlotInit.SlotNumber     = 8;
    hsai_BlockA1.SlotInit.SlotActive     = 0xFF;

    MX_SAI1_MspInit(&hsai_BlockA1);

    if (HAL_SAI_Init(&hsai_BlockA1) != HAL_OK) {
        //Error_Handler(); 		// Change existing MX_SAI1_Init
	}
}
