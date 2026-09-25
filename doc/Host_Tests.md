# SDS_110 – Host-Tests

Sammlung aller Tests, die bei der Code-Analyse und der Bearbeitung der Befunde
(`doc/Analyse_Befunde.md`) verwendet wurden. Sie laufen unter Linux/WSL2 mit `g++` –
ohne Board, ohne HAL, ohne RTOS.

Referenzergebnisse: Stand 25.09.2026, Code-Stand Commit `0dc8150`.

---

## 1. Überblick

| Art | Programm | Bezug (Befund) | Inhalt |
|---|---|---|---|
| Prüfung | `t_scaling` | 6 | Rohdaten-Skalierung in 114 (24 Bit linksbündig) |
| Prüfung | `t_frame_assembler` | 17 | Analysefenster mit 50 % Überlappung |
| Prüfung | `t_unit_report` | 10 | Serialisierung des UnitReport (128 Byte) |
| Prüfung | `t_sdram_selftest` | 14 | Logik des SDRAM-Selbsttests |
| Prüfung | `t_timebase` | 16 | 64-bit-Erweiterung des DWT-Zählers |
| Prüfung | `t_logger` | 19 | Logger-Ringpuffer mit mehreren Schreibern |
| Prüfung | `t_bearing_broadband` | 23, 8 | Peilung einer breitbandigen Quelle, 24 Richtungen |
| Messung | `m_overview` | 7, 8, 23, 24 | Alle Simulator-Szenarien + Empfindlichkeit |
| Messung | `m_hbd_diag` | 7 | HBD-Rauschboden und SNR je Harmonischer |
| Messung | `m_bearing_drone` | 8, 17, 23, 24 | Peilung eines Drohnensignals über die volle Kette |
| Messung | `m_selection` | 24 | Band-Selektion, Detektion, Reports, Fehlalarme |
| Messung | `m_distance` | 9, 17 | Distanzschätzung aus dem Pegel |
| Messung | `m_confidence` | 18 | Konfidenz und Peilresiduum |
| Messung | `m_longrun` | 7, 17 | Langzeitverhalten des HBD-Rauschbodens (~5 min) |

- **Prüfungen (`t_*`)** haben ein festes Kriterium und liefern Exit-Code 0 (bestanden) oder ≠ 0.
- **Messungen (`m_*`)** geben Tabellen aus; sie werden mit den Referenzwerten in Abschnitt 6
  verglichen. Sie dienen zum Abstimmen von Parametern und zum Erkennen von Rückschritten.

---

## 2. Aufruf

```bash
make -C test/host              # alle Programme bauen (Ausgabe: build/test_host/)
make -C test/host check        # alle Prüfungen, Abbruch mit Fehler, wenn eine fehlschlägt
make -C test/host measure      # alle Messungen (einige Minuten)
make -C test/host longrun      # Langzeitmessung (einige Minuten)
make -C test/host clean
make -C test/host gegenprobe TEST=t_name CODE=/pfad   # siehe unten
```

Einzelne Programme direkt starten, z. B. `build/test_host/m_bearing_drone 0`
(Argumente siehe Abschnitt 5 bzw. Kopfkommentar der Quelldatei).

Voraussetzungen: `g++` mit C++17, `make`. Die Firmware-Toolchain wird nicht benötigt.

### Gegenprobe gegen einen älteren Stand

Mit `CODE=…` übersetzt das Makefile die aktuellen Tests gegen einen anderen Code-Stand.
So lässt sich zeigen, dass ein Test den Fehler vor der Korrektur tatsächlich erkennt:

```bash
git worktree add --detach /tmp/sds_alt ad18673~1
make -C test/host gegenprobe TEST=t_scaling CODE=/tmp/sds_alt
git worktree remove --force /tmp/sds_alt
```

Die Gegenprobe baut nach `build/test_host_gegenprobe/`. Erwartet wird, dass der Test gegen den
alten Stand fehlschlägt (make meldet dann einen Fehler).

Ergebnisse (Stand vor der jeweiligen Korrektur → Test schlägt fehl, aktueller Stand → besteht):

| Test | Stand | Ergebnis |
|---|---|---|
| `t_scaling` | `ad18673~1` (vor Befund 6) | Vollaussteuerung ±256 statt ±1 – FEHLER |
| `t_unit_report` | `d572820~1` (vor Befund 10) | 57/64 Bänder → nsel 57/64, 130/144 Byte – FEHLER |
| `t_logger` | `7da4097~1` (vor Befund 19) | 256 Byte mit Nullbyte; erste Meldung defekt – FEHLER |

Grenzen: Tests, die die Hop-Kette aus Befund 17 benutzen (`t_bearing_broadband`, alle `m_*`),
übersetzen gegen Stände vor Commit `05b3d3d` nicht (API `generateHop`, `Frame_Assembler`).
Für die Gegenprobe zu Befund 23 wurde damals eine Vorversion ohne Hop-Kette verwendet
(Ergebnis: 180° Fehler in allen 24 Richtungen). `t_timebase`, `t_sdram_selftest` und
`t_frame_assembler` testen Code, der mit der Korrektur neu entstand; Gegenprobe für
`t_timebase` siehe Abschnitt 4.

---

## 3. Aufbau

```
test/host/
  Makefile
  shim/        Stubs: arm_math.h (CMSIS-DSP-Teilmenge), cmsis_os2.h (RTOS, einfädig),
               arm_rfft.cpp (arm_rfft_fast_f32 über kiss_fft, CMSIS-Packing, IFFT mit 1/N),
               sds_stubs.cpp (USBDriver/SDS_Data für Output_Interface_130; fängt Nachricht id 4 ab)
  shim_hal/    stm32f7xx_hal.h – SDRAM-Handle und SCB_CleanInvalidateDCache (zählt Aufrufe)
  shim_irq/    stm32f7xx.h – PRIMASK-Sperre als globaler Mutex (für echte Threads im Logger-Test)
  common/      chain.hpp – Simulator → 114 (Hop) → 118 → Frame_Assembler, wie Sensor_Unit_112::nextFrame()
  t_*.cpp      Prüfungen
  m_*.cpp      Messungen
```

Getestet wird der **unveränderte** Projektcode aus `Core/SDS_110` (Module 114, 118, 122,
124/HBD, 126, 128, 130, Signal_Simulator, Frame_Assembler, TimeBase, Logger, SDRAMSelfTest).
Die Signalquelle ist `Harness/Signal_Simulator` im Hardware-Rohformat.

**Was die Host-Tests nicht abdecken:** HAL, DMA, Interrupt-Prioritäten, Cache und MPU,
FreeRTOS-Scheduling, USB-Übertragung, Rechenzeit auf dem Cortex-M7, echte Mikrofone.
Die FFT kommt aus `kiss_fft` statt aus CMSIS-DSP (numerisch gleichwertig, nicht bitgleich).

---

## 4. Prüfungen (`t_*`)

### t_scaling – Befund 6
Speist Rohwerte im SAI/ADAU7118-Format (`pcm24 << 8`) über `Microphone_Array_114::pushBlock()`
ein und prüft den float-Wert im Puffer.
Kriterium: ±Vollaussteuerung, +0,5, −0,25, 1 LSB und 0 exakt (relativ 1e-6).
Referenz: 6/6 OK. Schiebt Blöcke nach, bis ein 114-Puffer fertig ist – funktioniert daher
mit Hop- (ab Befund 17) und Frame-großen Puffern.

### t_frame_assembler – Befund 17
Prüft `Frame_Assembler::push()` mit markierten Hops.
Kriterium: nach Hop 5 nicht voll; nach Hop 6 Frame [5|6] mit Zeitstempel von Hop 5; nach Hop 7
[6|7], `frame_id` fortlaufend; Hop 9 nach Lücke → Neubeginn; Hop 10 → [9|10]; `reset()`.
Referenz: 7/7 OK.

### t_unit_report – Befund 10
Serialisiert UnitReports mit 0, 3, 8, 56, 57, 64 Bändern über `Output_Interface_130::send()`
(USB abgefangen).
Kriterium: nsel = min(Bänder, 56), Bandindizes und -wahrscheinlichkeiten innerhalb 128 Byte.
Referenz: 6/6 OK.

### t_sdram_selftest – Befund 14
`sdramSelfTest()` mit HAL-Stub auf einem Speicherfeld von 591 kB + Wächterwörtern.
Kriterium: Handle nicht READY → false ohne Speicherzugriff; intakter Bereich → true, 20
Prüfadressen, 2 Cache-Flushes, Wächter unberührt; leerer Bereich → true.
Nicht abgedeckt: defektes SDRAM, echtes Cache-Verhalten.

### t_timebase – Befund 16
`CycleExtender` (216 000 Zyklen/ms) in vier Szenarien: DMA-Takt 2,67 ms über 10 min;
zufällige Abstände bis 15 s; Pausen 18–60 s; Pausen bis 1 h; Tick bis 1 ms verzögert.
Kriterium: monoton, max. Fehler < 1 µs. Referenz: alle 0,000 µs.
Gegenprobe: In einer Kopie von `TimeBase.hpp` die Korrekturzeile
`if (expected > d + (1ull << 31))` durch `if (false && …)` ersetzen und mit `-I` auf die Kopie
übersetzen – die Pausen-Szenarien schlagen dann fehl (Fehler im Bereich von Stunden).

### t_logger – Befund 19
(1) 300-Zeichen-Meldung → 255 Byte ohne Nullbyte. (2) 4 Schreib-Threads × 20 000 Meldungen und
1 Leser parallel.
Kriterium: 0 defekte, 0 vertauschte Meldungen je Schreiber, empfangen + verworfen = gesendet.
Referenz: gesendet 80 000, empfangen ≈ 30 000, verworfen ≈ 50 000 (gewollt: Schreiber schneller
als Leser), 0 defekt, 0 vertauscht.

### t_bearing_broadband – Befunde 23, 8
Breitbandige Quelle (Wind-Szenario als Punktquelle), alle Bänder selektiert, 24 Richtungen
0…345°; TDOA-LS-Peilung (`estimateBearing`) und SRP-Scan.
Kriterium: 24/24 gültig, max. Fehler < 0,5°. Referenz: max. 0,09° (TDOA-LS), 0,07° (SRP).

---

## 5. Messungen (`m_*`) und Referenzwerte

Frame-Angaben in den Messprogrammen: Seit Befund 17 ist ein Analyse-Frame 32 ms; die Argumente
von `m_overview` und `m_selection` werden in „alten“ 64-ms-Frames angegeben und intern verdoppelt.
Der HBD entscheidet nach dem Start 3 s lang nicht (Befund 24) – Messungen, die früher beginnen,
zeigen deshalb weniger als 100 % Detektion.

### m_overview – Übersicht (Aufruf: `m_overview [Frames, Standard 120]`)
Je Szenario (SNR 20 dB): HBD-DRONE-Anteil, detected (≥ 3 Bänder > θ_sel), Bänder, f0, Score,
mittlerer Floor, gültige Peilungen, Median-Fehler; danach DroneStatic 30…−6 dB.
Referenz (Standardaufruf, Messung ab 1,9 s – enthält die HBD-Anlaufzeit):

| Szenario | HBD DRONE | detected | Peilung | Fehler |
|---|---|---|---|---|
| DroneSweep / DroneStatic | 81 % | 81 % | 100 % | 0,8° / 0,6° |
| SingleTone | 0 % | 0 % | 30 % | – |
| WindNoise | 0 % | 0 % | 81 % | – |
| Silence | 0 % | 0 % | 37 % | zufällig |

Empfindlichkeit DroneStatic (HBD DRONE): 30 dB 81 %, 10 dB 78 %, 6 dB 56 %, 3 dB 42 %,
0 dB 14 %, −3 dB 3 %, −6 dB 1 %. Für eingeschwungene Werte `m_overview 300` verwenden.

### m_hbd_diag – HBD-Rauschboden (Aufruf: `m_hbd_diag [Szenario 0..4, Standard 4]`)
Quantile von (magDb − noiseFloorDb) im Bereich 100–4000 Hz und SNR je Harmonischer.
Referenz: Silence Median +1,4 dB (5 %/95 %: −9,6 / +7,8 dB), kein DRONE;
DroneStatic Median +2,1 dB, 95 % +36,5 dB (Harmonische), SNR je Harmonischer 16–47 dB, DRONE.
Vor Befund 7 lag der Median bei rund +70 dB (Floor an −60 dB festgeklemmt).

### m_bearing_drone – Peilung Drohne (Aufruf: `m_bearing_drone [1 = Static, 0 = Sweep]`)
Volle Kette 118 → 122 → 124 → 126, 12 Richtungen, Messung ab 1,3 s.
Referenz DroneStatic:

| SNR | gültig | Paare | Ratio-Median | Fehler Median | Fehler 95 % |
|---|---|---|---|---|---|
| 30 dB | 100 % | 28,0 | 15,9 | 0,19° | 0,67° |
| 20 dB | 100 % | 28,0 | 15,2 | 0,43° | 1,28° |
| 10 dB | 100 % | 27,3 | 14,5 | 0,88° | 3,14° |
| 6 dB | 100 % | 27,2 | 15,1 | 1,17° | 4,17° |
| 3 dB | 99 % | 26,8 | 16,5 | 1,55° | 5,31° |
| 0 dB | 90 % | 23,7 | 13,4 | 2,21° | 7,52° |

DroneSweep (1° je Hop): Median 0,7–2,2°, 95 % 1,7–7,4° (der Sweep dreht innerhalb eines Frames).
Vor Befund 8 waren bei Drohnensignal 0 % der Peilungen gültig (Ratio-Median ≈ 1,17).

### m_selection – Band-Selektion (Aufruf: `m_selection [Frames] [noise]`)
6 Richtungen, Messung ab 3,2 s. „Report“ = Peilung gültig und detected (so sendet 120).
Referenz:

| Szenario | detected | HBD | Bänder | davon Harmonische | Report |
|---|---|---|---|---|---|
| Drohne 20 dB | 100 % | 100 % | 8,0 | 87 % | 100 % |
| Drohne 10 dB | 100 % | 94 % | 8,0 | 87 % | 100 % |
| Drohne 3 dB | 100 % | 52 % | 5,0 | 94 % | 100 % |
| Drohne 0 dB | 57 % | 13 % | 2,8 | 99 % | 56 % |
| Einzelton | 1 % | 0,5 % | 0,1 | – | 0 % |
| Wind | 0 % | 0 % | 0 | – | 0 % |
| Stille | 0 % | 0 % | 0 | – | 0 % |

`m_selection 650 noise`: Langlauf nur mit Rauschszenarien (Fehlalarmrate), Referenz 0 Reports.
Vor Befund 24: Wind 22 % Reports, Stille detected 68 %, Harmonischen-Anteil ~42 %.

### m_distance – Distanz (Aufruf: `m_distance`)
Simulator-Pegel ∝ 1/r, 10…200 m; levelA durch `frameCenterGain()` geteilt (wie 120), r = K/A.
Kriterium (Auswertung): Verhältnis r_geschätzt/r_wahr über alle Distanzen konstant.
Referenz: 0,518 / 0,525 / 0,522 / 0,520 / 0,524. Vor Befund 9: 0,073 … 0,024 (AGC regelte).
Der absolute Wert hängt an `LEVEL_DIST_K_REF` (unkalibriert).

### m_confidence – Konfidenz (Aufruf: `m_confidence`)
Verteilung von `candidateConfidence()` und Residuum (Samples), 6 Richtungen, ab 3,2 s.
Referenz (Median): Drohne 30/20/10/3/0/−3 dB 0,96/0,86/0,68/0,53/0,49/0,29 (Residuum
0,85…5,5 Samples); Einzelton 0,13 (9,1); Stille 0,01 (32); Wind 0,99 (0,32).
Die Konfidenz bewertet die Peilung, nicht die Drohnen-Detektion.

### m_longrun – Langzeitverhalten (Aufruf: `m_longrun [maxRise, −1 = Projektwert] [SNR]`)
DRONE-Anteil je 10 s über ~5 min bei stehender Drohne (20 dB).
Referenz (Projektwert 0,156 dB/s): 70 % (Anlaufzeit) · 100 % bis ~2 min 20 s · dann Abfall auf
~78 % (der stehende Ton wird allmählich als Rauschen gelernt).
Vergleich (damals, 64-ms-Frames): 0,05 dB/Frame → Abfall nach ~30 s; 0 → kein Abfall.

---

## 6. Einmalige Untersuchungen (nicht als Test übernommen)

Diese Programme dienten einer Entscheidung und sind nicht Teil von `test/host`;
die Ergebnisse stehen hier zur Nachvollziehbarkeit.

- **Varianten der Band-Wahrscheinlichkeiten in 124 (Befund 24)** – Kopie von
  `Machine_Learning_Module_124.cpp` mit Umschalter, SNR 20 dB, Reports:

  | Variante | Drohne 0 dB | Einzelton | Wind | Stille |
  |---|---|---|---|---|
  | V0 Ist (Gate am Score) | 67 % | 3 % | 22 % | 0 % |
  | V1 Gate = HBD-Entscheidung | 15 % | 0 % | 0 % | 0 % |
  | V2 nur Harmonische | 76 % | 2 % | 5 % | 4 % |
  | V3 nur Harmonische + Konsistenz-Gate | 48 % | 1 % | 3 % | 3 % |
  | V5 nur Harmonische + HBD-Haltezeit 16 (umgesetzt) | 69 % | 2 % | 4 % | 3 % |
  | V6 wie V5, Haltezeit 32 | 75 % | 2 % | 5 % | 4 % |

  Die Restfehlalarme von V5 stammten aus der HBD-Anlaufphase → HBD-Anlaufzeit 3 s ergänzt,
  danach 0 % (siehe `m_selection`).
- **HBD-Anlaufverhalten (Befund 24)** – letzter HBD-Treffer bei Wind/Stille nach dem Start
  bei Frame 22–29 (64-ms-Frames), beim Einzelton bis Frame 39 → Anlaufzeit 3 s.
- **GCC-PHAT-Kohärenz** (Peak / theoretisches Maximum, zu Befund 24/8) – Median Drohne 0,60,
  Stille 0,42 (95 %: 0,67): trennt nicht sauber, daher nicht als Kriterium verwendet.
- **Floor-Anstiegsrate (Befund 7)** – 0,05 / 0,01 / 0 dB je 64-ms-Frame; Grundlage für
  `HBD_FLOOR_RISE_DB_S`.

---

## 7. Neuen Test hinzufügen

1. Datei `test/host/t_name.cpp` (Prüfung, Exit-Code) oder `m_name.cpp` (Messung) mit
   Kopfkommentar (Bezug, was geprüft wird, Aufruf).
2. Für die Signalkette `#include "chain.hpp"` und `nextAnalysisFrame(sim, arr, pre)` benutzen;
   vor jedem Szenario `sim.init(...)`, `g_fa.reset()` und `init()` der Module aufrufen.
3. Im Makefile den Namen in `CHECKS` bzw. `MEASURES` eintragen; braucht der Test nur einzelne
   Module, eine eigene Regel wie bei `t_scaling` anlegen.
4. Referenzergebnis in diesem Dokument ergänzen.
