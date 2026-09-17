/*
 * Correlation_Processing_Module_126.cpp
 */
#include "Correlation_Processing_Module_126.hpp"
#include <cmath>
#include <cstring>

namespace sds110 {

// ---------------------------------------------------------------- init
void Correlation_Processing_Module_126::init(const Microphone_Array_114& array)
{
    arm_rfft_fast_init_f32(&ifft_, N_FFT);
    float dmax = 0.0f;
    for (uint32_t m = 0; m < NUM_MICS; ++m) {
        micPos_[m] = array.position(m);
        for (uint32_t n = m + 1; n < NUM_MICS; ++n) {
            const Vec3& a = array.position(m); const Vec3& b = array.position(n);
            const float d = std::sqrt((a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y) + (a.z-b.z)*(a.z-b.z));
            if (d > dmax) dmax = d;
        }
    }
    maxIntraDelay_s_ = dmax / SPEED_OF_SOUND * 1.1f;   // 10 % Reserve
    uint32_t idx = 0;
    for (uint32_t i = 0; i < NUM_MICS; ++i)
        for (uint32_t j = i + 1; j < NUM_MICS; ++j, ++idx) {
            pairDx_[idx] = (micPos_[i].x - micPos_[j].x) / SPEED_OF_SOUND * SAMPLE_RATE_HZ;
            pairDy_[idx] = (micPos_[i].y - micPos_[j].y) / SPEED_OF_SOUND * SAMPLE_RATE_HZ;
        }
    std::memset(pairCorr_, 0, sizeof(pairCorr_));
    clearFeedback();
}

void Correlation_Processing_Module_126::clearFeedback()
{
    for (uint32_t b = 0; b < NUM_BANDS; ++b) { thetaSel_[b] = THETA_SEL; weightBoost_[b] = 1.0f; }
    feedback_ = TrackingFeedback{};
}

void Correlation_Processing_Module_126::applyFeedback(const TrackingFeedback& fb)
{
    feedback_ = fb;
    for (uint32_t b = 0; b < NUM_BANDS; ++b) {
        const bool ref = fb.valid && fb.ref_state[b] > THETA_REF;
        thetaSel_[b]    = ref ? THETA_LOW : THETA_SEL;          // Abschnitt 10 (a)
        weightBoost_[b] = ref ? (1.0f + fb.ref_state[b]) : 1.0f; // Abschnitt 10 (b)
    }
}

// ---------------------------------------------------------------- Abschnitt 4
void Correlation_Processing_Module_126::deriveSelection(const AcousticState& s, ComponentSelection& sel) const
{
    std::memset(sel.selected, 0, sizeof(sel.selected));
    std::memset(sel.weight, 0, sizeof(sel.weight));
    sel.num_bands = sel.num_bins = 0;

    // Bänder mit p_b > θ_sel; falls weniger als B_MIN, die B_MIN stärksten nehmen
    bool take[NUM_BANDS];
    uint32_t n = 0;
    for (uint32_t b = 0; b < NUM_BANDS; ++b) { take[b] = s.p[b] > thetaSel_[b]; if (take[b]) ++n; }
    if (n < B_MIN) {
        for (uint32_t k = n; k < B_MIN; ++k) {
            uint32_t best = NUM_BANDS; float pv = -1.0f;
            for (uint32_t b = 0; b < NUM_BANDS; ++b)
                if (!take[b] && s.p[b] > pv) { pv = s.p[b]; best = b; }
            if (best == NUM_BANDS) break;
            take[best] = true;
        }
    }
    for (uint32_t b = 0; b < NUM_BANDS; ++b) {
        if (!take[b]) continue;
        ++sel.num_bands;
        const float w = std::pow(s.p[b], WEIGHT_GAMMA) * weightBoost_[b];
        uint32_t k0, k1;
        Feature_Extraction_Module_122::bandBins(b, k0, k1);
        for (uint32_t k = k0; k < k1 && k < NUM_BINS; ++k) {
            sel.selected[k] = true; sel.weight[k] = w; ++sel.num_bins;
        }
    }
}

// ---------------------------------------------------------------- Abschnitt 5
bool Correlation_Processing_Module_126::crossCorrelate(const Spectrum& X, const Spectrum& Y,
                                                        const ComponentSelection& sel,
                                                        float maxDelay_s, TdoaMeasurement& out)
{
    out.valid = false;
    if (sel.num_bins == 0) return false;

    // R(k) = w(k) · X Y* / |X Y*| auf S(t), sonst 0  -> CMSIS-Packing [Re0, ReN/2, Re1, Im1, ...]
    std::memset(spec_, 0, sizeof(spec_));
    for (uint32_t k = 1; k < NUM_BINS - 1; ++k) {
        if (!sel.selected[k]) continue;
        const float cr = X.re[k] * Y.re[k] + X.im[k] * Y.im[k];
        const float ci = X.im[k] * Y.re[k] - X.re[k] * Y.im[k];
        float mag = std::sqrt(cr * cr + ci * ci);
        if (mag < 1e-9f) continue;
        spec_[2 * k]     = sel.weight[k] * cr / mag;
        spec_[2 * k + 1] = sel.weight[k] * ci / mag;
    }
    arm_rfft_fast_f32(&ifft_, spec_, corr_, 1);      // reelle Kreuzkorrelation, zirkulär

    // Peak in ±maxDelay (negative Lags liegen am Ende des Puffers)
    int maxLag = static_cast<int>(maxDelay_s * SAMPLE_RATE_HZ);
    if (maxLag < 1) maxLag = 1;
    if (maxLag > static_cast<int>(N_FFT / 2 - 1)) maxLag = N_FFT / 2 - 1;
    auto at = [&](int lag) { return corr_[(lag + static_cast<int>(N_FFT)) % N_FFT]; };

    int bestLag = 0; float best = -1e30f;
    for (int lag = -maxLag; lag <= maxLag; ++lag) { const float v = at(lag); if (v > best) { best = v; bestLag = lag; } }
    if (best <= 0.0f) return false;

    // Zweithöchster Peak außerhalb der Nachbarschaft des Hauptpeaks (Peak-Ratio-Test)
    const int excl = static_cast<int>(PEAK_EXCLUDE_S * SAMPLE_RATE_HZ);
    float second = 0.0f;
    for (int lag = -maxLag; lag <= maxLag; ++lag) {
        if (std::abs(lag - bestLag) <= excl) continue;
        const float v = at(lag); if (v > second) second = v;
    }
    const float ratio = best / (second + 1e-9f);

    // Sub-Sample-Interpolation (Parabel)
    const float ym = at(bestLag - 1), y0 = at(bestLag), yp = at(bestLag + 1);
    const float den = ym - 2.0f * y0 + yp;
    const float delta = (std::fabs(den) > 1e-12f) ? 0.5f * (ym - yp) / den : 0.0f;

    out.tdoa_s     = (bestLag + delta) / static_cast<float>(SAMPLE_RATE_HZ);
    out.peak       = best;
    out.peak_ratio = ratio;
    out.valid      = ratio >= PEAK_RATIO_MIN;
    return out.valid;
}

// ---------------------------------------------------------------- Intra-Unit-Peilung
// Fernfeld: τ_ij = ((p_i - p_j) · u) / c mit u = (cos φ, sin φ). Gewichtete LS in (ux, uy),
// Gewicht = Peakhöhe; Azimut = atan2(uy, ux).
bool Correlation_Processing_Module_126::estimateBearing(const Spectrum* S, const ComponentSelection& sel, Bearing& out)
{
    out = Bearing{};
    float sxx = 0, sxy = 0, syy = 0, bx = 0, by = 0, peakSum = 0;
    uint32_t idx = 0, valid = 0;
    for (uint32_t i = 0; i < NUM_MICS; ++i) {
        for (uint32_t j = i + 1; j < NUM_MICS; ++j, ++idx) {
            TdoaMeasurement& m = pairTdoa_[idx];
            m.i = i; m.j = j;
            const bool ok = crossCorrelate(S[i], S[j], sel, maxIntraDelay_s_, m);
            if (SRP_REFERENCE_ENABLED) {
                // Korrelationsfenster für den SRP-Scan sichern (corr_ ist zirkulär, negative Lags am Ende)
                for (int lag = -static_cast<int>(SRP_MAX_LAG); lag <= static_cast<int>(SRP_MAX_LAG); ++lag)
                    pairCorr_[idx][lag + SRP_MAX_LAG] = (sel.num_bins == 0) ? 0.0f : corr_[(lag + static_cast<int>(N_FFT)) % N_FFT];
            }
            if (!ok) continue;
            ++valid; peakSum += m.peak;
            const float ax = (micPos_[i].x - micPos_[j].x) / SPEED_OF_SOUND;
            const float ay = (micPos_[i].y - micPos_[j].y) / SPEED_OF_SOUND;
            const float w  = m.peak;
            sxx += w * ax * ax; sxy += w * ax * ay; syy += w * ay * ay;
            bx  += w * ax * m.tdoa_s; by += w * ay * m.tdoa_s;
        }
    }
    out.valid_pairs = static_cast<uint8_t>(valid);
    if (valid < static_cast<uint32_t>(NUM_MIC_PAIRS * BEARING_MIN_PAIRS_FRACTION)) return false;

    const float det = sxx * syy - sxy * sxy;
    if (std::fabs(det) < 1e-18f) return false;
    const float ux = ( syy * bx - sxy * by) / det;
    const float uy = (-sxy * bx + sxx * by) / det;

    // Residuum
    float res = 0.0f; idx = 0;
    for (uint32_t i = 0; i < NUM_MICS; ++i)
        for (uint32_t j = i + 1; j < NUM_MICS; ++j, ++idx) {
            const TdoaMeasurement& m = pairTdoa_[idx];
            if (!m.valid) continue;
            const float pred = ((micPos_[i].x - micPos_[j].x) * ux + (micPos_[i].y - micPos_[j].y) * uy) / SPEED_OF_SOUND;
            res += m.peak * (m.tdoa_s - pred) * (m.tdoa_s - pred);
        }
    out.residual    = std::sqrt(res / peakSum);
    out.mean_peak   = peakSum / valid;
    float az = std::atan2(uy, ux) * 180.0f / PI;
    if (az < 0.0f) az += 360.0f;
    out.azimuth_deg = az;
    out.valid = true;
    return true;
}

// ---------------------------------------------------------------- SRP-PHAT-Referenz
// SRP(φ) = Σ_pairs C_ij(τ_ij(φ)), τ_ij(φ) = ((p_i - p_j)·u(φ)) / c · fs, linear interpoliert.
bool Correlation_Processing_Module_126::srpScan(float& azimuth_deg, float& peakPower, float& peakRatio) const
{
    float best = -1e30f, second = -1e30f; uint32_t bestStep = 0;
    for (uint32_t s = 0; s < SRP_AZ_STEPS; ++s) {
        const float phi = static_cast<float>(s) * (2.0f * PI / SRP_AZ_STEPS);
        const float ux = std::cos(phi), uy = std::sin(phi);
        float acc = 0.0f;
        for (uint32_t k = 0; k < NUM_MIC_PAIRS; ++k) {
            const float tau = pairDx_[k] * ux + pairDy_[k] * uy;            // Lag in Samples
            const float pos = tau + static_cast<float>(SRP_MAX_LAG);
            const int   i0  = static_cast<int>(std::floor(pos));
            if (i0 < 0 || i0 + 1 > static_cast<int>(2 * SRP_MAX_LAG)) continue;
            const float fr = pos - static_cast<float>(i0);
            acc += pairCorr_[k][i0] * (1.0f - fr) + pairCorr_[k][i0 + 1] * fr;
        }
        if (acc > best) { second = best; best = acc; bestStep = s; }
        else if (acc > second) second = acc;
    }
    if (best <= 0.0f) return false;
    // Parabel-Interpolation um das Maximum (1°-Raster)
    auto at = [&](int s) {
        s = (s + static_cast<int>(SRP_AZ_STEPS)) % static_cast<int>(SRP_AZ_STEPS);
        const float phi = static_cast<float>(s) * (2.0f * PI / SRP_AZ_STEPS);
        const float ux = std::cos(phi), uy = std::sin(phi); float acc = 0.0f;
        for (uint32_t k = 0; k < NUM_MIC_PAIRS; ++k) {
            const float pos = pairDx_[k] * ux + pairDy_[k] * uy + static_cast<float>(SRP_MAX_LAG);
            const int i0 = static_cast<int>(std::floor(pos));
            if (i0 < 0 || i0 + 1 > static_cast<int>(2 * SRP_MAX_LAG)) continue;
            const float fr = pos - static_cast<float>(i0);
            acc += pairCorr_[k][i0] * (1.0f - fr) + pairCorr_[k][i0 + 1] * fr;
        }
        return acc;
    };
    const float ym = at(static_cast<int>(bestStep) - 1), y0 = best, yp = at(static_cast<int>(bestStep) + 1);
    const float den = ym - 2.0f * y0 + yp;
    const float delta = (std::fabs(den) > 1e-12f) ? 0.5f * (ym - yp) / den : 0.0f;
    float az = (static_cast<float>(bestStep) + delta) * (360.0f / SRP_AZ_STEPS);
    if (az < 0.0f)    az += 360.0f;
    if (az >= 360.0f) az -= 360.0f;
    azimuth_deg = az; peakPower = best; peakRatio = best / (std::fabs(second) + 1e-9f);
    return true;
}

} // namespace sds110
