/*
 * SDS_RingBuffer.h
 *
 *  Created on: Aug 30, 2026
 *      Author: 310004
 */

#ifndef SDS_UTILS_SDS_RINGBUFFER_H_
#define SDS_UTILS_SDS_RINGBUFFER_H_

#include <stdint.h>
#include <stdbool.h>
#include "TDM_Parser.h"

#define SDS_RINGBUFFER_CAPACITY 64

typedef struct
{
    TDM_Frame_t frames[SDS_RINGBUFFER_CAPACITY];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} SDS_RingBuffer_t;

void SDS_RingBuffer_Init(SDS_RingBuffer_t *rb);
bool SDS_RingBuffer_PushFrame(SDS_RingBuffer_t *rb, const TDM_Frame_t *frame);
bool SDS_RingBuffer_PopFrame(SDS_RingBuffer_t *rb, TDM_Frame_t *frame);


#endif /* SDS_UTILS_SDS_RINGBUFFER_H_ */
