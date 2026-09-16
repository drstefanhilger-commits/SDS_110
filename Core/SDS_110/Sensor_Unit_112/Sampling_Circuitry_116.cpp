/*
 * Sampling_Circuitry_116.cpp
 */
#include "Sampling_Circuitry_116.hpp"
#include "ADAU7118_Registers.hpp"
#include "cmsis_os2.h"

namespace sds110 {

// DMA-Puffer: 32-Byte-Alignment für D-Cache-Invalidierung auf Cortex-M7
alignas(32) int32_t Sampling_Circuitry_116::dmaBuffer_[2 * HALF_WORDS];

Sampling_Circuitry_116& Sampling_Circuitry_116::instance()
{
    static Sampling_Circuitry_116 inst;
    return inst;
}

// ---------------------------------------------------------------- init
bool Sampling_Circuitry_116::init(SAI_HandleTypeDef* hsai, I2C_HandleTypeDef* hi2c)
{
    hsai_ = hsai;
    hi2c_ = hi2c;
    if (!hsai_ || !hi2c_) return false;

    enablePin(true);
    HAL_Delay(10);

    if (!configureCodec()) { ++errors_; return false; }
    if (!configureSai())   { ++errors_; return false; }
    return true;
}

void Sampling_Circuitry_116::enablePin(bool on)
{
    // Enable-Pin PE3 (aus adua7118Driver.c)
    __HAL_RCC_GPIOE_CLK_ENABLE();
    GPIO_InitTypeDef g{};
    g.Pin   = GPIO_PIN_3;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &g);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool Sampling_Circuitry_116::writeReg(uint8_t reg, uint8_t val)
{
    return HAL_I2C_Mem_Write(hi2c_, static_cast<uint16_t>(ADAU7118_I2C_ADDR_7B << 1),
                             reg, I2C_MEMADD_SIZE_8BIT, &val, 1,
                             ADAU7118_I2C_TIMEOUT_MS) == HAL_OK;
}

bool Sampling_Circuitry_116::configureCodec()
{
    for (const auto& rv : adau7118::INIT_SEQUENCE)
        if (!writeReg(rv.reg, rv.val)) return false;
    return true;
}

bool Sampling_Circuitry_116::configureSai()
{
    // Aus sai.c: Master RX, PCM long, TDM-8 x 32 bit (Frame 256 bit)
    SAI_HandleTypeDef& h = *hsai_;
    h.Init.AudioMode      = SAI_MODEMASTER_RX;
    h.Init.Synchro        = SAI_ASYNCHRONOUS;
    h.Init.OutputDrive    = SAI_OUTPUTDRIVE_DISABLE;
    h.Init.NoDivider      = SAI_MASTERDIVIDER_ENABLE;
    h.Init.FIFOThreshold  = SAI_FIFOTHRESHOLD_1QF;
    h.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_48K;
    h.Init.Protocol       = SAI_FREE_PROTOCOL;
    h.Init.DataSize       = SAI_DATASIZE_32;
    h.Init.FirstBit       = SAI_FIRSTBIT_MSB;
    h.Init.ClockStrobing  = SAI_CLOCKSTROBING_FALLINGEDGE;
    h.Init.MonoStereoMode = SAI_STEREOMODE;
    h.Init.CompandingMode = SAI_NOCOMPANDING;

    h.FrameInit.FrameLength       = 32 * NUM_MICS;   // 256
    h.FrameInit.ActiveFrameLength = 32;
    h.FrameInit.FSDefinition      = SAI_FS_STARTFRAME;
    h.FrameInit.FSPolarity        = SAI_FS_ACTIVE_HIGH;
    h.FrameInit.FSOffset          = SAI_FS_FIRSTBIT;

    h.SlotInit.FirstBitOffset = 0;
    h.SlotInit.SlotSize       = SAI_SLOTSIZE_32B;
    h.SlotInit.SlotNumber     = NUM_MICS;
    h.SlotInit.SlotActive     = 0xFF;

    // GPIO/Clock-MSP kommt aus HAL_SAI_MspInit (CubeMX, stm32f7xx_hal_msp.c)
    return HAL_SAI_Init(&h) == HAL_OK;
}

// ---------------------------------------------------------------- run
bool Sampling_Circuitry_116::start()
{
    if (!hsai_) return false;
    if (HAL_SAI_Receive_DMA(hsai_, reinterpret_cast<uint8_t*>(dmaBuffer_),
                            2 * HALF_WORDS) != HAL_OK) {
        ++errors_;
        return false;
    }
    running_ = true;
    return true;
}

void Sampling_Circuitry_116::stop()
{
    if (hsai_) HAL_SAI_DMAStop(hsai_);
    running_ = false;
}

uint64_t Sampling_Circuitry_116::now_us() const
{
    // Platzhalter bis Hardware-Zeitbasis (TIM / GNSS PPS) vorhanden ist
    return static_cast<uint64_t>(osKernelGetTickCount()) * 1000ULL;
}

void Sampling_Circuitry_116::onRxHalf()
{
    SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(dmaBuffer_), sizeof(int32_t) * HALF_WORDS);
    array_.pushBlock(&dmaBuffer_[0], DMA_BLOCK_SAMPLES, now_us());
}

void Sampling_Circuitry_116::onRxComplete()
{
    SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(&dmaBuffer_[HALF_WORDS]), sizeof(int32_t) * HALF_WORDS);
    array_.pushBlock(&dmaBuffer_[HALF_WORDS], DMA_BLOCK_SAMPLES, now_us());
}

void Sampling_Circuitry_116::onError()
{
    ++errors_;
    // DMA neu starten; HAL_SAI_ErrorCallback hat den Transfer bereits gestoppt
    if (running_) start();
}

} // namespace sds110

// ---------------------------------------------------------------- HAL hooks
// Genau eine Definition pro Projekt – die alten Hooks in adau7118.c
// (HAL_I2S_*) und stm32f7xx_it.c dürfen nicht parallel aktiv sein.
extern "C" {

void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef* hsai)
{
    (void)hsai;
    sds110::Sampling_Circuitry_116::instance().onRxHalf();
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef* hsai)
{
    (void)hsai;
    sds110::Sampling_Circuitry_116::instance().onRxComplete();
}

void HAL_SAI_ErrorCallback(SAI_HandleTypeDef* hsai)
{
    (void)hsai;
    sds110::Sampling_Circuitry_116::instance().onError();
}

} // extern "C"
