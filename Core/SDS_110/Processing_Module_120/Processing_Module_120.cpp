/*
 * Processing_Module_120.cpp
 */
#include "Processing_Module_120.hpp"
#include "Infrastructure/Model/SDS_Data.hpp"
#include "Infrastructure/Driver/USBDriver.hpp"
#include <cmath>

namespace sds110 {

SDS110_SDRAM_SECTION Spectrum Processing_Module_120::spectra_[NUM_MICS];

Processing_Module_120& Processing_Module_120::instance()
{
    // Instanz (122: Mel-Filterbank/FFT-Puffer, 126: Korrelationspuffer) im SDRAM.
    // Konstruktor läuft beim ersten Aufruf (SDS110_Init, nach MX_FMC_Init).
    static SDS110_SDRAM_SECTION Processing_Module_120 inst;
    return inst;
}

bool Processing_Module_120::init(SAI_HandleTypeDef* hsai, I2C_HandleTypeDef* hi2c)
{
    SDS_Data& dm = SDS_Data::instance();
    const bool unitOk = unit_.init(hsai, hi2c);
    if (!unitOk) dm.pushErrorMessage("116 init failed");

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
    if (!ml_.infer(features_, state_)) { dm.setMlRunError(true); return true; }
    dm.setMlRunError(false);
    dm.setAcousticState(state_);

    // Feedback der Tracking Unit (Abschnitt 10), falls vorhanden
    if (out_.pollFeedback(feedback_)) corr_.applyFeedback(feedback_);

    // (d)(e) 126: Selektion, Gewichtung, quellkonditionierte GCC-PHAT, Peilung
    corr_.deriveSelection(state_, selection_);
    corr_.estimateBearing(spectra_, selection_, bearing_);

    // (f) 128: Kandidatenposition (Einzel-Unit: Peilung + Pegel-Fallback)
    float levelA = 0.0f;
    for (uint32_t b = 0; b < NUM_BANDS; ++b) levelA += std::exp(features_.band_log_power[b]) * state_.p[b];
    levelA = std::sqrt(levelA);
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
    for (uint32_t m = 0; m < NUM_MICS; ++m)
        for (uint32_t f = 0; f < FRAME_SAMPLES / SDS_MSG_BUFFER_SIZE; ++f)
            USBDriver::sendRead(ts, m, f, frame);
    unit_.releaseFrame(frame);
    return true;
}

} // namespace sds110
