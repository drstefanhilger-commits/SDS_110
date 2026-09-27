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
 *   - UnitReport (Message id 5, 128-Byte-Payload) mit µs-Zeitstempel (UTC nach Abgleich über
 *     USB-Kommando Typ 7) und selektierten Bändern; ersetzt id 4 (Zeit nur in ms, Befund 33)
 */
#pragma once
#include "Data_Interface_140/Candidate_Report_140.hpp"
#include "Processing_Module_120/Localisation_Module_128/Localisation_Module_128.hpp"

namespace sds110 {

class Output_Interface_130 {
public:
    static constexpr uint32_t UNIT_REPORT_ID   = 5;    // Message-Id des UnitReport
    static constexpr uint32_t UNIT_REPORT_HEAD = 25;   // Kopf bis einschließlich level (Byte)
    bool init();
    void buildReport(const CandidateLocation& loc, const AcousticState& s, const ComponentSelection& sel,
                     float level, uint64_t uptime_us, UnitReport& r) const;   // uptime_us: Frame-Beginn, Laufzeit
    bool send(const UnitReport& r);
    bool pollFeedback(TrackingFeedback& fb);      // TODO: Nachrichtentyp 8 in USBTask (6 = SRP, 7 = UTC)
    uint32_t sent() const { return sent_; }
private:
    uint32_t sent_ = 0;
};

} // namespace sds110
