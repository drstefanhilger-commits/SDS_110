/*
 * adua7118Driver.c
 *
 *  Created on: Aug 30, 2026
 *      Author: 310004
 */


#include "ADAU7118.h"
#include "stm32f7xx_hal.h"

#define REG_POWER            0x00
#define REG_PLL_CTRL         0x01
#define REG_MODE_CTRL        0x02
#define REG_DECIMATOR_CTRL   0x03
#define REG_CHANNEL_ENABLE   0x04
#define REG_LR_SWAP          0x05
#define REG_FORMAT           0x06
#define REG_MISC             0x07

static HAL_StatusTypeDef ADAU7118_WriteReg(I2C_HandleTypeDef *hi2c,
                                           uint8_t reg,
                                           uint8_t value)
{
    return HAL_I2C_Mem_Write(hi2c,
                             ADAU7118_I2C_ADDR,
                             reg,
                             I2C_MEMADD_SIZE_8BIT,
                             &value,
                             1,
                             100);
}

void ADAU7118_Initxxx(I2C_HandleTypeDef *hi2c)
{
    // Enable pin (PE3)
    __HAL_RCC_GPIOE_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_Delay(10);

    // Power-Up
    ADAU7118_WriteReg(hi2c, REG_POWER, 0x01);

    // Slave Mode (PLL off)
    ADAU7118_WriteReg(hi2c, REG_PLL_CTRL, 0x00);

    // TDM Mode, 32-bit Slots
    ADAU7118_WriteReg(hi2c, REG_MODE_CTRL, 0x30);

    // HPF on, Gain = 0 dB
    ADAU7118_WriteReg(hi2c, REG_DECIMATOR_CTRL, 0x00);

    // Enable all 8 channels
    ADAU7118_WriteReg(hi2c, REG_CHANNEL_ENABLE, 0xFF);

    // No LR swap
    ADAU7118_WriteReg(hi2c, REG_LR_SWAP, 0x00);

    // TDM Format
    ADAU7118_WriteReg(hi2c, REG_FORMAT, 0x01);

    // Misc
    ADAU7118_WriteReg(hi2c, REG_MISC, 0x00);
}
