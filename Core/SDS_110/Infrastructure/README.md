Nicht patentrelevante Infrastruktur (unveränderte Übernahme aus SDS):
- Tasks/  : TaskBase, LCDTask, LoggerTask (+ neuer SDS_110_Task als Aufrufer von 120)
- Driver/ : LCDDriver, Font8x12, SDRAMDriver, MPUDriver, PrintfDriver
- Utils/  : Logger, DWT, crc32, SDS_RingBuffer, syscalls
- Model/  : SDS_Params, SDS_Structs, SDS_Data (Status/Debug für LCD & Logger)
