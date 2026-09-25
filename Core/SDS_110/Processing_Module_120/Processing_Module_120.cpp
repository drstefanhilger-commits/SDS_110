/*
 * Processing_Module_120.cpp
 */
#include "Processing_Module_120.hpp"
#include "Infrastructure/Model/SDS_Data.hpp"
#include "Infrastructure/Driver/USBDriver.hpp"
#include <cmath>

namespace sds110 {

SDS110_SDRAM_SECTION Spectrum Processing_Module_120::spectra_[NUM_MICS];
void* Processing_Module_120_spectraProbe() { return &Processing_Module_120::spectra_[0]; }

Processing_Module_120& Processing_Module_120::instance()
{
    // Instanz (122: Mel-Filterbank/FFT-Puffer, 126: Korrelationspuffer) im SDRAM.
    // Konstruktor läuft beim ersten Aufruf (SDS110_Init, nach MX_FMC_Init).
    static SDS110_SDRAM_SECTION Processing_Module_120 inst;
    return inst;
}

static bool sdramSelfTest()
{
    // schreibt/liest ein Muster in das erste Spektrum (liegt in .sdram_data)
    volatile uint32_t* p = reinterpret_cast<volatile uint32_t*>(Processing_Module_120_spectraProbe());
    const uint32_t pat[2] = { 0xA5A5F00Fu, 0x5A5A0FF0u };
    for (int i = 0; i < 2; ++i) { p[i] = pat[i]; }
    for (int i = 0; i < 2; ++i) { if (p[i] != pat[i]) return false; }
    return true;
}

bool Processing_Module_120::init(SAI_HandleTypeDef* hsai, I2C_HandleTypeDef* hi2c)
{
    SDS_Data& dm = SDS_Data::instance();
    if (!sdramSelfTest()) { dm.pushErrorMessage("SDRAM not initialised"); return false; }

    const bool unitOk = unit_.init(hsai, hi2c);
    if (!unitOk) dm.pushErrorMessage(unit_.sampling().errorCount() ? "116: SAI clock / I2C" : "116 init failed");

    feat_.init();
    const bool mlOk = ml_.init();
    dm.setMlInitError(!mlOk);
    corr_.init(unit_.array());
    const Vec3 origin{0, 0, 0};
    loc_.init(&origin, 1);                 // NUM_UNITS = 1: Referenzpunkt = Arraymitte
    out_.init();
    return unitOk;
}

bool Processing_Module_120::start() { return unit_.start(); }

bool Processing_Module_120::processFrame()
{
    SDS_Data& dm = SDS_Data::instance();
    MicFrame* frame = unit_.nextFrame();   // 112: Frame + Pre-Processing 118
    if (!frame) return false;

    // (b) 122: STFT Referenzkanal + Merkmale, dann übrige Kanäle für 126
    feat_.process(*frame, spectra_[REF_MIC], features_);
    for (uint32_t m = 0; m < NUM_MICS; ++m)
        if (m != REF_MIC) feat_.computeSpectrum(frame->data[m], FRAME_SAMPLES, spectra_[m]);
    const uint64_t t = frame->time_utc_us;
    unit_.releaseFrame(frame);

    // (c) 124: Acoustic State s(t)
    if (!ml_.infer(feat_.magnitude(), features_, state_)) { dm.setMlRunError(true); return true; }
    dm.setMlRunError(false);
    dm.setAcousticState(state_);
    dm.setHbd(ml_.hbd().f0Hz, ml_.hbd().score, ml_.hbd().globalSnrAvgDb, ml_.hbd().consistentBands, ml_.hbd().droneDetected);

    // Feedback der Tracking Unit (Abschnitt 10), falls vorhanden
    if (out_.pollFeedback(feedback_)) corr_.applyFeedback(feedback_);

    // (d)(e) 126: Selektion, Gewichtung, quellkonditionierte GCC-PHAT, Peilung
    corr_.deriveSelection(state_, selection_);
    corr_.estimateBearing(spectra_, selection_, bearing_);
    if (SRP_REFERENCE_ENABLED) {                       // Vergleich TDOA-LS (Patent) vs. SRP-PHAT (alt)
        float srpAz = 0.0f, srpPow = 0.0f, srpRatio = 0.0f;
        if (corr_.srpScan(srpAz, srpPow, srpRatio)) { dm.setDebugValue(0, srpAz); dm.setDebugValue(1, srpRatio); }
    }

    // (f) 128: Kandidatenposition (Einzel-Unit: Peilung + Pegel-Fallback)
    // levelA aus dem Referenzspektrum nach 118; durch die dort angewendete Verstärkung
    // (NS · AGC) teilen, sonst misst der Pegel die AGC statt der Quelle
    float levelA = 0.0f;
    for (uint32_t b = 0; b < NUM_BANDS; ++b) levelA += std::exp(features_.band_log_power[b]) * state_.p[b];
    const float gRef = unit_.preprocessor().appliedGain(REF_MIC);
    levelA = std::sqrt(levelA) / ((gRef > 1e-6f) ? gRef : 1e-6f);
    loc_.fromBearing(bearing_, levelA, location_);
    dm.setCandidate(location_.azimuth_deg, location_.distance_m, location_.accepted_pairs, location_.ls_residual, location_.valid);

    // UnitReport über 140 an das Processing Module (PC); der Candidate Report (g) entsteht dort
    if (location_.valid && dm.getDetected()) {
        out_.buildReport(location_, state_, selection_, levelA, t, report_);
        out_.send(report_);
    }
    dm.pushEvent(SDS_DataEventType::REPORT_UPDATE, 0.0f);
    return true;
}

bool Processing_Module_120::streamFrame()
{
    MicFrame* frame = unit_.nextFrame();
    if (!frame) return false;
    const uint32_t ts = static_cast<uint32_t>(frame->time_utc_us / 1000ULL);
    // 192 x 532 B je Frame (~1,6 MB/s) übersteigt USB-FS: auf Platz im TX-Puffer warten;
    // gelingt das nicht, Rest des Frames verwerfen statt einzelne Pakete zu verlieren.
    // Nicht gestreamte Frames zählt 114 als dropped.
    constexpr uint32_t kWaitMs = 20;
    bool ok = true;
    for (uint32_t m = 0; m < NUM_MICS && ok; ++m)
        for (uint32_t f = 0; f < FRAME_SAMPLES / SDS_MSG_BUFFER_SIZE && ok; ++f)
            ok = USBDriver::sendRead(ts, m, f, frame, kWaitMs);
    unit_.releaseFrame(frame);
    return true;
}

} // namespace sds110
