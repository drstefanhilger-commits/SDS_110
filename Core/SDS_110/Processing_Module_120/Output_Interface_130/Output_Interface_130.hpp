/*
 * Output_Interface_130.hpp
 *
 * Ausgabeschnittstelle der Sensoreinheit (Board-Seite von 130/140).
 * Baut den UnitReport und sendet ihn über USB-CDC an das Processing Module (PC);
 * nimmt Feedback (ŝ, x̂) für 126 entgegen.
 *
 * Der Candidate Report des Patents (FIG. 6) wird erst auf dem PC aus der
 * Inter-Unit-TDOA-Lösung (128) gebildet – dort liegt die eigentliche 130.
 *
 * Transport:
 *   - Legacy-Detect-Frame (32 Byte, id 1) für den bestehenden PC-Monitor:
 *     mic = Unit-ID, azi = Peilung, distance = Pegel-Fallback, conf = Peilqualität
 *   - UnitReport (Message id 4, 128-Byte-Payload) mit selektierten Bändern
 */
#pragma once
#include "Data_Interface_140/Candidate_Report_140.hpp"
#include "Processing_Module_120/Localisation_Module_128/Localisation_Module_128.hpp"

namespace sds110 {

class Output_Interface_130 {
public:
    bool init();
    void buildReport(const CandidateLocation& loc, const AcousticState& s, const ComponentSelection& sel,
                     float level, uint64_t time_utc_us, UnitReport& r) const;
    bool send(const UnitReport& r);
    bool pollFeedback(TrackingFeedback& fb);      // TODO: Nachrichtentyp 6 in USBTask
    uint32_t sent() const { return sent_; }
private:
    uint32_t sent_ = 0;
};

} // namespace sds110
