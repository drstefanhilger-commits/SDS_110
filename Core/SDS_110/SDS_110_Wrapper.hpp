/*
 * SDS_110_Wrapper.hpp
 * C-Schnittstelle für main.c (Ersatz für SDS_Wrapper.hpp).
 */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void SDS110_Init(void);
void SDS110_StartProcessingTask(void);   // Processing_Module_120
void SDS110_StartDisplayTask(void);
void SDS110_StartLoggerTask(void);
#ifdef __cplusplus
}
#endif
