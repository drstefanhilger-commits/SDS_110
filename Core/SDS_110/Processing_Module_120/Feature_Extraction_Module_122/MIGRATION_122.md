# Migration 118 + 122

| Neu                                   | Aus SDS                                   | Änderung |
|---------------------------------------|-------------------------------------------|----------|
| Pre_Processor_118.{hpp,cpp}           | SDS_Params (SDS_BANDPASS_LOW/HIGH)        | neu: Biquad-Bandpass 80–8000 Hz, NS, AGC |
| Feature_Extraction_Module_122.{hpp,cpp} | ML/FFTProcessor, MelFilterbank, MelSpectrogram, SDS_Data::computeMelFeatures | 256 → 4096 Punkte; Mel-Tabelle → Laufzeit |

## Speicher (statisch, je Instanz)
- 122: window 12 kB, buf+fftOut 32 kB, mag+prevMag 16 kB, Mel sparse 40×512×4 = 80 kB, AM 4 kB → ~145 kB
  → Instanz `Feature_Extraction_Module_122` in Processing_Module_120 mit `SDS110_SDRAM_SECTION` ablegen
  oder `MEL_MAX_BINS_PER_BAND` verkleinern (oberstes Mel-Band bei 8 kHz ist ~110 Bins breit; 128 reicht → 20 kB).
- 118: < 1 kB.

## Laufzeit (F746 @ 216 MHz, grob)
- 4096-Punkt-RFFT ≈ 0,25 ms; Bandpass 8 × 3072 Samples × 2 Biquads ≈ 0,5 ms; Mel/Flux/AM < 0,2 ms.
- Bei 30 Frames/s (Hop 32 ms) ist 122 für den Referenzkanal unkritisch. 8 Kanäle FFT (für 126) ≈ 2 ms.

## Offene Punkte
1. **CubeAI-Modell**: trainiert auf 40 Log-Mel aus 256-Punkt-FFT/48 kHz – mit der neuen Filterbank nicht mehr identisch. Neutraining nötig (ohnehin für B=64 Sigmoid-Ausgänge, Abschnitt 3).
2. `band_am_depth` ist Modulationstiefe (std/mean), nicht das volle AM-Spektrum. Falls das Modell das Spektrum braucht: 16-Punkt-RFFT über amHist_ je Band ergänzen.
3. Frame-Overlap 50 %: 114 liefert derzeit disjunkte Frames (3072). Für Hop 1536 muss 114 einen Gleitpuffer führen oder 122 zwei Halbframes zusammensetzen – noch nicht implementiert.
4. NS arbeitet frameweise im Zeitbereich (Breitband-Gain). Eine spektrale Variante gehört in 122 vor die Bandleistung.
