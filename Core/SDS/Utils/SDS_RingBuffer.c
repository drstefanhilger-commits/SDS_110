/*
 * SDS_RingBuffer.c
 *
 *  Created on: Aug 30, 2026
 *      Author: 310004
 */

#include "SDS_RingBuffer.h"

void SDS_RingBuffer_Init(SDS_RingBuffer_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

bool SDS_RingBuffer_PushFrame(SDS_RingBuffer_t *rb, const TDM_Frame_t *frame)
{
    if (rb->count >= SDS_RINGBUFFER_CAPACITY)
        return false;

    rb->frames[rb->head] = *frame;
    rb->head = (rb->head + 1) % SDS_RINGBUFFER_CAPACITY;
    rb->count++;

    return true;
}

bool SDS_RingBuffer_PopFrame(SDS_RingBuffer_t *rb, TDM_Frame_t *frame)
{
    if (rb->count == 0)
        return false;

    *frame = rb->frames[rb->tail];
    rb->tail = (rb->tail + 1) % SDS_RINGBUFFER_CAPACITY;
    rb->count--;

    return true;
}
