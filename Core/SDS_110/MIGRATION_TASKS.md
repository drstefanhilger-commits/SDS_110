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
