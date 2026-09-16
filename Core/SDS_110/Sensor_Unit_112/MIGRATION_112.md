# Migration Sensor_Unit_112 (114 + 116)

## Herkunft
| Neu                                | Aus SDS                                              |
|------------------------------------|------------------------------------------------------|
| Microphone_Array_114.{hpp,cpp}     | Model/SDS_MicrophoneBuffer, SDS_Params (Geometrie)   |
| Sampling_Circuitry_116.{hpp,cpp}   | ADUA7118/adua7118Driver.c, sai.c, adau7118.c, dma.c  |
| ADAU7118_Registers.hpp             | ADUA7118/adua7118Driver.c (REG_*)                    |
| entfällt                           | TDM_Parser.*, SDS_SAI_A.*, Utils/SDS_RingBuffer.*    |

## Offene Punkte (vor dem ersten Test klären)
1. **I2C-Adresse**: 0x4B (adua7118Driver.c) vs. 0x3A (adau7118.c) → `ADAU7118_I2C_ADDR_7B` in Config.
2. **SAI-Block**: sai.c nutzt SAI1_Block_A (PE4/5/6), main.c (CubeMX) SAI2_Block_A/B.
   `init()` bekommt das Handle übergeben; MSP (GPIO/Clock) muss in CubeMX zum gewählten Block passen.
3. **Registerwerte** in ADAU7118_Registers.hpp gegen Datenblatt prüfen (zwei Sequenzen im alten Code widersprachen sich).
4. **Linker**: Sektion `.sdram_data` für `MicFrame frames_[3]` (3 × ~98 kB) anlegen; sonst Makro `SDS110_SDRAM_SECTION` leer definieren und NUM_MIC_FRAMES/FRAME_SAMPLES reduzieren.
5. **Doppelte HAL-Callbacks**: HAL_SAI_Rx*Callback nur noch in Sampling_Circuitry_116.cpp; alte Definitionen entfernen.
6. **Zeitbasis**: now_us() ist Tick-basiert (1 ms); für Patent-Sync (10 µs) TIM2 32-bit oder GNSS-PPS.

## Aufruf aus main.c / Wrapper
```
extern SAI_HandleTypeDef hsai_BlockA2; extern I2C_HandleTypeDef hi2c1;
unit.init(&hsai_BlockA2, &hi2c1); unit.start();
```
