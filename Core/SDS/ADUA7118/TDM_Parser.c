/*
 * TDM_Parser.c
 *
 *  Created on: Aug 30, 2026
 *      Author: 310004
 */


#include "TDM_Parser.h"

void TDM_ParseFrame(const uint32_t *src, TDM_Frame_t *dst)
{
    for (int i = 0; i < TDM_NUM_CHANNELS; ++i)
        dst->ch[i] = (int32_t)src[i];
}
