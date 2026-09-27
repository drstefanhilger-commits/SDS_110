# SDS_110 – Traceability Report FSL9

Stand 27.09.2026. Online-Fassung mit änderbarem Status und Diagramm:
[Traceability Report FSL9](https://claude.ai/code/artifact/28cd8a5a-fb43-4f4b-8f02-fa667071683c).

## Zusammenfassung

Von 36 zugesicherten Werten aus FSL9 sind 17 erfüllt, 8 teilweise erfüllt, 7 nicht erfüllt, 2 offen und 2 nicht im Umfang der Firmware. Die Signalkette 118 → 122 → 126 hält die Parameter der bevorzugten Ausführung weitgehend ein: Frames, FFT, Bänder, Selektion, Gewichtung, GCC-PHAT und Peak-Ratio. Die Abweichungen liegen vor allem im Systemaufbau und in den Zeitvorgaben:

- **Eine statt mindestens zwei Sensoreinheiten** (`NUM_UNITS = 1`). Die Distanz kommt deshalb aus einem Pegelmodell, das FSL9 nicht vorsieht; Multilateration und der Zwei-Unit-Modus werden nicht genutzt.
- **Keine Synchronisation nach IEEE 1588 und keine UTC-Zeit.** Der Report überträgt die Zeit in ms statt µs.
- **Rechenzeit:** Das Processing Module braucht am Board 37 ms je Frame, der Frame-Takt ist 32 ms. Die zugesicherten ~30 Reports/s werden deshalb nicht erreicht.
- **ML-Modul:** s(t) kommt vom HBD. Das trainierte MLP läuft nur im Schatten und ist kein CNN. Die Labels folgen dem Band-SNR statt der Harmonischen aus Telemetrie.

Die Tracking-Einheit (Abschnitte 8–9, Ansprüche 6–8 und 11) ist nicht Teil von SDS_110.

## Grundlage und Methode

Geprüft wurde die Firmware SDS_110 `master` Commit `d046577` (A3 nachgeführt nach der Umstellung auf 200 mm) gegen `doc/Stefan_FSL9.docx`: Abschnitte 1–12 (bevorzugte Ausführung) und Ansprüche 1–5 und 10 (Detektionssystem). Die Anforderungs-IDs A1–A36 sind in diesem Report vergeben; FSL9 selbst hat keine IDs.

| Nachweisart | Bedeutung |
| --- | --- |
| Code | am Quelltext nachvollzogen, Referenz `Datei:Zeile` relativ zu `Core/SDS_110/` |
| Host-Test | gemessen mit `make -C test/host check` bzw. `measure` (Simulator, x86) |
| Board | gemessen am STM32F746G-DISCO, Debug-Build, Simulationsbetrieb |

| Status | Bedeutung |
| --- | --- |
| Erfüllt | Wert wie in FSL9 umgesetzt |
| Teilweise | umgesetzt, aber mit Abweichung im Detail |
| Nicht erfüllt | Wert fehlt oder weicht grundlegend ab |
| Offen | noch nicht geprüft oder Klärung nötig |
| Nicht im Umfang | gehört nicht zur Firmware SDS_110 |

Nicht geprüft wurde der Hardware-Pfad mit echten Mikrofonen. Er ist durch die Blocker 2–4 in `doc/Analyse_Befunde.md` gesperrt.

## Traceability-Matrix

Je Zeile ein zugesicherter Wert aus FSL9 mit Ist-Wert, Code-Referenz (relativ zu `Core/SDS_110/`) und Status.

| ID | FSL9 | Anforderung | Zugesichert | Ist | Code-Referenz | Status |
| --- | --- | --- | --- | --- | --- | --- |
| A1 | §1, Anspr. 1/10 | Anzahl Sensoreinheiten | N ≥ 2 (Multilateration ab 3) | 1 Einheit | `SDS_110_Config.hpp:12` | Nicht erfüllt |
| A2 | §1 | Mikrofone je Einheit | M = 8, regelmäßiges Oktagon | 8, Oktagon | `SDS_110_Config.hpp:13, Sensor_Unit_112/Microphone_Array_114.cpp:23` | Erfüllt |
| A3 | §1 | Arraydurchmesser | 200 mm | 200 mm (Radius 0,10 m, seit 27.09.2026; vorher 400 mm) | `SDS_110_Config.hpp:38` | Erfüllt |
| A4 | §1 | Abtastung | PDM, 48 kHz, synchron | 48 000 Hz; Board 47 991 Hz (186 ppm, Befund 5) | `SDS_110_Config.hpp:14, Sensor_Unit_112/Sampling_Circuitry_116.cpp configureSai()` | Erfüllt |
| A5 | §1 | Vorverarbeitung 118 | AGC, Bandpass 80 Hz–8 kHz, adaptive Rauschunterdrückung | Butterworth-HPF/LPF 80–8000 Hz, NS, AGC | `SDS_110_Config.hpp:15–16, Sensor_Unit_112/Pre_Processor_118.hpp:5–12` | Erfüllt |
| A6 | §1 | Synchronisation der Einheiten | IEEE 1588 PTP, ≤ 10 µs | DWT-Laufzeit in µs, kein PTP, kein UTC | `Infrastructure/Utils/TimeBase.hpp:9–10` | Nicht erfüllt |
| A7 | §1 | Positionen der Einheiten | vermessen auf 0,1 m, gespeichert | ein Referenzpunkt (0, 0, 0) | `Processing_Module_120/Processing_Module_120.cpp:37` | Nicht im Umfang |
| A8 | §2 | Frames | 64 ms, 50 % Überlappung | 3072 Samples, Hop 1536 (Befund 17) | `SDS_110_Config.hpp:20, 28–29` | Erfüllt |
| A9 | §2 | STFT | 4096 Punkte | N_FFT = 4096, Hann-Fenster | `SDS_110_Config.hpp:21, Feature_Extraction_Module_122.cpp:65` | Erfüllt |
| A10 | §2 | Frequenzbänder | B = 64, Δf = 62,5 Hz, 80 Hz–4 kHz | 64 Bänder, 62,5 Hz, 80–4000 Hz | `SDS_110_Config.hpp:23–26, Feature_Extraction_Module_122.cpp:53` | Erfüllt |
| A11 | §2 | Referenzkanal | Mikrofon 1 | REF_MIC = 0 (erstes Mikrofon) | `SDS_110_Config.hpp:27` | Erfüllt |
| A12 | §2 | Merkmale | log-Power je Band, MFCC Ordnung 13, spektraler Fluss, AM-Spektrum über 0,5 s | log-Power je Band, Log-Mel 40 statt MFCC 13, Fluss, AM-Tiefe (std/mean) über 0,5 s statt Spektrum | `Feature_Extraction_Module_122.cpp:83–138, SDS_110_Config.hpp:73, 77` | Teilweise |
| A13 | §3 | Modell | CNN mit B = 64 Sigmoid-Ausgängen | s(t) vom HBD; MLP 845→48→64→64 (Sigmoid) nur im Schatten | `Machine_Learning_Module_124/ML124_Config.hpp:21, Machine_Learning_Module_124.cpp:34, 130` | Teilweise |
| A14 | §3, Anspr. 3 | Training | gleiche Hardware, UAVs 20–200 m, harmonische Negative, Label je Band aus Harmonischen ±½ Band, BCE | synthetische Drohnen + Umweltaufnahmen, Label σ(SNR_b / 3 dB), BCE | `ML_Test app/train_ml124/ml124_data.py, train.py` | Teilweise |
| A15 | §3 | Generalisierung | Hold-out-AUC je Band 0,65–0,95 für nicht trainierten Typ | Modell ohne 6-Rotor-Drohnen vorhanden, Wert hier nicht geprüft | `ML_Test model/ml124/k5_h48_d3_ohne6rot` | Offen |
| A16 | §3 (optional) | Glättung s(t) | 3 Frames | 6 Frames (0,19 s) | `SDS_110_Config.hpp:80, Machine_Learning_Module_124.cpp:99` | Teilweise |
| A17 | §4, Anspr. 4 | Selektionsschwelle | θ_sel = 0,5 | 0,5 | `SDS_110_Config.hpp:83, Correlation_Processing_Module_126.cpp:60` | Erfüllt |
| A18 | §4 | Mindestzahl Bänder | weniger als B_min = 3 Bänder: kein TDOA | füllt auf die 3 stärksten Bänder auf und berechnet TDOA; Report nur bei ≥ 3 Bändern über θ_sel | `Correlation_Processing_Module_126.cpp:70–76, Infrastructure/Model/SDS_Data.cpp:33, Processing_Module_120.cpp:110` | Teilweise |
| A19 | §4, Anspr. 4 | Gewicht | g(p) = p | p^γ mit γ = 1, × Feedback-Faktor | `SDS_110_Config.hpp:85, Correlation_Processing_Module_126.cpp:82` | Erfüllt |
| A20 | §4, Anspr. 2 | Gemeinsame Selektion | einmal abgeleitet, für alle Einheiten gleich | eine Selektion für alle 28 Paare je Frame | `Correlation_Processing_Module_126.cpp:218 (prepareBins einmal je Frame)` | Erfüllt |
| A21 | §5, Anspr. 1(e) | GCC-PHAT | nur ausgewählte Bins, gewichtet, übrige 0 | Schnellpfad ±32 Lags oder IFFT, gleiche Formel (t_gcc_direct) | `Correlation_Processing_Module_126.cpp:118–216` | Erfüllt |
| A22 | §5 | Lag-Bereich | τ bis ±d_ij / c je Paar | für alle Paare ±(größter Mikrofonabstand / c) × 1,1 | `Correlation_Processing_Module_126.cpp:28` | Teilweise |
| A23 | §5 | Schallgeschwindigkeit | temperaturkorrigiert | fest 343 m/s | `SDS_110_Config.hpp:90` | Nicht erfüllt |
| A24 | §5 | Interpolation | Parabel um das Maximum | Parabel | `Correlation_Processing_Module_126.cpp:201` | Erfüllt |
| A25 | §5 | Peak-Ratio | verwerfen unter 1,5 | PEAK_RATIO_MIN = 1,5 | `SDS_110_Config.hpp:88, Correlation_Processing_Module_126.cpp:209` | Erfüllt |
| A26 | §5 | Intra-Unit-Peilung | Kreuzkorrelation der Mikrofone, kein Beamforming | TDOA-Least-Squares über 28 Paare | `Correlation_Processing_Module_126.cpp:218–270` | Erfüllt |
| A27 | §6, Anspr. 1(f) | Lokalisation | N ≥ 3 Multilateration, N = 2 TDOA + zwei Peilungen | N = 1: Peilung + Pegel-Distanz (nicht in FSL9); Multilateration vorhanden, ungenutzt | `Localisation_Module_128.cpp:26, 67, SDS_110_Config.hpp:104` | Nicht erfüllt |
| A28 | §6, FIG. 5 | Referenzpunkt, Azimut | Zentroid der Einheiten, Azimut ab Nord | Arraymitte; Azimut = atan2(uy, ux), also ab x-Achse | `Correlation_Processing_Module_126.cpp:263, Processing_Module_120.cpp:37` | Offen |
| A29 | §7, Anspr. 1(g) | Zeitstempel im Report | UTC, µs | intern µs (uint64), gesendet ms (uint32), kein UTC (Befund 33) | `Data_Interface_140/Candidate_Report_140.hpp:32, Output_Interface_130.cpp:37` | Nicht erfüllt |
| A30 | §7, Anspr. 1(g) | Inhalt des Reports | φ, r, Qualität (Paare, Residuum), ausgewählte Bänder + p_b | UnitReport: Peilung, Residuum, Paare, Pegel, Bänder + p_b; φ/r im CandidateReport auf dem PC | `Data_Interface_140/Candidate_Report_140.hpp:30–41` | Teilweise |
| A31 | §7 | Report-Rate, Format | ≈ 30 Reports/s, feste Binärstruktur | Binärstruktur 128 Byte (t_unit_report); Proc 37 ms > 32-ms-Frame-Takt | `Output_Interface_130.cpp, LCDTask.cpp (Zeitanzeige)` | Nicht erfüllt |
| A32 | §7, Anspr. 10 | Keine Trajektorie im SDS | SDS bildet keine Trajektorie | keine Tracking-Funktion in der Firmware | – | Erfüllt |
| A33 | §10 | Feedback Schwelle/Gewicht | ŝ_b > 0,6: θ = 0,3, Gewicht × (1 + ŝ_b) | wie gefordert; wird nie zurückgesetzt (Befund 34) | `SDS_110_Config.hpp:129–130, Correlation_Processing_Module_126.cpp:49–55` | Erfüllt |
| A34 | §10 | Feedback Suchfenster | TDOA-Fenster ±2 ms um die Vorhersage | TDOA_WINDOW_S definiert, ungenutzt | `SDS_110_Config.hpp:131` | Nicht erfüllt |
| A35 | §11 | Ohne UAV | keine Selektion, keine Korrelation, kein Report | kein Report (0 % bei Rauschen, m_selection); Korrelation läuft wegen A18 immer | `Correlation_Processing_Module_126.cpp:70` | Teilweise |
| A36 | §8–9, Anspr. 6–8, 11 | Tracking-Einheit | ŝ mit α = 0,2, Kosinus > 0,7, χ²-Gate, 3 Reports, 2 s | nicht Teil von SDS_110 | – | Nicht im Umfang |

## Rechenzeit und Ressourcen am Board

FSL9 sichert ≈ 30 Reports/s zu, also einen Frame je 32 ms (Hop 1536 Samples bei 48 kHz). Das Processing Module braucht dazu noch 5 ms zu viel; mit dem Simulator sind es 18 ms.

Board-Messung STM32F746G-DISCO, LCD-Zeitzeilen, 27.09.2026 (master `d046577`, Debug-Build, Simulationsbetrieb):

| Stufe | ms je Frame |
| --- | --- |
| P – 118 + Analysefenster | 7 |
| F – 122 (8 FFT + Merkmale) | 15 |
| M – 124 (HBD + MLP im Schatten) | 4 |
| K – 126 Selektion | 0,1 |
| K – 126 GCC (28 Paar-Korrelationen) | 8,5 |
| K – 126 SRP-Referenzscan | 2,3 |
| **Processing Module gesamt** | **36,9** |
| Simulator (nur Testbetrieb) | 13,4 |
| **mit Simulator** | **50,3** |

Mit echten Mikrofonen fällt der Simulator weg, das Budget muss also das Processing Module allein einhalten. Ausgangswert vor der Optimierung war 380 ms je Hop. Der Release-Build (`-Os`) ist noch nicht gemessen.

| Ressource | Belegt | Verfügbar |
| --- | --- | --- |
| Flash | 489 kB (46,7 %) | 1 MB |
| RAM (DTCM + SRAM1) | 288 kB (92,7 %) | 304 kB |
| SDRAM | 390 kB (6,2 %) | 6 MB (+ 2 MB Framebuffer) |

FSL9 nennt keine Grenzen für Speicher. Der interne RAM ist durch die Verlagerung von 122, 124 und 126 weitgehend belegt.

## Abweichungen und nächste Schritte

Zuerst anzugehen ist die Rechenzeit. Der Arraydurchmesser ist seit 27.09.2026 auf 200 mm umgestellt (A3 erfüllt). Die übrigen Punkte hängen an der Hardware (mehrere Einheiten, PTP) oder sind kleine Code-Änderungen.

| Prio | IDs | Abweichung | Maßnahme | Bezug |
| --- | --- | --- | --- | --- |
| 1 | A31 | Proc 37 ms > 32 ms | Release-Build messen; SRP-Referenzscan abschaltbar machen (2,3 ms); 122 und 118 weiter prüfen | Befund 28, `doc/Host_Tests.md` |
| 2 | A18, A35 | TDOA auch bei < 3 Bändern | bei weniger als B_min Bändern keine Korrelation rechnen, wie FSL9 §4 | `Correlation_Processing_Module_126.cpp:70` |
| 3 | A29, A6 | Zeit in ms, kein UTC, kein PTP | Zeitstempel in µs übertragen; UTC-Bezug über PTP oder GNSS-PPS | Befunde 16, 33 |
| 4 | A1, A7, A27 | eine Einheit, Pegel-Distanz | Multilateration mit N ≥ 3 Einheiten, wenn die Hardware vorliegt; Positionen der Einheiten konfigurieren | Blocker 1–4 |
| 5 | A22, A23, A34 | Lag-Fenster global, c fest, Fenster aus Feedback ungenutzt | Fenster je Paar aus d_ij; c aus Temperatur; `TDOA_WINDOW_S` um die Vorhersage anwenden | Befunde 26, 34 |
| 6 | A33 | Feedback wird nie zurückgesetzt | Feedback nach Zeitablauf (z. B. 2 s ohne Report) löschen | Befund 34 |
| 7 | A12–A16 | Merkmale, Modell, Labels, Glättung weichen ab | entscheiden, ob die Abweichungen als Variante nach §12 gelten; sonst MFCC 13, CNN und Harmonischen-Labels umsetzen | `doc/Vergleich_HBD_ML124.md` |
| 8 | A15 | Hold-out-AUC nicht geprüft | Auswertung des Modells ohne 6-Rotor-Drohnen in ML_Test gegen 0,65–0,95 prüfen | ML_Test `docs/Training_ML124.md` |
| 9 | A28 | Azimut ab x-Achse statt ab Nord | Konvention mit dem PC-Programm und der Tracking-Einheit abstimmen | `Correlation_Processing_Module_126.cpp:263` |

Die Befundnummern beziehen sich auf `doc/Analyse_Befunde.md`.
