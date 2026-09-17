# Migration Infrastructure/Tasks + Driver + Utils

| Neu | Aus SDS | Änderung |
|---|---|---|
| Tasks/TaskBase.{hpp,cpp} | Tasks/TaskBase | Namespace; `reportStats(TaskId)` schreibt Monitoring in SDS_Data |
| Tasks/USBTask.{hpp,cpp} | Tasks/USBTask + `usb_debug_counter` aus LCDTask.cpp | neue SDS_Data-API; RX-Queue wird leer gelesen statt 1 Msg/Loop |
| Driver/USBDriver.{hpp,cpp} | Driver/USBDriver + SDSUSBMicSender | Klasse `USBDriver` (statisch); sendRead liest MicFrame (float) |
| Utils/DWT.hpp, Utils/crc32.hpp | Utils/ | Namespace |

Noch nicht migriert (main.c ruft sie über den Wrapper): LCDTask, LoggerTask, Logger,
LCDDriver, Font8x12, SDRAMDriver, MPUDriver, PrintfDriver, syscalls.c.

## Wichtig bei komplett ausgeschlossenem Core/SDS
- `Core/SDS/Utils/syscalls.c` muss im Build bleiben (oder nach Infrastructure/Utils kopiert
  werden), sonst fehlen `_write` & Co. für printf/ITM.
- `SDS_Init`, `SDS_StartDisplayManagerTask`, `SDS_StartLoggerTask` bleiben undefined, bis
  LCDTask/LoggerTask migriert und `SDS_110_Wrapper.cpp` angelegt ist. Übergangsweise in
  main.c auskommentieren oder `SDS_110_Wrapper.cpp` mit leeren Rümpfen anlegen.

## Stand 2 (LCD/Logger)
| Neu | Aus SDS | Änderung |
|---|---|---|
| Tasks/LCDTask.{hpp,cpp} | Tasks/LCDTask | neue SDS_Data-API; READ-Modus zeigt 64-Band-Spektrum s(t) statt 4 AI-Klassen |
| Tasks/LoggerTask.{hpp,cpp} | Tasks/LoggerTask | USBDriver::sendMessage |
| Utils/Logger.{hpp,cpp} | Utils/Logger | Include-Pfad |
| Driver/LCDDriver.hpp, Font8x12.*, SDRAMDriver.h, PrintfDriver.h, MPUDriver.h | Driver/ | unverändert kopiert |

main.c: `SDS_Init()`, `SDS_StartDisplayManagerTask()`, `SDS_StartUSBTask()`, `SDS_StartLoggerTask()`
können wieder einkommentiert werden. `SDS_StartMicTask()` ist ein No-op-Makro,
`SDS_StartSRPPhatTask()` bleibt leer bis Processing_Module_120 fertig ist.

Hinweis 122: Feature_Extraction_Module_122.{hpp,cpp} gehören nach
Processing_Module_120/Feature_Extraction_Module_122/ (Patent-Ort). Die Kopie unter
Sensor_Unit_112/ im Repo löschen, sonst zwei gleichnamige Header im Include-Pfad.
