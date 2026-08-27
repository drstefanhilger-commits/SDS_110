/*
 * adau7118.c
 *
 * ADAU7118 8‑Channel PDM‑to‑TDM Microphone Converter Driver
 * - I2C config
 * - I2S2 RX DMA (TDM‑8)
 * - Demux to per‑channel samples
 * - Hand‑off to SDS_MicrophoneBuffer via ADAU7118_OnSample()
 */

#include "adau7118.h"

// Local pointers to shared RX buffer (owned by SDS_MicrophoneBuffer)
static int32_t* adau7118_rxBuffer = NULL;
static uint32_t adau7118_rxSize   = 0;

// -------- I2C helpers --------
static HAL_StatusTypeDef ADAU7118_WriteReg(uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(&hi2c1,
                             ADAU7118_I2C_ADDR,
                             reg,
                             I2C_MEMADD_SIZE_8BIT,
                             &value,
                             1,
                             100);
}

static HAL_StatusTypeDef ADAU7118_WriteRegs(const uint8_t *data, uint16_t length)
{
    for (uint16_t i = 0; i + 1 < length; i += 2) {
        HAL_StatusTypeDef st = ADAU7118_WriteReg(data[i], data[i + 1]);
        if (st != HAL_OK) {
            return st;
        }
    }
    return HAL_OK;
}

// -------- ADAU7118 register configuration (placeholder) --------
static HAL_StatusTypeDef ADAU7118_Configure(void)
{
    const uint8_t initSeq[] = {
        // reg, val (mit Datenblatt abgleichen)
        0x00, 0x01,   // Power‑up
        0x01, 0x10,   // Decimation / sample rate
        0x02, 0xB4,   // PDM clock map
        0x03, 0xFF,   // enable 8 channels
        0x04, 0x02,   // TDM‑8, 32‑bit slots
        0x05, 0x01    // HPF enable
    };

    return ADAU7118_WriteRegs(initSeq, sizeof(initSeq));
}

// -------- I2S2 init (TDM‑8, 48 kHz) --------
static void ADAU7118_I2S2_Init(void)
{
    hi2s2.Instance         = SPI2;
    hi2s2.Init.Mode        = I2S_MODE_SLAVE_RX;
    hi2s2.Init.Standard    = I2S_STANDARD_PHILIPS;
    hi2s2.Init.DataFormat  = I2S_DATAFORMAT_32B;
    hi2s2.Init.MCLKOutput  = I2S_MCLKOUTPUT_ENABLE;
    hi2s2.Init.AudioFreq   = I2S_AUDIOFREQ_48K;
    hi2s2.Init.CPOL        = I2S_CPOL_LOW;
    hi2s2.Init.ClockSource = I2S_CLOCK_PLL;

    HAL_I2S_Init(&hi2s2);
}

// -------- Public init --------
void ADAU7118_Init(void)
{
    // Get shared DMA buffer from SDS_MicrophoneBuffer (C++)
    adau7118_rxBuffer = SDS_GetRxBuffer();
    adau7118_rxSize   = SDS_GetRxBufferSize();

    (void)ADAU7118_Configure();
    ADAU7118_I2S2_Init();
}

// -------- Start / stop --------
void ADAU7118_Start(void)
{
    if (adau7118_rxBuffer == NULL || adau7118_rxSize == 0U) {
        return;
    }

    (void)HAL_I2S_Receive_DMA(&hi2s2,
                              (uint16_t*)adau7118_rxBuffer,
                              adau7118_rxSize);
}

void ADAU7118_Stop(void)
{
    (void)HAL_I2S_DMAStop(&hi2s2);
}

// -------- Demux TDM buffer --------
void ADAU7118_ProcessRxBuffer(int32_t *buffer, uint32_t samplesPerChannel)
{
    int numMics = SDS_GetNumMics();

    for (uint32_t s = 0; s < samplesPerChannel; ++s) {
        for (int ch = 0; ch < numMics; ++ch) {
            uint32_t idx = s * (uint32_t)numMics + (uint32_t)ch;
            int32_t pcm24 = buffer[idx];
            ADAU7118_OnSample((uint8_t)ch, pcm24);
        }
    }
}

// -------- HAL callback wrappers --------
void ADAU7118_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI2 && adau7118_rxBuffer != NULL) {
        uint32_t halfWords = adau7118_rxSize / 2U;
        uint32_t samplesPerChannel = halfWords / (uint32_t)SDS_GetNumMics();
        ADAU7118_ProcessRxBuffer(adau7118_rxBuffer, samplesPerChannel);
    }
}

void ADAU7118_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI2 && adau7118_rxBuffer != NULL) {
        uint32_t halfWords = adau7118_rxSize / 2U;
        uint32_t samplesPerChannel = halfWords / (uint32_t)SDS_GetNumMics();
        ADAU7118_ProcessRxBuffer(&adau7118_rxBuffer[halfWords], samplesPerChannel);
    }
}

// -------- HAL global hooks --------
void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    ADAU7118_I2S_RxHalfCpltCallback(hi2s);
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    ADAU7118_I2S_RxCpltCallback(hi2s);
}
