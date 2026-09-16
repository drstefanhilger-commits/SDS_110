/*
 * Sampling_Circuitry_116.hpp
 *
 * Synchrone Abtastschaltung 116 (Patent, Abschnitt 1, FIG. 2):
 *  ADAU7118 (PDM -> TDM-8, 32-bit Slots, 48 kHz) -> SAI (Master RX) -> DMA
 *  Ping-Pong -> Demux -> Microphone_Array_114::pushBlock().
 *
 * Migration aus SDS (Vereinigung der beiden Pfade):
 *  - adua7118Driver.c : Registermap, Enable-Pin PE3, I2C-Schreibsequenz
 *  - sai.c            : SAI Master-RX TDM-8 Konfiguration (FrameLength 256)
 *  - adau7118.c       : DMA-Half/Complete-Callbacks, Demux interleaved -> Kanal
 *  - dma.c            : DMA-IRQ-Freigabe
 *  Entfallen: TDM_Parser + SDS_RingBuffer (1 Frame à 8 Samples je IRQ) –
 *             ersetzt durch Blockübernahme von DMA_BLOCK_SAMPLES je Halbpuffer.
 *
 * Hardware-Handles (hsai, hi2c) kommen aus main.c / CubeMX.
 * Zeitreferenz: aktuell osKernelGetTickCount in µs; für Inter-Unit-Sync
 * (Patent: GNSS / PTP, 10 µs) später durch Hardware-Timer ersetzen.
 */
#pragma once
#include <cstdint>
#include "stm32f7xx_hal.h"
#include "SDS_110_Config.hpp"
#include "Microphone_Array_114.hpp"

namespace sds110 {

class Sampling_Circuitry_116 {
public:
    static Sampling_Circuitry_116& instance();

    /// Handles binden; Codec über I2C konfigurieren; SAI initialisieren
    bool init(SAI_HandleTypeDef* hsai, I2C_HandleTypeDef* hi2c);
    bool start();
    void stop();
    bool running() const { return running_; }

    /// aus HAL_SAI_RxHalfCpltCallback / HAL_SAI_RxCpltCallback
    void onRxHalf();
    void onRxComplete();
    void onError();

    uint32_t errorCount() const { return errors_; }

private:
    Sampling_Circuitry_116() = default;
    bool configureCodec();
    bool writeReg(uint8_t reg, uint8_t val);
    bool configureSai();
    void enablePin(bool on);
    uint64_t now_us() const;

    SAI_HandleTypeDef* hsai_ = nullptr;
    I2C_HandleTypeDef* hi2c_ = nullptr;
    bool     running_ = false;
    uint32_t errors_  = 0;
    Microphone_Array_114& array_ = Microphone_Array_114::instance();

    static constexpr uint32_t HALF_WORDS = DMA_BLOCK_SAMPLES * NUM_MICS;
    static int32_t dmaBuffer_[2 * HALF_WORDS];   // Ping-Pong, interleaved TDM
};

} // namespace sds110
