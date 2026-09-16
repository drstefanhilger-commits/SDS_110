#pragma once

#include "ai_platform.h"
#include "sds_model.h"
#include "sds_model_data.h"
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    ai_handle  network;
    ai_buffer *input;
    ai_buffer *output;
} SDS_AIModel;

/**
 * @brief Initialisiert das X-CUBE-AI Modell.
 */
bool SDS_AIModel_Init(SDS_AIModel *m);

/**
 * @brief Führt eine Inferenz aus.
 *
 * @param m           Modellinstanz
 * @param input_data  Zeiger auf Input-Buffer (float oder int8)
 * @param output_data Zeiger auf Output-Buffer (float oder int8)
 */
bool SDS_AIModel_Run(SDS_AIModel *m, const float *in, float *out);

#ifdef __cplusplus
}
#endif
