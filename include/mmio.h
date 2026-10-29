#ifndef MMIO_H
#define MMIO_H

#include <stddef.h>
#include <stdint.h>
#include "device_model.h"

/* A userspace, memory-mapped-I/O driver layer in the same shape as a real
 * UIO/VFIO-style PCIe driver: it maps a device's BAR into the process's
 * address space (here, via POSIX shared memory standing in for a real
 * /sys/bus/pci/devices/.../resource0 mapping) and reads/writes 32-bit
 * registers directly against that mapping, with no kernel module involved
 * on the data path. */

typedef struct {
    uint8_t *base;
    size_t size;
    char shm_name[64];
    int owns_shm; /* 1 if this handle created the segment and should unlink it */
    DeviceModel model;
} MmioHandle;

/* Opens (creating if needed) a shared-memory-backed BAR of DEVICE_BAR_SIZE
 * bytes and maps it into the process. Multiple MmioHandle instances can
 * attach to the same shm_name to simulate separate processes (or driver
 * instances) observing the same device. Returns 0 on success, -1 on
 * failure. */
int mmio_open(MmioHandle *h, const char *shm_name, int create);

/* Writes a 32-bit register through the device model, then mirrors the
 * model's full register state into the mapped BAR bytes so any other
 * process holding the same shm_name observes the update. Returns 0 on
 * success, -1 if the register write was rejected by the device model. */
int mmio_write32(MmioHandle *h, uint32_t offset, uint32_t value);

/* Reads a 32-bit register directly from the mapped BAR bytes (not the
 * in-process model), so this genuinely exercises the shared-memory mapping
 * rather than just the local struct. Returns 0 on success, -1 if the
 * register is invalid. */
int mmio_read32(const MmioHandle *h, uint32_t offset, uint32_t *out_value);

void mmio_close(MmioHandle *h);

#endif /* MMIO_H */
