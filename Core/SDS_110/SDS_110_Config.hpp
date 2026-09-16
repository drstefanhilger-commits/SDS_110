/*
 * SDS_110_Config.hpp
 * Implementierungsparameter des bevorzugten Ausführungsbeispiels
 * (Patent, Abschnitte 1–10; Werte in [ ] sind noch zu bestätigen).
 */
#pragma once
#include <cstdint>

namespace sds110 {

// --- 1. Systemarchitektur ---------------------------------------------
constexpr uint32_t NUM_UNITS        = 1;      // N Sensor Units (Patent: N >= 3)
constexpr uint32_t NUM_MICS         = 8;      // M pro Unit (Oktagon)
constexpr uint32_t SAMPLE_RATE_HZ   = 48000;
constexpr float    BANDPASS_LO_HZ   = 80.0f;
constexpr float    BANDPASS_HI_HZ   = 8000.0f;

// --- 2. Framing / STFT --------------------------------------------------
constexpr uint32_t FRAME_MS         = 64;
constexpr uint32_t FRAME_OVERLAP_PC = 50;
constexpr uint32_t N_FFT            = 4096;
constexpr uint32_t NUM_BINS         = N_FFT / 2 + 1;
constexpr uint32_t NUM_BANDS        = 64;     // B
constexpr float    BAND_WIDTH_HZ    = 62.5f;  // Δf
constexpr float    BAND_LO_HZ       = 80.0f;
constexpr float    BAND_HI_HZ       = 4000.0f;
constexpr uint32_t REF_MIC          = 0;      // Referenzkanal (Patent: Mikrofon [1])
constexpr uint32_t FRAME_SAMPLES    = SAMPLE_RATE_HZ * FRAME_MS / 1000;   // 3072
constexpr uint32_t HOP_SAMPLES      = FRAME_SAMPLES * (100 - FRAME_OVERLAP_PC) / 100; // 1536

// --- 114 / 116 Hardware ---------------------------------------------
constexpr float    MIC_RADIUS_M     = 0.20f;  // Oktagon-Radius (aus SDS_Params)
constexpr uint32_t DMA_BLOCK_SAMPLES = 128;   // Samples pro Mic je DMA-Halbpuffer
constexpr uint32_t NUM_MIC_FRAMES   = 3;      // Triple-Buffering
constexpr uint8_t  ADAU7118_I2C_ADDR_7B = 0x4B; // alt: 0x3A in adau7118.c – prüfen!
constexpr uint32_t ADAU7118_I2C_TIMEOUT_MS = 100;
/// Große Puffer im externen SDRAM ablegen (SDRAMDriver muss vorher initialisiert sein)
#define SDS110_SDRAM_SECTION __attribute__((section(".sdram_data")))

// --- 118 Pre-Processing -------------------------------------------------
constexpr float    AGC_TARGET_RMS   = 0.1f;   // Zielpegel (float, Vollaussteuerung = 1)
constexpr float    AGC_MAX_GAIN     = 32.0f;  // +30 dB
constexpr float    AGC_MIN_GAIN     = 0.05f;  // -26 dB
constexpr float    AGC_ATTACK       = 0.30f;  // Glättung pro Frame, Pegel steigt
constexpr float    AGC_RELEASE      = 0.05f;  // Glättung pro Frame, Pegel fällt
constexpr float    NS_FLOOR_ALPHA   = 0.02f;  // Rauschboden-Nachführung (langsam)
constexpr float    NS_MAX_ATTEN     = 0.25f;  // maximale Dämpfung (-12 dB) bei reinem Rauschen

// --- 122 Feature Extraction ---------------------------------------------
constexpr uint32_t MEL_BANDS        = 40;     // wie altes Modell
constexpr float    MEL_LO_HZ        = 80.0f;
constexpr float    MEL_HI_HZ        = 8000.0f;
constexpr uint32_t MEL_MAX_BINS_PER_BAND = 512; // sparse Filterbank
constexpr uint32_t AM_HISTORY_FRAMES = 16;    // ~0.5 s bei 32 ms Hop

// --- 3. ML ------------------------------------------------------------
constexpr uint32_t STATE_SMOOTH_FRAMES = 3;

// --- 4. Selektion / Gewichtung -----------------------------------------
constexpr float    THETA_SEL        = 0.5f;   // Selektionsschwelle
constexpr uint32_t B_MIN            = 3;      // min. selektierte Bänder
constexpr float    WEIGHT_GAMMA     = 1.0f;   // g(p)=p^γ, γ=1 bevorzugt

// --- 5. GCC-PHAT / TDOA -------------------------------------------------
constexpr float    PEAK_RATIO_MIN   = 1.5f;
constexpr float    SPEED_OF_SOUND   = 343.0f; // wird temperaturkorrigiert

// --- 6. Lokalisation ----------------------------------------------------
constexpr uint32_t MIN_PAIRS        = 3;

// --- 10. Feedback -------------------------------------------------------
constexpr float    THETA_REF        = 0.6f;
constexpr float    THETA_LOW        = 0.3f;
constexpr float    TDOA_WINDOW_S    = 2e-3f;

} // namespace sds110
