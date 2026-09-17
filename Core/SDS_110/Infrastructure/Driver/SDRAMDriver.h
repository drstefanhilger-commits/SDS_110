/*
 * SDRAMDriver.h
 *
 *  Created on: Aug 13, 2026
 *      Author: 310004
 */

#pragma once

#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

void SDRAM_InitSequence(SDRAM_HandleTypeDef *hsdram)
{
    FMC_SDRAM_CommandTypeDef Command;

    /* Step 1: Clock enable command */
    Command.CommandMode            = FMC_SDRAM_CMD_CLK_ENABLE;
    Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
    Command.AutoRefreshNumber      = 1;
    Command.ModeRegisterDefinition = 0;
    HAL_SDRAM_SendCommand(hsdram, &Command, 0x1000);
    HAL_Delay(1);

    /* Step 2: Precharge All command */
    Command.CommandMode = FMC_SDRAM_CMD_PALL;
    HAL_SDRAM_SendCommand(hsdram, &Command, 0x1000);

    /* Step 3: Auto-refresh command */
    Command.CommandMode       = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
    Command.AutoRefreshNumber = 8;
    HAL_SDRAM_SendCommand(hsdram, &Command, 0x1000);

    /* Step 4: Load Mode Register */
    uint32_t mode =
          SDRAM_MODEREG_BURST_LENGTH_1
        | SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL
        | SDRAM_MODEREG_CAS_LATENCY_3
        | SDRAM_MODEREG_OPERATING_MODE_STANDARD
        | SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

    Command.CommandMode            = FMC_SDRAM_CMD_LOAD_MODE;
    Command.ModeRegisterDefinition = mode;
    HAL_SDRAM_SendCommand(hsdram, &Command, 0x1000);

    /* Step 5: Set the refresh rate counter */
    HAL_SDRAM_ProgramRefreshRate(hsdram, 1386);
}
