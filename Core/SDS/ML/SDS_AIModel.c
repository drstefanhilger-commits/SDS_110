/*
 * SDS_AIModel.c
 *
 *  Created on: Sep 8, 2026
 *      Author: 310004
 */


#include "SDS_AIModel.h"

/* Statischer Activation-Pool */
AI_ALIGNED(4)
static uint8_t s_activations[AI_SDS_MODEL_DATA_ACTIVATIONS_SIZE];

bool SDS_AIModel_Init(SDS_AIModel *m)
{
    ai_error err;

    /* Netzwerk erzeugen */
    err = ai_sds_model_create(&m->network, AI_SDS_MODEL_DATA_CONFIG);
    if (err.type != AI_ERROR_NONE)
        return false;

    /* Netzwerk initialisieren */
    const ai_handle act_addr = s_activations;
    if (!ai_sds_model_init(m->network, act_addr))
        return false;

    /* Input/Output Buffer holen */
    m->input  = ai_sds_model_inputs_get(m->network, NULL);
    m->output = ai_sds_model_outputs_get(m->network, NULL);

    return true;
}

bool SDS_AIModel_Run(SDS_AIModel *m, void *input_data, void *output_data)
{
    m->input[0].data  = input_data;
    m->output[0].data = output_data;

    ai_i32 result = ai_sds_model_run(m->network, m->input, m->output);
    return (result == 0);
}
