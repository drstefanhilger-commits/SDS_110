#include "SDS_AIModel.h"

AI_ALIGNED(4)
static uint8_t s_activations[AI_SDS_MODEL_DATA_ACTIVATIONS_SIZE];

bool SDS_AIModel_Init(SDS_AIModel *m)
{
    ai_error err;

    /* Netzwerk erzeugen */
    err = ai_sds_model_create(&m->network, AI_SDS_MODEL_DATA_CONFIG);
    if (err.type != AI_ERROR_NONE)
        return false;

    /* Netzwerk-Parameter (Weights + Activations) */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
    const ai_network_params params = {
        AI_SDS_MODEL_DATA_WEIGHTS(ai_sds_model_data_weights_get()),
        AI_SDS_MODEL_DATA_ACTIVATIONS(s_activations)
    };
#pragma GCC diagnostic pop

    /* Netzwerk initialisieren */
    if (!ai_sds_model_init(m->network, &params))
        return false;

    /* Input/Output Buffer holen */
    m->input  = ai_sds_model_inputs_get(m->network, NULL);
    m->output = ai_sds_model_outputs_get(m->network, NULL);

    return true;
}

//bool SDS_AIModel_Run(SDS_AIModel *m, void *input_data, void *output_data)
//{
//    m->input[0].data  = input_data;
//    m->output[0].data = output_data;
//
//    ai_i32 res = ai_sds_model_run(m->network, m->input, m->output);
//    return (res == 0);
//}

bool SDS_AIModel_Run(SDS_AIModel* m, const float* in, float* out)
{
    if (!m || !m->network || !in || !out)
        return false;

    // 1. Input-Daten in den AI-Input-Buffer kopieren
    memcpy(m->input[0].data, in, AI_SDS_MODEL_IN_1_SIZE * sizeof(float));

    // 2. Inferenz ausführen
    ai_i32 nbatch = ai_sds_model_run(m->network, m->input, m->output);

    if (nbatch != 1)
        return false;

    // 3. Output-Daten kopieren
    memcpy(out, m->output[0].data, AI_SDS_MODEL_OUT_1_SIZE * sizeof(float));

    return true;
}
