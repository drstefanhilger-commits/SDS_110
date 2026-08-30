/*
 * TDM_Parser.h
 *
 *  Created on: Aug 30, 2026
 *      Author: 310004
 */

#ifndef SDS_ADUA7118_TDM_PARSER_H_
#define SDS_ADUA7118_TDM_PARSER_H_

#include <stdint.h>

#define TDM_NUM_CHANNELS 8

typedef struct
{
    int32_t ch[TDM_NUM_CHANNELS];
} TDM_Frame_t;

void TDM_ParseFrame(const uint32_t *src, TDM_Frame_t *dst);

#endif /* SDS_ADUA7118_TDM_PARSER_H_ */
