# Migration Processing_Module_120 (124 / 126 / 128 / 130 / 120 / ProcessingTask)

**Zuordnung (siehe README):** Das Board ist Sensoreinheit 112-n; die Klasse `Processing_Module_120`
hier ist die *Vorstufe* auf der Einheit (122, 124, 126-intra). Inter-Unit-Korrelation, `128::solve()`
und der Candidate Report liegen im Processing Module auf dem PC. Das Board sendet einen `UnitReport`.

| Neu | Aus SDS | Änderung |
|---|---|---|
| Machine_Learning_Module_124.{hpp,cpp}, HBD.{hpp,cpp} | HBD-Vorlage (klassischer Harmonic Band Detector) | HBD auf |X| aus 122; Brücke HBD -> s(t) über Band-SNR, Harmonischen-Boost, Score-Gate |
| Correlation_Processing_Module_126.{hpp,cpp} | Algorithm/SRPPhat, Model/SDS_SRPBuffers | GCC-PHAT jetzt quellkonditioniert (S(t), w); SRP-Gitterscan durch TDOA-Least-Squares-Peilung ersetzt; kiss_fft → CMSIS |
| Localisation_Module_128.{hpp,cpp} | Algorithm/DistanceEstimator, SRP_DAS_Distance, SRPPhat::calibrateAzimuth | hyperbolische LS-Lösung für N≥3; Einzel-Unit: Peilung + Pegel-Fallback |
| Output_Interface_130.{hpp,cpp} | USB_SendDetection in SRPTask | Legacy-Frame + neuer Candidate Report (id 4) |
| Processing_Module_120.{hpp,cpp} | Tasks/SRPTask (detectHandler/aiHandler) | Orchestrator 112→122→124→126→128→130 |
| Infrastructure/Tasks/ProcessingTask.{hpp,cpp} | Tasks/SRPTask (Task-Hülle) | Moduswahl DETECT/READ |
| entfällt | Algorithm/TrackingTask, SRPPhat::filterAzimuth | Tracking Unit 150, nicht in SDS 110 (Claim 10) |
| entfällt | Third_Party/kiss_fft | CMSIS-DSP |

## Speicher (statisch)
- Processing_Module_120: spectra_ 8 × 16,4 kB = 131 kB (SDRAM-Sektion)
- Feature_Extraction_Module_122 (Mitglied von 120): ~145 kB (MEL_MAX_BINS_PER_BAND=512) – die Instanz
  liegt im internen RAM! Entweder `MEL_MAX_BINS_PER_BAND` auf 128 (→ ~85 kB) oder 120 als Ganzes in SDRAM.
  Empfehlung: `Processing_Module_120::instance()` durch eine `SDS110_SDRAM_SECTION static` Instanz ersetzen.
- Correlation 126: spec_+corr_ 32 kB
- ProcessingTask-Stack 16 kB (FreeRTOS-Heap prüfen)

## Laufzeit pro Frame (F746, grob)
8 × RFFT 4096 ≈ 2 ms · 28 Paare × (Spektrum-Produkt + IFFT) ≈ 28 × 0,35 ms ≈ 10 ms · Rest < 1 ms
→ ~13 ms pro 64-ms-Frame; mit 50 % Overlap später ~40 % Last.

## Offene Punkte
1. HBD-Parameter (HBD_BAND_SNR_DB, HBD_GATE_FLOOR, perBandSnrDb) mit Aufnahmen abstimmen.
2. Feedback-Empfang (Nachrichtentyp 4 in USBTask → Output_Interface_130::pollFeedback).
3. PC-Monitor (Processing Module 120): Inter-Unit-GCC-PHAT auf den Referenzkanal-Spektren ≥ [3]
   Einheiten (Patent Abschnitt 1/6; Werte in Klammern noch offen) mit `126::crossCorrelate()` und
   `128::solve()` als Referenzcode. Voraussetzung: gemeinsame Zeitbasis ([10 µs]) und Übertragung der
   Referenzspektren im UnitReport. Bis dahin: Schnitt der Peilstrahlen ≥ 2 Einheiten.
4. Azimut-Kalibrierung (alt: +12°, ×0.98) nach Messung über `setCalibration()` setzen.
5. Linker-Sektion `.sdram_data` in STM32F746NGHX_FLASH.ld.
