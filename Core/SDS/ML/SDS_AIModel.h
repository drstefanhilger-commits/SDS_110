/*
 * SDS_AIModel.h
 *
 *  Created on: Sep 8, 2026
 *      Author: 310004
 */

#pragma once
#include "ai_platform.h"
#include "sds_model.h"
#include "sds_model_data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    ai_handle network;
    ai_buffer *input;
    ai_buffer *output;
} SDS_AIModel;

/* Initialisierung */
bool SDS_AIModel_Init(SDS_AIModel *m);

/* Inferenz */
bool SDS_AIModel_Run(SDS_AIModel *m, void *input_data, void *output_data);

#ifdef __cplusplus
}
#endif
