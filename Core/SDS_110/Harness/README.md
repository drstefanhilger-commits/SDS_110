# Harness – Test ohne Mikrofone

## Signal_Simulator (On-Target)
Erzeugt synthetische 8-Kanal-Frames und speist sie über `Microphone_Array_114::pushBlock()`
ein (gleicher Pfad wie der SAI-DMA). Aktiv, wenn `SDS_Data::simulation == 1` (Standard;
USB-Kommando Typ 3 schaltet um). Der ProcessingTask erzeugt pro Durchlauf einen Frame.

Szenario und Grundwerte: `SIM_SCENARIO_ID`, `SIM_F0_HZ`, `SIM_SNR_DB` in SDS_110_Config.hpp.
Feinere Parameter: `Signal_Simulator::instance().params()` (Azimut, Distanz, Harmonische,
Blattpass-AM, Pegel, Sweep-Schritte).

| Szenario | Erwartung im LCD (DETECT / READ) |
|---|---|
| 0 DroneSweep  | HBD `DRONE`, f0 ≈ SIM_F0_HZ, Azimut folgt "True Azimuth" (±3° grün), s(t)-Balken an den Harmonischen |
| 1 DroneStatic | wie oben, fest – zum Abstimmen von HBD_BAND_SNR_DB / perBandSnrDb / THETA_SEL |
| 2 SingleTone  | f0 gefunden, aber Konsistenz < 5 Harmonische -> kein `DRONE`; kaum selektierte Bänder |
| 3 WindNoise   | f0 springt am 80-Hz-Rand (Schwäche des Pegel-f0-Schätzers), Score klein, kein `DRONE` |
| 4 Silence     | Noise-Floor sinkt auf minFloorDb; p_b -> 0 |

Parameter-Sweeps: SIM_SNR_DB von 30 dB abwärts senken, bis `DRONE` ausfällt -> Empfindlichkeit;
SIM_F0_HZ über 80..350 Hz -> Bandabdeckung der Harmonischen bis 4 kHz (h ≤ 8 bei f0 ≤ 500 Hz).

## Host-Harness (WSL, geplant)
122/124/126/128 sind portables C++ (CMSIS-DSP-Quellen). Mit demselben Generator oder WAV-Dateien
lassen sich Parameter auf dem PC in Schleifen durchtesten. Noch nicht angelegt.
