#include "ring_buffer.h"
#include <string.h>

void ring_buffer_init(RingBuffer *rb, size_t capacity) {
    memset(rb, 0, sizeof(*rb));
    rb->capacity = capacity;
}

int ring_buffer_is_full(const RingBuffer *rb) {
    return rb->count == rb->capacity;
}

int ring_buffer_is_empty(const RingBuffer *rb) {
    return rb->count == 0;
}

int ring_buffer_push(RingBuffer *rb, uint32_t descriptor) {
    if (ring_buffer_is_full(rb)) {
        return -1;
    }
    rb->descriptors[rb->head] = descriptor;
    rb->head = (rb->head + 1) % rb->capacity;
    rb->count++;
    return 0;
}

int ring_buffer_pop(RingBuffer *rb, uint32_t *out) {
    if (ring_buffer_is_empty(rb)) {
        return -1;
    }
    *out = rb->descriptors[rb->tail];
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->count--;
    return 0;
}
