# SDS_110 – Offene Punkte

Stand 27.09.2026

Alle offenen Punkte aus `doc/Analyse_Befunde.md`, `doc/Analyse_27_09_2026.md` und dem Trainingskonzept ML124, nach Bereich. Status und Priorität werden im Online-Dokument gepflegt; diese Datei ist ein Abzug vom 27.09.2026.

## Firmware ohne Board

Alles hier lässt sich auf dem Host umsetzen und mit `test/host` belegen.

| Befund | Aufgabe | Priorität | Status |
| --- | --- | --- | --- |
| 26 | Peilung bei f0 über 400 Hz absichern (SRP-Vorbelegung oder Konsistenzprüfung), Host-Test für f0 = 120…650 Hz | hoch | Offen |
| 27 | Lücken im Hop-Strom erkennen (`nextId_` im Verwerf-Pfad erhöhen), Test über `pushBlock()` mit vollem Puffer | hoch | Offen |
| 35 | READ-Modus: Rohdaten vor 118 senden, alle bereiten Hops senden, Hop-Nummer im Kopf | hoch | Offen |
| 29 | `SDS_Data`-Getter: Sperr-Timeout nicht als Wert 0 zurückgeben | hoch | Offen |
| 30 | Start von SAI/Verarbeitung nur nach erfolgreicher Initialisierung | mittel | Offen |
| 28 | Simulation und READ an den 32-ms-Hop koppeln (alle fälligen Hops je Durchlauf) | mittel | Offen |
| 31 | Beim Wechsel Simulation ↔ Hardware Hop-Puffer und Frame_Assembler zurücksetzen | mittel | Offen |
| 32 | Mehrere oder geteilte Kommandos je USB-Paket auswerten | mittel | Offen |
| 33 | UnitReport-Zeitstempel in µs übertragen | mittel | Offen |
| 12 | CRC der USB-Kommandos prüfen | mittel | Offen |
| 16 | Inter-Unit-Zeitbasis (GNSS-PPS oder PTP), `syncTimeDifference` anwenden | mittel | Offen |
| 34 | Feedback nach Ausbleiben zurücksetzen (`clearFeedback()`), x̂ und `TDOA_WINDOW_S` nutzen | niedrig | Offen |
| 34 | TX-Ring beim Trennen leeren; Kopf von `sendLogging()` angleichen | niedrig | Offen |
| 8 | Peak-Ratio in `srpScan()` wie in `crossCorrelate()` berechnen | niedrig | Offen |

## ML-124 (Trainingskonzept, ML_Test)

Arbeitspakete (AP) nach `docs/Trainingskonzept_ML124.md` § 9, dazu die Befunde 36–38. AP 1, 3 und 4 sind erledigt.

| Bezug | Aufgabe | Priorität | Status |
| --- | --- | --- | --- |
| ⚠ PRÜFEN | Freigabe der Drohnendaten fürs Training prüfen (Prüfpunkte 1–4 in `docs/Daten_ML124.md`) | hoch | Offen |
| AP 2 | Inventur echter Drohnen abschließen; saubere echte Aufnahmen für die Mischung beschaffen | hoch | Offen |
| 38 | Synthetische Drohnen bis BPF ca. 700 Hz erweitern, Datensatz neu erzeugen und neu trainieren | hoch | Offen |
| AP 5 | Export `k5_h48_d3` als Header mit Referenzvektoren und Merkmalsversion | hoch | Offen |
| AP 6 | Inferenz in 124, Umschalter HBD/ML, Host-Test `t_ml124` | hoch | Offen |
| 36 | `HBD_ML_Model_Data.hpp` löschen und Befund 20 schließen | mittel | Offen |
| 37 | HBD-Vergleich ab Frame 94 wiederholen (nach der HBD-Anlaufzeit) | mittel | Offen |
| AP 7 | Vergleich HBD ↔ ML mit `m_selection` und `m_bearing_drone`, f0 bis 650 Hz | mittel | Offen |
| AP 8 | Board-Aufnahmen und Endabnahme (setzt Befund 35 und Blocker 1–4 voraus) | mittel | Offen |
| 38 | Validierung: Umwelt-Clips getrennt vom Training ziehen | niedrig | Offen |
| Doku | Merkmalshash auf einen Header mit den Merkmalskonstanten begrenzen | niedrig | Offen |
| Lizenz | ESC-50 (CC BY-NC 3.0) vor einer kommerziellen Verwertung klären oder ersetzen | niedrig | Offen |

## Hardware und Messungen am Board

Die Blocker 1–4 stehen in `doc/Analyse_Befunde.md` auf „nicht bearbeiten“ und sind hier als zurückgestellt geführt. Ohne sie läuft auf dem Board keine Verarbeitung; alle Messungen darunter setzen sie voraus.

| Befund | Aufgabe | Priorität | Status |
| --- | --- | --- | --- |
| 1 | ProcessingTask starten (`main.c` Z. 270) | hoch | Zurückgestellt |
| 2 | SAI-DMA einrichten (MspInit, Streams, IRQ-Handler) | hoch | Zurückgestellt |
| 3 | CubeMX an die eigene Platine anpassen: SAI1 PE4/PE5, I2C2, PE3; SAI-Takt aus PLLI2S, SPDIFRX aus | hoch | Zurückgestellt |
| 4 | ADAU7118-Registertabelle und I2C-Adresse (0x4B oder 0x3A) klären | hoch | Zurückgestellt |
| 5 | Abtastrate messen: FSYNC 47,991 kHz, BCLK 12,286 MHz | mittel | Offen |
| 6, 34 | Bitlage im 32-Bit-Slot und SAI-Taktflanke am Oszilloskop prüfen | mittel | Offen |
| 17 | Rechenlast mit 50-%-Überlappung messen (Task-Statistik; ML ≤ 2 ms je Frame) | mittel | Offen |
| 9 | `LEVEL_DIST_K_REF` mit realer Drohne in bekanntem Abstand kalibrieren | niedrig | Offen |
| 23 | Azimut-Kalibrierung nach der Vorzeichenkorrektur neu messen | niedrig | Offen |
| 21 | Leistungsgewinn durch gecachten SRAM1 messen | niedrig | Offen |

## Dokumentation und Klärungen

| Bezug | Aufgabe | Priorität | Status |
| --- | --- | --- | --- |
| Repo | `doc/Analyse_27_09_2026.md` ins Repo bringen (Push-Rechte einrichten oder Patch mit `git am`) | hoch | Offen |
| Doku | Befunde 26–38 in `doc/Analyse_Befunde.md` aufnehmen | mittel | Offen |
| Doku | Veraltete Stellen korrigieren: Kommentare in `ProcessingTask.hpp` und `Frame_Assembler.hpp`, Befunde 6, 9, 20, `SPEED_OF_SOUND`, Trainingskonzept Frage 6 | mittel | Offen |
| 22 | Nachrichtentyp für das Tracking-Feedback festlegen (4 oder 6) | mittel | Offen |
| 25 | Moduswerte in `PC_Monitor_Test.ptp` gegen den PC-Monitor prüfen und angleichen | mittel | Offen |
| Konzept | Offene Fragen 1–5 im Trainingskonzept klären (Patent-Vorgaben Abschnitt 3, Datenlage, Board-Aufnahmen, ML ersetzt oder ergänzt HBD, Framework) | mittel | Offen |
| 9 | PC-Monitor: neue Skala des UnitReport-Felds `level` berücksichtigen | niedrig | Offen |
| Doku | `doc/ADUA_Design.md` korrigieren oder mit Hinweis auf Befund 4 versehen | niedrig | Offen |
| Doku | `doc/Test & Integration.md` füllen oder entfernen | niedrig | Offen |
