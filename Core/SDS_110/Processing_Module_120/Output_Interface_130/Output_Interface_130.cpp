/*
 * Output_Interface_130.cpp
 */
#include "Output_Interface_130.hpp"
#include "Infrastructure/Driver/USBDriver.hpp"
#include "Infrastructure/Model/SDS_Data.hpp"
#include <cstring>

namespace sds110 {

bool Output_Interface_130::init() { sent_ = 0; return true; }

void Output_Interface_130::buildReport(const CandidateLocation& loc, const AcousticState& s,
                                       const ComponentSelection& sel, float level, uint64_t t, UnitReport& r) const
{
    r = UnitReport{};
    r.unit_id          = SDS_Data::instance().getId();
    r.time_utc_us      = t;
    r.bearing_deg      = loc.azimuth_deg;
    r.bearing_residual = loc.ls_residual;
    r.confidence       = loc.confidence;
    r.valid_pairs      = loc.accepted_pairs;
    r.level            = level;
    r.num_selected     = 0;
    for (uint32_t b = 0; b < NUM_BANDS; ++b) {
        uint32_t k0, k1; Feature_Extraction_Module_122::bandBins(b, k0, k1);
        if (k0 < NUM_BINS && sel.selected[k0]) {
            r.band_index[r.num_selected] = static_cast<uint8_t>(b);
            r.band_prob [r.num_selected] = s.p[b];
            ++r.num_selected;
        }
    }
}

bool Output_Interface_130::send(const UnitReport& r)
{
    const uint32_t ts = static_cast<uint32_t>(r.time_utc_us / 1000ULL);
    const float distFallback = (SINGLE_UNIT_LEVEL_DISTANCE && r.level > 0.0f) ? LEVEL_DIST_K_REF / (r.level + LEVEL_DIST_EPS) : 0.0f;

    // 1) Legacy-Frame für den bestehenden PC-Monitor
    bool ok = USBDriver::sendDetection(ts, r.unit_id, r.bearing_deg, distFallback, r.confidence);

    // 2) UnitReport (id 4): [unit u16][bearing f32][residual f32][pairs u8][nsel u8][level f32][idx u8 x nsel][p u8 x nsel]
    MessageData d{};
    uint32_t o = 0;
    std::memcpy(&d.b[o], &r.unit_id, 2);          o += 2;
    std::memcpy(&d.b[o], &r.bearing_deg, 4);      o += 4;
    std::memcpy(&d.b[o], &r.bearing_residual, 4); o += 4;
    // Nutzlast 128 Byte: 16 Byte Kopf + 2 Byte je Band -> höchstens 56 Bänder. Im Feld nsel
    // steht die tatsächlich gesendete Anzahl, sonst liest der PC über das Ende hinaus.
    constexpr uint32_t kMaxBands = (sizeof(MessageData) - 16) / 2;   // 56
    const uint32_t n = (r.num_selected > kMaxBands) ? kMaxBands : r.num_selected;
    d.b[o++] = r.valid_pairs;
    d.b[o++] = static_cast<uint8_t>(n);
    std::memcpy(&d.b[o], &r.level, 4);            o += 4;
    for (uint32_t i = 0; i < n; ++i) d.b[o++] = r.band_index[i];
    for (uint32_t i = 0; i < n; ++i) d.b[o++] = static_cast<uint8_t>(r.band_prob[i] * 255.0f);
    ok = USBDriver::sendMessage(4, ts, d) && ok;

    if (ok) ++sent_;
    return ok;
}

bool Output_Interface_130::pollFeedback(TrackingFeedback& fb)
{
    fb = TrackingFeedback{};
    return false;
}

} // namespace sds110
