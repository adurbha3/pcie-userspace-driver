#ifndef DEVICE_MODEL_H
#define DEVICE_MODEL_H

#include <stdint.h>

/* A software model of the device's documented register semantics -- the
 * same kind of model a driver author writes against a datasheet before
 * real silicon or an FPGA prototype is available. Pure state in, state out:
 * no I/O, which makes it fully unit-testable on its own, independent of the
 * memory-mapped I/O layer in mmio.c that exposes it to a driver. */

typedef struct {
    uint32_t control;
    uint32_t status;
    uint32_t desc_head;
    uint32_t desc_tail;
} DeviceModel;

void device_model_init(DeviceModel *dev);

/* Returns 0 on success, -1 if the offset is unmapped, misaligned, or
 * read-only. */
int device_model_write(DeviceModel *dev, uint32_t offset, uint32_t value);

/* Returns 0 on success (writes *out_value), -1 if the offset is unmapped,
 * misaligned, or write-only. */
int device_model_read(const DeviceModel *dev, uint32_t offset, uint32_t *out_value);

#endif /* DEVICE_MODEL_H */
