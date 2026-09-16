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
