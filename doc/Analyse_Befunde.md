# SDS_110 – Befunde der Code-Analyse (25.09.2026)

Status: **nicht bearbeiten** = bewusst zurückgestellt, **offen** = zu bearbeiten,
**bearbeitet** = geändert (Datum, Commit; Testumfang steht dabei).

## Übersicht bearbeitet

| Punkt | Thema | Datum | Commit |
|---|---|---|---|
| 11 | USB-Senden über TX-Ringpuffer | 25.09.2026 | 15a87ab |
| 7 | HBD-Rauschboden / Normierung | 25.09.2026 | c8be312 |
| 23 | Peilung 180° verdreht (Vorzeichen Fernfeldmodell) | 25.09.2026 | 187441b |
| 8 | Peak-Ratio-Test (lokale Nebenmaxima statt ±3 Samples) | 25.09.2026 | 82c8fcc |
| 24 | Band-Selektion 124: nur Harmonische, Gate mit HBD-Haltezeit, HBD-Anlaufzeit | 25.09.2026 | b4a18f4 |
| 13 | FreeRTOS-Heap 64 KB, Prüfung bei Task-Anlage, Hooks halten an | 25.09.2026 | 8d3644c |
| 12 | Kommandolänge: Vorlagen auf 16, gemeinsame Konstante | 25.09.2026 | 7207038 |
| 10 | UnitReport: nsel = tatsächlich gesendete Bänder | 25.09.2026 | d572820 |
| 9 | Distanz-Pegel vor NS/AGC (angewendete Verstärkung herausrechnen) | 25.09.2026 | b81255c |
| 6 | Rohdaten-Skalierung 2⁻³¹ (24 Bit linksbündig), Simulator im Hardwareformat | 25.09.2026 | ad18673 |
| 5 | SAI-Takt aus PLLI2S (47 991 Hz statt 53 571 Hz), Prüfung der Ist-Abtastrate | 25.09.2026 | f7d4a56 |
| 14 | SDRAM-Selbsttest vor der ersten Nutzung, am D-Cache vorbei | 25.09.2026 | 8a1240d |
| 15 | Wirkungsloses SDRAM-Kommando in MX_DMA2D_Init entfernt | 25.09.2026 | c673854 |
| 16 | Zeitstempel µs-genau (DWT) und für das erste Sample des Blocks | 25.09.2026 | ea1e5b7 |
| 17 | 50-%-Überlappung: Hops in 114/118, Analysefenster (Frame_Assembler), Zeitkonstanten in Sekunden | 25.09.2026 | 05b3d3d |
| 18 | Einheitliche Konfidenz aus Paaren und Residuum in Samples (128) | 25.09.2026 | e0ca865 |
| 19 | Logger: threadsicher, Überlaufschutz, max. 255 Zeichen | 25.09.2026 | 7da4097 |
| 21 | SRAM1 gecacht, DMA-Puffer in nicht cachebarem SRAM2 (Normal statt Strongly-ordered) | 25.09.2026 | noch nicht committet |

## Blocker (Hardware-Pfad)

1. **ProcessingTask nicht gestartet** – `SDS110_StartProcessingTask()` ist in `Core/Src/main.c:270` auskommentiert; weder Verarbeitung noch Simulator laufen.
   Status: **nicht bearbeiten**
2. **SAI2 ohne DMA** – `HAL_SAI_MspInit` richtet keinen DMA ein, es gibt keine DMA-IRQ-Handler. `HAL_SAI_Receive_DMA` dereferenziert `hsai->hdmarx` ohne Prüfung (`stm32f7xx_hal_sai.c:1494`) → HardFault, sobald die Simulation per USB abgeschaltet wird.
   Status: **nicht bearbeiten**
3. **CubeMX-Konfiguration passt nicht zur eigenen Platine** – `.ioc` ist das Preset STM32F746G-DISCO (ETH, LTDC, DCMI, ULPI). Laut `STM32F746_PINS.txt` nutzt die Platine SAI1 (PE4/PE5), I2C2 (PB10/PB11), PE3 als Enable; der Code nutzt SAI2_A und I2C1. Auf dem DISCO ist PE3 der FAULT-Ausgang des STMPS2151 (`OTG_HS_OverCurrent`) und wird von `Sampling_Circuitry_116.cpp:37` als Push-Pull auf High getrieben.
   Status: **nicht bearbeiten**
4. **ADAU7118-Registertabelle vermutlich falsch** – `ADAU7118_Registers.hpp` (POWER, PLL_CTRL, MODE_CTRL …) passt weder zur Tabelle in `ADUA_Design.md` noch zur Registerbelegung des Linux-Treibers (0x00–0x03 IDs nur lesbar, 0x04 ENABLES, 0x05 DEC_RATIO_CLK_MAP, 0x06 HPF, 0x07/0x08 SPT_CTRL1/2, 0x11 DRIVE, 0x12 RESET). Schreibzugriffe auf nur lesbare Register werden per ACK bestätigt → `init()` meldet Erfolg ohne Wirkung. I2C-Adresse (0x4B oder 0x3A) offen. Designdoku §5 falsch: der ADAU7118 ist an der seriellen Schnittstelle Slave (SAI als Master ist korrekt).
   Status: **nicht bearbeiten**
5. **Abtastrate vermutlich ≈ 53,6 kHz statt 48 kHz** – SAI2 bekommt 192 MHz aus PLLSAI, nach HAL-Formel MCKDIV = 7. Nur berechnet, am FSYNC messen. Abhilfe: SAI2 aus PLLI2S takten (≈ 49,152 MHz), da PLLSAI auch USB (48 MHz) und LTDC versorgt.
   Status: **bearbeitet (25.09.2026, Commit f7d4a56)** – gerechnet und gebaut, auf dem Board nicht gemessen.
   - Bestätigt mit der HAL-Formel dieses Projekts (`stm32f7xx_hal_sai.c:508`): 192 MHz → MCKDIV 7 → 53 571 Hz (+11,6 %).
   - Durchrechnung aller zulässigen Einstellungen (PLLM = 25 fest, weil 216 MHz SYSCLK aus 25 MHz HSE nur mit M = 25 geht → 1 MHz PLL-Eingang): exakt 48 kHz ist nicht erreichbar. PLLSAI mit USB = 48 MHz exakt: bestenfalls 46 875 Hz (−2,3 %). PLLI2S N = 344, Q = 7, DivQ = 1 → 49,143 MHz → MCKDIV 2 → **47 991 Hz (−186 ppm)**, das Optimum (entspricht den ST-Audio-Beispielen).
   - `Sampling_Circuitry_116::configureSaiClock()`: stellt vor `HAL_SAI_Init` den SAI-Takt (SAI1 oder SAI2, je nach Handle) auf PLLI2S mit `SAI_PLLI2S_N/Q/DIVQ` aus `SDS_110_Config.hpp`. Bewusst im eigenen Code statt in `PeriphCommonClock_Config()` (CubeMX-generiert). Läuft nach `MX_SPDIFRX_Init()` und überschreibt dessen PLLI2S-Einstellung (SPDIFRX wird nicht genutzt).
   - Nach `HAL_SAI_Init` wird die Ist-Abtastrate aus SAI-Kerneltakt und MCKDIV berechnet (`sampleRateHz()`); Abweichung > 0,1 % (`SAI_FS_TOLERANCE`) → Fehler, `init()` liefert false.
   - Empfehlung für CubeMX (Punkt 3): SAI-Taktquelle auf PLLI2S (N 344, Q 7) stellen und SPDIFRX deaktivieren, damit Clock-Tree und Code übereinstimmen. Am Board: FSYNC messen (erwartet 47,991 kHz, BCLK 12,286 MHz).
6. **Rohdaten um Faktor 256 falsch skaliert** – 24-Bit-Samples kommen linksbündig im 32-Bit-Slot; `Microphone_Array_114.cpp:64` skaliert mit 2⁻²³ statt 2⁻³¹. Der Simulator schreibt rechtsbündig und verdeckt den Fehler.
   Status: **bearbeitet (25.09.2026, Commit ad18673)** – im Host-Test geprüft, auf dem Board nicht getestet.
   - `SDS_110_Config.hpp`: `PCM_RAW_FULL_SCALE = 2^31` mit Beschreibung des Rohformats (pcm24 << 8).
   - `Microphone_Array_114::pushBlock()`: Skalierung 1/2^31 statt 1/2^23.
   - `Signal_Simulator`: schreibt jetzt wie die Hardware linksbündig (`pcm24 << 8`), sonst wäre er nach der Korrektur um Faktor 256 zu leise.
   - Test `pushBlock()` mit Rohwerten im Hardwareformat (±Vollaussteuerung, ±0,5/−0,25, 1 LSB, 0): vorher Faktor 256 zu groß (Vollaussteuerung → ±256), nachher alle exakt. Regression: Selektions-, Peil- und Distanztest liefern identische Ausgabe wie vor der Änderung.
   - `USBDriver::sendRead()` (READ-Modus) rechnet float wie bisher auf 24 Bit rechtsbündig zurück; das Wire-Format zum PC bleibt gleich, die Werte stimmen jetzt auch mit echter Hardware.
   - Auf der Hardware zu bestätigen (mit den Punkten 2–4): Liegen die 24 Bit tatsächlich in Bits 31…8 des Slots? Das hängt von der ADAU7118-Konfiguration (Datenbreite, Verzögerung im Slot) und vom SAI-Rahmen (`FSOffset`, `FirstBitOffset`) ab.

## Signalverarbeitung

7. **HBD meldet immer „DRONE“ (berechnet)** – Rauschboden auf max. −60 dB begrenzt (`HBD.cpp:16`), Betragsspektrum aber unnormiert (Rauschbins nach AGC ≈ +10 dB) → Boden klebt bei −60 dB, SNR ≈ 70 dB überall, alle p_b ≈ 1, alle Bänder selektiert, Fehlalarm auch bei Wind/Einzelton. Abhilfe: Spektrum durch Σw normieren (1536 für Hann/3072; `windowGain` ist vorgesehen, aber ungenutzt). Danach beachten: der gleitende Mittelwert (0,94) lernt einen stehenden Drohnenton in ≈ 1 s als Rauschen.
   Status: **bearbeitet (25.09.2026, Commit c8be312)** – im Host-Test geprüft, auf dem Board nicht getestet – `HBD.cpp/.hpp`: Spektrum in dBFS normiert (`windowGain = 2/Σw`), Floor-Grenzen −140…0 dBFS, Start-Floor aus dem spektralen Umgebungsmittel, EMA je Bin; schmale Peaks (> Umgebungsmittel + 10 dB) heben den Floor nur um 0,01 dB/Frame (stehender Drohnenton wird erst nach ~2 min gelernt). Host-Test (Simulator, SNR 20 dB, 300 Frames): DRONE bei Drohne 100 %, Einzelton 3 %, Wind 0 %, Stille 0 % (vorher überall 100 %); Empfindlichkeit 81 % bei 10 dB, 38 % bei 3 dB.
   Nachtrag: Die zu großzügige Band-Selektion in 124 (`SDS_Data::detected` bei Stille 22 %, Einzelton 54 %) ist in Punkt 24 behoben.
8. **Peak-Ratio-Test in 126 zu streng** – als zweiter Peak zählt jeder Wert außerhalb ±3 Samples statt des zweiten lokalen Maximums (`Correlation_Processing_Module_126.cpp:116`). Bei schmalbandigen Harmonischen < 1,5 kHz ist die Hauptkeule viel breiter → Ratio ≈ 1,1 < 1,5 → keine Peilung. Im Host-Test bestätigt: bei Drohnensignal 0 % gültige Peilungen, auch bei SNR 30 dB.
   Status: **bearbeitet (25.09.2026, Commit 82c8fcc)** – im Host-Test geprüft, auf dem Board nicht getestet. `crossCorrelate()`: Hauptkeule = vom Maximum aus nach beiden Seiten, solange die Korrelation fällt; zweiter Peak = höchstes lokales Maximum außerhalb der Hauptkeule. Maximum am Fensterrand wird verworfen. Ohne Nebenmaximum wird die Ratio auf `PEAK_RATIO_MAX = 20` begrenzt (Gewicht in `128::solve()`). `PEAK_EXCLUDE_S` entfernt. Host-Test Drohne (volle Kette 118–126, 12 Richtungen, SNR 30…0 dB): gültige Peilungen vorher 0 %, nachher 99 / 97 / 86 / 75 / 56 / 33 %; Fehler-Median 0,5–1,7°, 95 %-Quantil 1,8–6,0°. Breitband-Test aus Punkt 23 unverändert (max. 0,08°). Offen: `srpScan()` berechnet seine Peak-Ratio (nur Debug-Anzeige) noch mit dem direkten Nachbarn des Maximums.
9. **Distanzschätzung misst die AGC** – `levelA` wird nach NS/AGC pro Kanal berechnet (`Processing_Module_120.cpp:84`); r = K/A beschreibt die Verstärkung, nicht den Abstand.
   Status: **bearbeitet (25.09.2026, Commit b81255c)** – im Host-Test geprüft, auf dem Board nicht getestet.
   - `Pre_Processor_118`: `noiseSuppress()` und `agc()` geben die angewendete Verstärkung zurück; `appliedGain(ch)` = NS · AGC des letzten Frames. Zugriff über `Sensor_Unit_112::preprocessor()`.
   - `Processing_Module_120`: `levelA` wird durch `appliedGain(REF_MIC)` geteilt → Pegel vor der Regelung.
   - Host-Test (Drohne, SNR 20 dB, Simulator-Pegel ∝ 1/r, 10…200 m): Verhältnis r_geschätzt/r_wahr vorher 0,073 / 0,042 / 0,024 / 0,024 / 0,024 (AGC regelt unterhalb ~50 m, darüber an der Obergrenze 32), nachher 0,525 / 0,521 / 0,528 / 0,523 / 0,522 (konstant, < 1 % Schwankung).
   - Offen: `LEVEL_DIST_K_REF = 100` ist nicht kalibriert (Simulator: Faktor ≈ 1,9 zu klein; der Simulator-Quellpegel ist willkürlich, daher nicht daraus übernommen). K mit realer Drohne in bekanntem Abstand bestimmen. Das Feld `level` im UnitReport hat damit eine andere Skala als vorher – falls der PC-Monitor es auswertet, dort anpassen.
10. **UnitReport inkonsistent** – `num_selected` wird ungekürzt (bis 64) gesendet, serialisiert werden max. 56 Einträge (`Output_Interface_130.cpp:50`).
    Status: **bearbeitet (25.09.2026, Commit d572820)** – im Host-Test geprüft, auf dem Board nicht getestet.
    - `Output_Interface_130::send()`: Feld `nsel` enthält die tatsächlich gesendete Anzahl n = min(num_selected, 56); die 56 folgt aus `(sizeof(MessageData) − 16) / 2`.
    - Host-Test Serialisierung (0/3/8/56/57/64 Bänder): vorher bei 57 und 64 Bändern `nsel` = 57/64, d. h. der PC hätte 130 bzw. 144 von 128 Byte gelesen; nachher `nsel` = 56 und Bandliste vollständig innerhalb der 128 Byte.
    - Hinweis: Seit Punkt 24 werden nur noch Bänder mit Harmonischen selektiert (≤ 8 bzw. 3 als Rückfall), mehr als 56 kommen praktisch nicht mehr vor.

## USB und RTOS

11. **USB-Senden fehlerhaft** – `CDC_Transmit_FS` speichert nur den Zeiger, gesendet wird aus Stack-Variablen (OTG-FS füllt den FIFO später im Interrupt). `send()` schickt zwei Nachrichten direkt hintereinander → UnitReport (id 4) meist `USBD_BUSY`. READ-Modus sendet 192 × 532 Byte ohne BUSY-Behandlung. Mehrere Tasks senden ohne Sperre. Abhilfe: TX-Queue + ein Sende-Task mit statischem Puffer, Warten auf `TransmitCplt`.
    Status: **bearbeitet (25.09.2026, Commit 15a87ab)** – auf dem Board noch nicht getestet – TX-Ringpuffer (8 KB) in `USBDriver`, Nachrichten werden im kritischen Abschnitt vollständig kopiert, Versand in Blöcken bis 2 KB aus statischem `txBuf_`, Nachladen in `CDC_TransmitCplt_FS` (USER CODE 13). Kein zusätzlicher Task. READ-Streaming wartet bis 20 ms auf Platz und verwirft sonst den Rest des Frames. Zähler `USBDriver::txDropped()`.
12. **Kommandolänge widersprüchlich** – Handler verlangt `payloadLen == 16`, Vorlagen für Typ 1–3 in `SDS_Structs.hpp` haben `0x0C`. Gegen PC-Seite prüfen.
    Status: **bearbeitet (25.09.2026, Commit 7207038)** – gebaut, auf dem Board nicht getestet.
    - Klärung: Der Handler (16) ist richtig. Seit Commit `b0cfc2c` (07.09.2026, Magic `DE AD BE EF` eingeführt) ist das Längenfeld die Gesamtlänge der Nachricht (4 Magic + 1 Id + 3 Länge + 4 Wert + 4 CRC = 16), wie bei den Nachrichten Board → PC. Vorher (bis `ce6a0a2`, 27.08.2026) gab es keinen Magic, und die Länge war 12. `PC_Monitor_Test.ptp` sendet ebenfalls `00 00 10`. Die Vorlagen mit `0x0C` stammten aus dem alten Format und werden in der Firmware nicht verwendet.
    - `SDS_Structs.hpp`: Konstante `SDS_CMD_LENGTH = 16`, alle vier Vorlagen nutzen sie, `static_assert` auf 16 Byte; Formatbeschreibung als Kommentar.
    - `USBTask`: `payloadLen()` → `msgLen()` (ist die Gesamtlänge), Vergleich gegen `SDS_CMD_LENGTH` statt der Zahl 16.
    - Weiterhin offen (bekanntes ToDo): Die CRC der Kommandos wird nicht geprüft.
13. **FreeRTOS-Heap (32 KB) zu knapp** – mit ProcessingTask (16 KB Stack) ≈ 30 KB plus TCBs/Queues. `TaskBase::start()` prüft `osThreadNew` nicht auf NULL. Heap auf ≥ 64 KB erhöhen.
    Status: **bearbeitet (25.09.2026, Commit 8d3644c)** – gebaut, auf dem Board nicht getestet.
    - Genauere Bilanz (Größen aus dem ELF, TCB 172 B; Idle- und Timer-Task liegen statisch, nicht im Heap): heute ≈ 10,3 KB, mit ProcessingTask ≈ 26,9 KB von 32 KB. Es hätte gepasst, aber mit nur ≈ 6 KB Reserve und ohne Fehlermeldung bei Überschreitung.
    - `configTOTAL_HEAP_SIZE` 32 KB → 64 KB in `FreeRTOSConfig.h` **und** `SDS.ioc` (sonst setzt CubeMX den Wert zurück). RAM-Belegung 69 → 101 KB von 320 KB.
    - `TaskBase::start()` gibt jetzt `bool` zurück und hält per `configASSERT` an, wenn `osThreadNew` NULL liefert (wie `TaskTimerBase`).
    - `freertos.c` (USER CODE 4/5): `vApplicationStackOverflowHook` und `vApplicationMallocFailedHook` waren leer; sie halten jetzt mit abgeschalteten Interrupts an.
    - Hinweis: Der ProcessingTask ist weiterhin auskommentiert (Punkt 1, „nicht bearbeiten“).

## Kleinere Punkte

14. SDRAM-Selbsttest läuft erst nach dem Konstruktor, der bereits ins SDRAM schreibt → schützt nicht.
    Status: **bearbeitet (25.09.2026, Commit 8a1240d)** – Logik im Host-Test geprüft, auf dem Board nicht getestet.
    - Zusätzlich gefunden: Der alte Test hätte auch später nichts erkannt – das SDRAM ist per MPU Write-Back-cachebar, die zwei Testwörter wurden aus dem D-Cache zurückgelesen.
    - Neu `Infrastructure/Driver/SDRAMSelfTest.{hpp,cpp}`: prüft zuerst `hsdram1.State == READY` (sonst kein Zugriff), dann Muster an Offset 0, 2^k und am letzten Wort von `.sdram_data` (je Adresse eigener Wert → Adressleitungen), `SCB_CleanInvalidateDCache()` vor dem Zurücklesen, zweiter Durchgang invertiert (Datenbits).
    - `SDS110_Init()`: Test läuft vor `Processing_Module_120::instance()`; bei Fehler Meldung „SDRAM self-test failed“, keine Initialisierung von 112/120, `SDS110_StartProcessingTask()` startet dann nicht. Alter Test und `Processing_Module_120_spectraProbe()` entfernt.
    - Host-Test (HAL-Stub): Handle nicht READY → false ohne Speicherzugriff; intakter Bereich (591 KB) → true, 20 Prüfadressen, 2 Cache-Flushes, kein Schreiben außerhalb; leerer Bereich → true. Fehlerhaftes SDRAM und das Cache-Verhalten lassen sich auf dem Host nicht nachbilden.
15. SDRAM-Kommando in `MX_DMA2D_Init` läuft vor `MX_FMC_Init` → wirkungslos, entfernen.
    Status: **bearbeitet (25.09.2026, Commit c673854)** – gebaut.
    - Geprüft: `hsdram1.State` ist vor `MX_FMC_Init` `HAL_SDRAM_STATE_RESET`; `HAL_SDRAM_SendCommand` gibt dann `HAL_ERROR` zurück, ohne Register anzufassen. Der Block war also harmlos, kostete nur `HAL_Delay(1)` beim Start. Die vollständige Sequenz (inkl. Clock-Enable) sendet `SDRAM_InitSequence()` in USER CODE „FMC_Init 2“.
    - Inhalt von USER CODE „DMA2D_Init 0“ in `main.c` entfernt (Marker und CRLF erhalten).
16. Zeitstempel 1-ms-Tick-basiert und vom Ende des DMA-Blocks (≈ 2,7 ms zu spät).
    Status: **bearbeitet (25.09.2026, Commit ea1e5b7)** – Zeitlogik im Host-Test geprüft, auf dem Board nicht getestet.
    - Neu `Infrastructure/Utils/TimeBase.{hpp,cpp}`: `TimeBase::nowUs()` aus DWT->CYCCNT (216 MHz, 4,6 ns), auf 64 bit erweitert (`CycleExtender`); zwischen zwei Aufrufen verpasste Überläufe (CYCCNT läuft alle 19,9 s über) werden über den 1-ms-HAL-Tick ermittelt. TIM2/TIM5 bewusst nicht genutzt (von CubeMX als PWM mit Pins belegt).
    - `Sampling_Circuitry_116`: Zeitstempel = `nowUs()` − Blockdauer (`DMA_BLOCK_SAMPLES` / Ist-Fs aus Punkt 5 ≈ 2 667 µs) → Zeit des ersten Samples im Block.
    - `ProcessingTask`: Simulationspfad nutzt dieselbe Zeitbasis statt `HAL_GetTick() · 1000`.
    - Host-Test `CycleExtender` (DMA-Takt 10 min; zufällige Abstände bis 15 s; Pausen 18–60 s; Pausen bis 1 h; Tick bis 1 ms verzögert): monoton, max. Fehler 0 µs. Gegenprobe ohne Tick-Korrektur: Pausen-Szenarien schlagen fehl.
    - Weiterhin offen: Die Zeit ist Laufzeit seit Start, keine UTC. Für die Inter-Unit-Synchronisation (Patent: 10 µs) fehlen GNSS-PPS/PTP; der USB-Zeitabgleich (`SDS_Data::syncTimeDifference`) wird nicht angewendet.
17. 50-%-Überlappung nicht umgesetzt; Framerate 15,6/s statt 30/s (bekannt).
    Status: **bearbeitet (25.09.2026, Commit 05b3d3d)** – im Host-Test geprüft, auf dem Board nicht getestet (Rechenlast nicht gemessen).
    - Aufbau: 114 liefert Hops (`HOP_SAMPLES` = 1536, 32 ms) ohne Überlappung; 118 verarbeitet jeden Hop genau einmal (IIR-Zustand läuft durch); neuer `Sensor_Unit_112/Frame_Assembler` setzt je Hop einen Analyse-Frame aus den letzten 2 Hops zusammen (3072 Samples, 50 % Überlappung, 31,25 Frames/s). Bei Lücken in der Hop-Folge beginnt das Fenster neu. `Sensor_Unit_112::nextFrame()` liefert Analyse-Frames, `nextHop()` die Hops für den READ-Modus (12 statt 24 Pakete je Mikrofon).
    - Zeitkonstanten jetzt in Sekunden in `SDS_110_Config.hpp` (`AGC_*_TAU_S`, `NS_FLOOR_TAU_S`, `HBD_FLOOR_TAU_S`, `HBD_FLOOR_RISE_DB_S`, `HBD_WARMUP_S`, `HBD_CONSISTENCY_S`, `framesFor()` für `HBD_HOLD_FRAMES`, `STATE_SMOOTH_FRAMES`, `AM_HISTORY_FRAMES`); Werte so gewählt, dass das Zeitverhalten gleich bleibt.
    - 118: NS · AGC wird als lineare Rampe je Hop angewendet. Ohne Rampe lag der Verstärkungssprung an der Hop-Grenze mitten im Frame; im Test sank dadurch bei 30 dB SNR die Peak-Ratio von 15,7 auf 5,4 und die gültigen Peilungen von 100 auf 91 %. Für den Distanzpegel (Punkt 9) wird die Verstärkung in der Frame-Mitte verwendet (`frameCenterGain()`).
    - Simulator: erzeugte je Aufruf `FRAME + 2·GUARD` Quellsamples, die Quelle sprang daher alle 64 ms um 128 Samples (bei getrennten Frames unsichtbar, mit Überlappung mitten im Frame). Jetzt wird jedes Sample genau einmal erzeugt (`generateHop()`, Vor-/Nachlauf aus dem vorigen Aufruf).
    - Host-Tests (Vergleich vorher → nachher, gleiche Zeitspanne): Peilung Drohne 30/20/10/3/0 dB gültig 100/100/100/98/91 → 100/100/100/99/90 %, Median-Fehler 0,24/0,49/0,92/1,77/2,16° → 0,19/0,43/0,88/1,55/2,21°; Reports 20/10/3/0 dB 100/100/98/51 → 100/100/100/56 %; Einzelton/Wind/Stille 0/0/0 % Reports; Breitband-Peiltest max. 0,09°; Distanzverhältnis 0,518–0,525 (vorher 0,521–0,528); stehender Ton weiterhin ~2 min erkannt. Eigentest `Frame_Assembler` (Füllen, Schieben, Zeitstempel, Neubeginn bei Lücke, `reset()`): alle Fälle bestanden.
    - Offen: Rechenlast verdoppelt sich (Schätzung aus MIGRATION_120: ~13 ms je Frame → ~40 % bei 31 Frames/s) – am Board mit den Task-Statistiken prüfen. `MIGRATION_122.md` („Overlap noch nicht implementiert“) ist damit veraltet (siehe Punkt 22).
18. Mit `NUM_UNITS = 1` ist die angezeigte Konfidenz immer ≈ 1.
    Status: **bearbeitet (25.09.2026, Commit e0ca865)** – im Host-Test geprüft, auf dem Board nicht getestet.
    - Zusätzlich gefunden: Zwei verschiedene Formeln – LCD (`SDS_Data`) `1/(1+Residuum_s)` ≈ 1, USB-Legacy-Frame `(Paare/28)/(1+Residuum·1000)` ≈ Paare/28 (Residuum in s bzw. als ms gerechnet, jeweils fast 0). Außerdem hat `CandidateLocation::ls_residual` je nach Pfad unterschiedliche Einheiten (Einzel-Unit s, `solve()` m) – jetzt dokumentiert.
    - Neu `candidateConfidence()` in 128: (Paare / max. Paare) · 1/(1 + (Residuum_Samples / `CONF_RESIDUAL_REF_SAMPLES`)²), Referenz 4 Samples. `CandidateLocation::confidence` wird in `fromBearing()` (Residuum s → Samples, max. 28 Paare) und `solve()` (Residuum m → Samples) gesetzt; `SDS_Data::setCandidate()` und der Legacy-Frame (über `UnitReport::confidence`) übernehmen den Wert.
    - Host-Test (Median): Drohne 30/20/10/3/0/−3 dB 0,96/0,86/0,68/0,53/0,49/0,29; Einzelton 0,13; Stille 0,01; Wind 0,99 (Konfidenz bewertet die Peilung, nicht die Drohnen-Detektion – die entscheidet `detected`, Punkt 24). UnitReport-Serialisierungstest (Punkt 10) weiterhin bestanden.
19. `Logger::write` nicht threadsicher; Längenbegrenzung 255 statt 256.
    Status: **bearbeitet (25.09.2026, Commit 7da4097)** – im Host-Test geprüft, auf dem Board nicht getestet.
    - Zusätzlich gefunden: `write()` prüfte den freien Platz nicht – ungelesene Daten wurden überschrieben; erreichte `head` genau `tail`, galt der Puffer als leer (bis 4 KB verloren). Derzeit ruft niemand `write()` auf, die Fehler hätten sich erst bei Nutzung gezeigt.
    - `Logger::write()`: Formatierung außerhalb der Sperre (max. `MAX_MSG` = 255 Zeichen), Einfügen in kurzem kritischen Abschnitt (PRIMASK, damit auch aus ISRs aufrufbar), Meldung ganz oder gar nicht; passt sie nicht, wird sie verworfen und gezählt (`dropped()`). Ungenutztes `#include "SDS_Data.hpp"` aus `Logger.hpp` entfernt.
    - Host-Test: 300 Zeichen → 255 Bytes ohne Nullbyte (vorher 256 mit Nullbyte); 4 Schreib-Threads × 20 000 Meldungen + 1 Leser parallel: 0 defekte, 0 vertauschte Meldungen, empfangen + verworfen = gesendet (vorher bereits die erste Meldung defekt).
20. `HBD_ML_Model_Data.hpp` (≈ 185 KB) nirgends eingebunden; Modell erwartet Cepstrum-Merkmale, die 122 nicht liefert. `SDS_SimDrone` ebenfalls ungenutzt.
    Status: offen
21. MPU-Region 0 macht SRAM1/2 komplett uncached (für DMA nötig, kostet Leistung).
    Status: **bearbeitet (25.09.2026)** – gebaut und Platzierung geprüft, auf dem Board nicht getestet (Leistungsgewinn nicht gemessen).
    - Zusätzlich gefunden: Die Region war Strongly-ordered (TEX0/C0/B0), nicht nur uncached – jeder Zugriff geordnet, nicht ausgerichtete Zugriffe unzulässig. Betroffen waren u. a. der Stack aller ISRs (`_estack` = 0x20050000), Teile der Task-Stacks (Ende von `ucHeap`), USB-Puffer, Logger, `SDS_Data`.
    - DMA-Nutzer geprüft: SAI (Puffer mit Cache-Pflege), ETH-Deskriptoren (im DTCM, nie gecacht; ETH wird nicht gestartet), DMA2D (nur SDRAM-Framebuffer, Region 2), USB OTG FS (ohne DMA). SD/FatFS: DMA-Pfad vorhanden, aber keine DMA-Streams/IRQs eingerichtet und FatFS ungenutzt.
    - Linker: `RAM` 304 kB (DTCM + SRAM1), neu `RAM_NC` 16 kB (SRAM2) mit Sektion `.dma_nocache`; `_estack` = 0x2004C000 (Stack jetzt gecacht). MPU-Region 0: nur SRAM2, Normal nicht cachebar (TEX=1, C=0, B=0). SRAM1 fällt unter die Standard-Speicherkarte (Write-Back, Write-Allocate).
    - SAI-DMA-Puffer über `SDS110_DMA_SECTION` in `.dma_nocache`; Probe-Link mit aktivem ProcessingTask: `dmaBuffer_` (8 kB) an 0x2004C000.
    - Hinweis für FatFS (falls genutzt): SD-DMA erst einrichten (Streams, IRQs), Puffer dann in `.dma_nocache` legen oder `ENABLE_SD_DMA_CACHE_MAINTENANCE` mit 32-Byte-ausgerichteten Puffern verwenden (STs Invalidierung auf abgerundete Adressen kann sonst Nachbardaten verwerfen).
22. `MIGRATION_*.md` teilweise veraltet (z. B. Linker-Sektion existiert bereits).
    Status: offen

## Neu aus dem Host-Test (25.09.2026)

23. **Peilung um 180° verdreht** – `crossCorrelate()` bildet R = X_i·X_j*, dessen Peak bei τ = ((p_j − p_i)·u)/c liegt; `estimateBearing()` und `srpScan()` rechnen aber mit τ = ((p_i − p_j)·u)/c. Im Host-Test zeigt jede gültige Peilung (Wind, breitbandig) 179,9° neben dem wahren Azimut.
    Status: **bearbeitet (25.09.2026, Commit 187441b)** – im Host-Test geprüft, auf dem Board nicht getestet. `Correlation_Processing_Module_126`: Fernfeldmodell in `estimateBearing()` (Normalgleichungen, Residuum) und `srpScan()` (`pairDx_/pairDy_`) auf τ_ij = ((p_j − p_i)·u)/c umgestellt; `crossCorrelate()` unverändert (liefert τ_ij = t_i − t_j, gleiche Konvention wie `128::solve()`). Peiltest (breitbandige Quelle, alle Bänder, Azimut 0…345° in 15°-Schritten): vorher 180° Fehler bei allen 24 Richtungen, nachher 24/24 gültig, max. Fehler 0,08° (TDOA-LS) bzw. 0,07° (SRP). Hinweis: die in MIGRATION_120 erwähnte alte Azimut-Kalibrierung (+12°, ×0,98) stammt aus dem SRP-Code vor der Migration und muss nach dieser Korrektur neu gemessen werden.
24. **Band-Selektion in 124 bei Rauschen zu großzügig** – Das Gate `g = score / finalScoreThreshold` ist schon bei Rauschen offen (Score ≈ 0,5 > 0,48), und `HBD_BAND_SNR_DB = 8 dB` liegt nahe am Maximum von Rauschbins im Band. Folge: `SDS_Data::detected` bei Stille 22 %, bei Einzelton 54 %. Parameter mit Aufnahmen abstimmen (vgl. MIGRATION_120 „Offene Punkte 1“), evtl. Gate an `droneDetected` koppeln.
    Status: **bearbeitet (25.09.2026, Commit b4a18f4)** – im Host-Test geprüft, auf dem Board nicht getestet.
    - `Machine_Learning_Module_124`: Bänder ohne Harmonische erhalten höchstens `HBD_GATE_FLOOR · q_b` (< θ_sel). Das Gate hängt nicht mehr am Score, sondern ist offen, solange der HBD in den letzten `HBD_HOLD_FRAMES = 16` Frames (~1 s) eine Drohne erkannt hat; sonst p_b · `HBD_GATE_FLOOR`.
    - `HBD`: Anlaufzeit `warmupFrames = 48` (~3 s) ohne Entscheidung – der Floor schwingt nach dem Start (AGC-Anlauf) bis ~Frame 40 ein und löste vorher auch bei Wind aus. Folge: Nach jedem Start werden die ersten ~3 s keine UnitReports gesendet.
    - Host-Test (6 Richtungen, ab Frame 50): Reports Drohne 20/10/3/0 dB: vorher 99/97/85/67 %, nachher 100/100/98/51 %; Einzelton 3 → 0 %, Wind 22 → 0 %, Stille 0 → 0 % (`detected` 68 → 0 %). Langlauf 3 900 Frames je Rauschszenario: 0 Reports. Anteil selektierter Bänder mit Harmonischer: ~42 % → 87–99 %. Peilung Drohne (Punkt-8-Test): 20 dB Median 0,65° → 0,49°, 0 dB gültig 33 % → 91 %.
    - Hinweis: Eine gültige Peilung allein ist kein Detektionskriterium – Wind (Punktquelle) wird zu 77 % gültig gepeilt, gesendet wird nur bei `detected`.
25. **Modus-Werte in `PC_Monitor_Test.ptp` vertauscht** – Die Test-Makros senden „Calibrate“ = 3 und „Read“ = 2; die Firmware verwendet seit dem ersten Commit `DETECT = 1, CALIBRATE = 2, READ = 3` (`SDS_Mode`). Entweder sind die Makros falsch beschriftet oder der PC-Monitor nutzt eine andere Zuordnung. Gegen den PC-Monitor prüfen, dann `.ptp` oder `SDS_Mode` angleichen.
    Status: offen
