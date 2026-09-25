# SDS_110 – Befunde der Code-Analyse (25.09.2026)

Status: **nicht bearbeiten** = bewusst zurückgestellt, **offen** = zu bearbeiten,
**bearbeitet** = geändert (Datum, Commit; Testumfang steht dabei).

## Übersicht bearbeitet

| Punkt | Thema | Datum | Commit |
|---|---|---|---|
| 11 | USB-Senden über TX-Ringpuffer | 25.09.2026 | 15a87ab |
| 7 | HBD-Rauschboden / Normierung | 25.09.2026 | c8be312 |
| 23 | Peilung 180° verdreht (Vorzeichen Fernfeldmodell) | 25.09.2026 | 187441b |
| 8 | Peak-Ratio-Test (lokale Nebenmaxima statt ±3 Samples) | 25.09.2026 | noch nicht committet |

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
   Status: offen
6. **Rohdaten um Faktor 256 falsch skaliert** – 24-Bit-Samples kommen linksbündig im 32-Bit-Slot; `Microphone_Array_114.cpp:64` skaliert mit 2⁻²³ statt 2⁻³¹. Der Simulator schreibt rechtsbündig und verdeckt den Fehler.
   Status: offen

## Signalverarbeitung

7. **HBD meldet immer „DRONE“ (berechnet)** – Rauschboden auf max. −60 dB begrenzt (`HBD.cpp:16`), Betragsspektrum aber unnormiert (Rauschbins nach AGC ≈ +10 dB) → Boden klebt bei −60 dB, SNR ≈ 70 dB überall, alle p_b ≈ 1, alle Bänder selektiert, Fehlalarm auch bei Wind/Einzelton. Abhilfe: Spektrum durch Σw normieren (1536 für Hann/3072; `windowGain` ist vorgesehen, aber ungenutzt). Danach beachten: der gleitende Mittelwert (0,94) lernt einen stehenden Drohnenton in ≈ 1 s als Rauschen.
   Status: **bearbeitet (25.09.2026, Commit c8be312)** – im Host-Test geprüft, auf dem Board nicht getestet – `HBD.cpp/.hpp`: Spektrum in dBFS normiert (`windowGain = 2/Σw`), Floor-Grenzen −140…0 dBFS, Start-Floor aus dem spektralen Umgebungsmittel, EMA je Bin; schmale Peaks (> Umgebungsmittel + 10 dB) heben den Floor nur um 0,01 dB/Frame (stehender Drohnenton wird erst nach ~2 min gelernt). Host-Test (Simulator, SNR 20 dB, 300 Frames): DRONE bei Drohne 100 %, Einzelton 3 %, Wind 0 %, Stille 0 % (vorher überall 100 %); Empfindlichkeit 81 % bei 10 dB, 38 % bei 3 dB.
   Nachtrag: `sel>=B_MIN` (Band-Selektion in 124, steuert `SDS_Data::detected`) ist bei Stille noch 22 %, bei Einzelton 54 % – siehe Punkt 24.
8. **Peak-Ratio-Test in 126 zu streng** – als zweiter Peak zählt jeder Wert außerhalb ±3 Samples statt des zweiten lokalen Maximums (`Correlation_Processing_Module_126.cpp:116`). Bei schmalbandigen Harmonischen < 1,5 kHz ist die Hauptkeule viel breiter → Ratio ≈ 1,1 < 1,5 → keine Peilung. Im Host-Test bestätigt: bei Drohnensignal 0 % gültige Peilungen, auch bei SNR 30 dB.
   Status: **bearbeitet (25.09.2026)** – im Host-Test geprüft, auf dem Board nicht getestet. `crossCorrelate()`: Hauptkeule = vom Maximum aus nach beiden Seiten, solange die Korrelation fällt; zweiter Peak = höchstes lokales Maximum außerhalb der Hauptkeule. Maximum am Fensterrand wird verworfen. Ohne Nebenmaximum wird die Ratio auf `PEAK_RATIO_MAX = 20` begrenzt (Gewicht in `128::solve()`). `PEAK_EXCLUDE_S` entfernt. Host-Test Drohne (volle Kette 118–126, 12 Richtungen, SNR 30…0 dB): gültige Peilungen vorher 0 %, nachher 99 / 97 / 86 / 75 / 56 / 33 %; Fehler-Median 0,5–1,7°, 95 %-Quantil 1,8–6,0°. Breitband-Test aus Punkt 23 unverändert (max. 0,08°). Offen: `srpScan()` berechnet seine Peak-Ratio (nur Debug-Anzeige) noch mit dem direkten Nachbarn des Maximums.
9. **Distanzschätzung misst die AGC** – `levelA` wird nach NS/AGC pro Kanal berechnet (`Processing_Module_120.cpp:84`); r = K/A beschreibt die Verstärkung, nicht den Abstand.
   Status: offen
10. **UnitReport inkonsistent** – `num_selected` wird ungekürzt (bis 64) gesendet, serialisiert werden max. 56 Einträge (`Output_Interface_130.cpp:50`).
    Status: offen

## USB und RTOS

11. **USB-Senden fehlerhaft** – `CDC_Transmit_FS` speichert nur den Zeiger, gesendet wird aus Stack-Variablen (OTG-FS füllt den FIFO später im Interrupt). `send()` schickt zwei Nachrichten direkt hintereinander → UnitReport (id 4) meist `USBD_BUSY`. READ-Modus sendet 192 × 532 Byte ohne BUSY-Behandlung. Mehrere Tasks senden ohne Sperre. Abhilfe: TX-Queue + ein Sende-Task mit statischem Puffer, Warten auf `TransmitCplt`.
    Status: **bearbeitet (25.09.2026, Commit 15a87ab)** – auf dem Board noch nicht getestet – TX-Ringpuffer (8 KB) in `USBDriver`, Nachrichten werden im kritischen Abschnitt vollständig kopiert, Versand in Blöcken bis 2 KB aus statischem `txBuf_`, Nachladen in `CDC_TransmitCplt_FS` (USER CODE 13). Kein zusätzlicher Task. READ-Streaming wartet bis 20 ms auf Platz und verwirft sonst den Rest des Frames. Zähler `USBDriver::txDropped()`.
12. **Kommandolänge widersprüchlich** – Handler verlangt `payloadLen == 16`, Vorlagen für Typ 1–3 in `SDS_Structs.hpp` haben `0x0C`. Gegen PC-Seite prüfen.
    Status: offen
13. **FreeRTOS-Heap (32 KB) zu knapp** – mit ProcessingTask (16 KB Stack) ≈ 30 KB plus TCBs/Queues. `TaskBase::start()` prüft `osThreadNew` nicht auf NULL. Heap auf ≥ 64 KB erhöhen.
    Status: offen

## Kleinere Punkte

14. SDRAM-Selbsttest läuft erst nach dem Konstruktor, der bereits ins SDRAM schreibt → schützt nicht.
    Status: offen
15. SDRAM-Kommando in `MX_DMA2D_Init` läuft vor `MX_FMC_Init` → wirkungslos, entfernen.
    Status: offen
16. Zeitstempel 1-ms-Tick-basiert und vom Ende des DMA-Blocks (≈ 2,7 ms zu spät).
    Status: offen
17. 50-%-Überlappung nicht umgesetzt; Framerate 15,6/s statt 30/s (bekannt).
    Status: offen
18. Mit `NUM_UNITS = 1` ist die angezeigte Konfidenz immer ≈ 1.
    Status: offen
19. `Logger::write` nicht threadsicher; Längenbegrenzung 255 statt 256.
    Status: offen
20. `HBD_ML_Model_Data.hpp` (≈ 185 KB) nirgends eingebunden; Modell erwartet Cepstrum-Merkmale, die 122 nicht liefert. `SDS_SimDrone` ebenfalls ungenutzt.
    Status: offen
21. MPU-Region 0 macht SRAM1/2 komplett uncached (für DMA nötig, kostet Leistung).
    Status: offen
22. `MIGRATION_*.md` teilweise veraltet (z. B. Linker-Sektion existiert bereits).
    Status: offen

## Neu aus dem Host-Test (25.09.2026)

23. **Peilung um 180° verdreht** – `crossCorrelate()` bildet R = X_i·X_j*, dessen Peak bei τ = ((p_j − p_i)·u)/c liegt; `estimateBearing()` und `srpScan()` rechnen aber mit τ = ((p_i − p_j)·u)/c. Im Host-Test zeigt jede gültige Peilung (Wind, breitbandig) 179,9° neben dem wahren Azimut.
    Status: **bearbeitet (25.09.2026, Commit 187441b)** – im Host-Test geprüft, auf dem Board nicht getestet. `Correlation_Processing_Module_126`: Fernfeldmodell in `estimateBearing()` (Normalgleichungen, Residuum) und `srpScan()` (`pairDx_/pairDy_`) auf τ_ij = ((p_j − p_i)·u)/c umgestellt; `crossCorrelate()` unverändert (liefert τ_ij = t_i − t_j, gleiche Konvention wie `128::solve()`). Peiltest (breitbandige Quelle, alle Bänder, Azimut 0…345° in 15°-Schritten): vorher 180° Fehler bei allen 24 Richtungen, nachher 24/24 gültig, max. Fehler 0,08° (TDOA-LS) bzw. 0,07° (SRP). Hinweis: die in MIGRATION_120 erwähnte alte Azimut-Kalibrierung (+12°, ×0,98) stammt aus dem SRP-Code vor der Migration und muss nach dieser Korrektur neu gemessen werden.
24. **Band-Selektion in 124 bei Rauschen zu großzügig** – Das Gate `g = score / finalScoreThreshold` ist schon bei Rauschen offen (Score ≈ 0,5 > 0,48), und `HBD_BAND_SNR_DB = 8 dB` liegt nahe am Maximum von Rauschbins im Band. Folge: `SDS_Data::detected` bei Stille 22 %, bei Einzelton 54 %. Parameter mit Aufnahmen abstimmen (vgl. MIGRATION_120 „Offene Punkte 1“), evtl. Gate an `droneDetected` koppeln.
    Status: offen
