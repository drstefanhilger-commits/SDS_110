/*
 * Feature_Extraction_Module_122.hpp
 *
 * Merkmalsextraktion 122 (Patent, Abschnitt 2, FIG. 2):
 *  - Frame 64 ms (3072 Samples), Hann-Fenster, Zero-Padding auf N_FFT = 4096,
 *    STFT via CMSIS arm_rfft_fast_f32 -> X(t,k), k = 0 .. 2048
 *  - Partition in B = 64 Bänder K_b à 62,5 Hz ab 80 Hz (bandBins)
 *  - Merkmale je Frame (Referenzkanal):
 *      band_log_power[B]   Log-Leistung je Band
 *      mel[MEL_BANDS]      Log-Mel (Filterbank zur Laufzeit erzeugt, sparse)
 *      spectral_flux       positive Magnitudenänderung zum Vorframe (normiert)
 *      band_am_depth[B]    Amplitudenmodulation je Band über AM_HISTORY_FRAMES
 *                          (std/mean der Bandleistung ~0,5 s)
 *
 * Migration aus SDS:
 *  - ML/FFTProcessor      : Hann + arm_rfft_fast (256 -> 4096 Punkte)
 *  - ML/MelFilterbank     : 40x129-Flash-Tabelle -> Laufzeit-Dreiecksfilter für 2049 Bins
 *  - ML/MelSpectrogram    : arm_dot_prod + log -> sparse Skalarprodukt
 *  - SDS_Data::computeMelFeatures : Ablauf (Mic 0 -> FFT -> Mel)
 *  ACHTUNG: Das bisherige CubeAI-Modell (40 Log-Mel aus 256-Punkt-FFT) ist mit
 *  der neuen Auflösung nicht kompatibel und muss neu trainiert werden (124).
 */
#pragma once
#include "arm_math.h"
#include "SDS_110_Config.hpp"
#include "Sensor_Unit_112/Frame_Assembler.hpp"

namespace sds110 {

/// Komplexes Spektrum eines Kanals, k = 0 .. N_FFT/2
struct Spectrum {
    float re[NUM_BINS];
    float im[NUM_BINS];
};

struct FeatureVector {
    float band_log_power[NUM_BANDS];
    float mel[MEL_BANDS];
    float spectral_flux;
    float band_am_depth[NUM_BANDS];
};

class Feature_Extraction_Module_122 {
public:
    void init();

    /// Referenzkanal: STFT + alle Merkmale (Standardpfad Claim 1 (b))
    void process(const AnalysisFrame& frame, Spectrum& refSpectrum, FeatureVector& features);

    /// STFT eines beliebigen Kanals (für 126: Intra-Unit-Korrelation)
    void computeSpectrum(const float* x, uint32_t n, Spectrum& out);

    /// Bin-Bereich [k_lo, k_hi) des Bandes b
    static void bandBins(uint32_t b, uint32_t& k_lo, uint32_t& k_hi);
    static float binHz(uint32_t k) { return static_cast<float>(k) * SAMPLE_RATE_HZ / N_FFT; }

    const float* magnitude() const { return mag_; }

private:
    void computeBandPower(const float* mag, float* bandPow);
    void computeMel(const float* mag, float* mel);
    float computeFlux(const float* mag);
    void  updateAm(const float* bandPow, float* amDepth);
    void  buildMelFilterbank();
    static float hzToMel(float hz) { return 2595.0f * std::log10(1.0f + hz / 700.0f); }
    static float melToHz(float m)  { return 700.0f * (std::pow(10.0f, m / 2595.0f) - 1.0f); }

    arm_rfft_fast_instance_f32 fft_;
    float window_[FRAME_SAMPLES];
    float buf_[N_FFT];
    float fftOut_[N_FFT];
    float mag_[NUM_BINS];
    float prevMag_[NUM_BINS];
    bool  havePrev_ = false;

    // Sparse Mel-Filterbank: je Band Startbin + Gewichte
    struct MelFilter { uint32_t k0; uint32_t len; float w[MEL_MAX_BINS_PER_BAND]; };
    MelFilter mel_[MEL_BANDS];

    // AM-Historie der Bandleistung (Ring)
    float amHist_[AM_HISTORY_FRAMES][NUM_BANDS];
    uint32_t amIdx_   = 0;
    uint32_t amCount_ = 0;
};

} // namespace sds110
