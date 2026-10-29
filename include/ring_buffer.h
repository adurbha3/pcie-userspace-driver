#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stddef.h>
#include <stdint.h>

/* A fixed-capacity circular descriptor ring, the same core data structure a
 * real NIC driver uses to hand DMA descriptors to hardware: a software
 * producer index and a hardware (or, here, device-model) consumer index
 * chasing each other around a fixed-size array. */

typedef struct {
    uint32_t descriptors[256];
    size_t capacity;
    size_t head; /* next free slot to produce into */
    size_t tail; /* next slot to consume from */
    size_t count;
} RingBuffer;

void ring_buffer_init(RingBuffer *rb, size_t capacity);

/* Returns 0 on success, -1 if the ring is full. */
int ring_buffer_push(RingBuffer *rb, uint32_t descriptor);

/* Returns 0 on success (writes *out), -1 if the ring is empty. */
int ring_buffer_pop(RingBuffer *rb, uint32_t *out);

int ring_buffer_is_full(const RingBuffer *rb);
int ring_buffer_is_empty(const RingBuffer *rb);

#endif /* RING_BUFFER_H */
