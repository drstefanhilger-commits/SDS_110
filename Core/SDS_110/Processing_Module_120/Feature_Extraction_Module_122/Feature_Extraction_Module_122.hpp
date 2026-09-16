/*
 * Feature_Extraction_Module_122.hpp
 * Merkmalsextraktion 122 (Patent, Abschnitt 2, FIG. 2):
 *  - Framing 64 ms / 50 %, STFT 4096 -> X(t,k)
 *  - Partition in B = 64 Bänder K_b
 *  - Merkmale pro Frame: Log-Leistung je Band, Log-Mel/MFCC,
 *    spektraler Fluss, Amplitudenmodulationsspektrum je Band
 * Migration: ML/FFTProcessor, MelFilterbank, MelSpectrogram, SDS_Data::computeMelFeatures
 */
#pragma once
#include "SDS_110_Config.hpp"
#include "Sensor_Unit_112/Microphone_Array_114.hpp"

namespace sds110 {

/// Komplexes Spektrum des Referenzkanals einer Unit
struct Spectrum {
    float re[NUM_BINS];
    float im[NUM_BINS];
};

struct FeatureVector {
    float band_log_power[NUM_BANDS];
    float mel[40];                  // Ordnung [..] gemäß Patent offen
    float spectral_flux;
    float band_am_spectrum[NUM_BANDS];
};

class Feature_Extraction_Module_122 {
public:
    void init();
    /// STFT aller Mikrofone; Spektrum des Referenzkanals + Merkmale
    void process(const MicFrame& frame, Spectrum& refSpectrum, FeatureVector& features);
    /// Bin-Bereich [k_lo, k_hi) des Bandes b
    static void bandBins(uint32_t b, uint32_t& k_lo, uint32_t& k_hi);
private:
    float window_[N_FFT];
};

} // namespace sds110
