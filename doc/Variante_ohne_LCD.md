# Variante ohne LCD (STM32F746G-Discovery)

Stand 30.09.2026. Dieser Branch übernimmt `Core/SDS_110` aus dem Repo **SDS_110_STM32F746ZGT6**
(Stand nach PR #4) und passt es an das Discovery-CubeMX-Projekt dieses Repos an. Ziel: auf dem
Discovery genau die Firmware testen, die auf dem eigenen Board läuft.

## Unterschiede zu `master`

* **Kein LCDTask** (LCDTask, LCDDriver, Font entfallen). `SDS110_Init()` schaltet Display
  (`LCD_DISP`) und Hintergrundbeleuchtung (`LCD_BL_CTRL`) aus; LTDC/SDRAM bleiben initialisiert.
* **Nur Simulation** (`SDS110_SAI_ENABLED 0` in `Core/SDS_110/SDS_110_Board.h`): 116 wird nicht
  initialisiert, USB-Kommando Typ 3 wählt das Szenario, Typ 3 = 0 wird ignoriert. Mit 1 laufen die
  Mikrofone wie bisher über SAI2 Block A + I2C1.
* **Speicher wie auf dem ZGT6-Board**: Hops als 16-Bit-Blockgleitkomma, Frame_Assembler ohne eigene
  Slots, Spektren bis Bin 352, gemeinsamer DSP-Scratch, FFT-Twiddles im Flash – das SDRAM trägt
  keine SDS-Puffer mehr (nur den LTDC-Framebuffer), kein SDRAM-Selbsttest.
  RAM 294 kB / 304 KB, FreeRTOS-Heap 32 kB (`.ioc` und `FreeRTOSConfig.h`).
* **Status**: LED1 (PI1, grün) = Herzschlag 1 Hz, dauerhaft an bei fatalem Fehler (Meldung
  zusätzlich über ITM/SWO). PI1 ist in CubeMX als SPI2_SCK (Arduino D13) eingetragen und wird dafür
  auf Ausgang umgestellt.
* **PC-Verbindung** unverändert: USB-CDC an CN13 (USB FS).
* `test/host` und `tools/features` an das neue Hop-Format angepasst; die Merkmalsversion weicht
  vom trainierten Modell ab (`t_ml124` meldet einen Hinweis) – vor der Stufe `Ml` neu trainieren.

## Prüfung (Kommandozeile)

* `make -C wsl PREFIX=arm-none-eabi-`: Build ohne Fehler/Warnungen
* `make -C test/host check`: alle Prüfungen bestanden
* `make -C tools/features check`: Board-Kette = Werkzeug (0 Abweichungen), alle Prüfungen OK
