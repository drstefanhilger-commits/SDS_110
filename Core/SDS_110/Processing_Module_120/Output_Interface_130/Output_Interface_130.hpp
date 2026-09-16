/*
 * Output_Interface_130.hpp
 * Ausgabeschnittstelle 130 (Patent, Abschnitt 7, FIG. 6):
 * Baut den Candidate Report und sendet ihn über 140 (~30 Reports/s);
 * empfängt Feedback (ŝ, x̂) und reicht es an 126 weiter.
 * Migration: Driver/USBDriver, SDSUSBMicSender, Tasks/USBTask, USB_SendDetection
 */
#pragma once
#include "Data_Interface_140/Candidate_Report_140.hpp"
#include "Processing_Module_120/Localisation_Module_128/Localisation_Module_128.hpp"

namespace sds110 {

class Output_Interface_130 {
public:
    bool init();
    void buildReport(const CandidateLocation& loc, const AcousticState& state,
                     const ComponentSelection& sel, uint64_t time_utc_us,
                     CandidateReport& report) const;
    bool send(const CandidateReport& report);
    bool pollFeedback(TrackingFeedback& fb);
};

} // namespace sds110
