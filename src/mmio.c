#include "mmio.h"
#include "device_regs.h"

#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static const uint32_t READABLE_OFFSETS[] = {
    REG_DEVICE_ID, REG_CONTROL, REG_STATUS, REG_DESC_RING_HEAD, REG_DESC_RING_TAIL,
};
static const size_t READABLE_OFFSETS_COUNT =
    sizeof(READABLE_OFFSETS) / sizeof(READABLE_OFFSETS[0]);

static void mirror_readable_registers(MmioHandle *h) {
    for (size_t i = 0; i < READABLE_OFFSETS_COUNT; i++) {
        uint32_t reg_value = 0;
        device_model_read(&h->model, READABLE_OFFSETS[i], &reg_value);
        memcpy(h->base + READABLE_OFFSETS[i], &reg_value, sizeof(reg_value));
    }
}

int mmio_open(MmioHandle *h, const char *shm_name, int create) {
    memset(h, 0, sizeof(*h));

    int flags = create ? (O_CREAT | O_RDWR) : O_RDWR;
    int fd = shm_open(shm_name, flags, 0666);
    if (fd < 0) {
        return -1;
    }

    if (create) {
        if (ftruncate(fd, (off_t)DEVICE_BAR_SIZE) != 0) {
            close(fd);
            return -1;
        }
    }

    void *base = mmap(NULL, DEVICE_BAR_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd); /* the mapping stays valid after the descriptor is closed */
    if (base == MAP_FAILED) {
        return -1;
    }

    h->base = (uint8_t *)base;
    h->size = DEVICE_BAR_SIZE;
    h->owns_shm = create;
    strncpy(h->shm_name, shm_name, sizeof(h->shm_name) - 1);
    device_model_init(&h->model);

    if (create) {
        memset(h->base, 0, h->size);
        /* Populate fixed, read-only registers like DEVICE_ID immediately,
         * the same way real hardware has them readable at power-on rather
         * than only after the driver writes something. Without this, a
         * second handle attaching before any write happened would see a
         * BAR full of zeros instead of the device's actual identity. */
        mirror_readable_registers(h);
    }

    return 0;
}

int mmio_write32(MmioHandle *h, uint32_t offset, uint32_t value) {
    if (device_model_write(&h->model, offset, value) != 0) {
        return -1;
    }

    mirror_readable_registers(h);

    return 0;
}

int mmio_read32(const MmioHandle *h, uint32_t offset, uint32_t *out_value) {
    DeviceModel dummy;
    uint32_t dummy_value;
    device_model_init(&dummy);

    /* device_model_read's validity rules (aligned, in range, readable) are
     * the single source of truth for which offsets this BAR exposes for
     * reading; the actual value still comes from the shared mapping below,
     * not from this throwaway model, so this genuinely exercises the IPC
     * path rather than only the local struct. */
    if (device_model_read(&dummy, offset, &dummy_value) != 0) {
        return -1;
    }

    memcpy(out_value, h->base + offset, sizeof(uint32_t));
    return 0;
}

void mmio_close(MmioHandle *h) {
    if (h->base) {
        munmap(h->base, h->size);
    }
    if (h->owns_shm) {
        shm_unlink(h->shm_name);
    }
    memset(h, 0, sizeof(*h));
}
