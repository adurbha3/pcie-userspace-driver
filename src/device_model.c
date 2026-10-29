#include "device_model.h"
#include "device_regs.h"
#include <string.h>

void device_model_init(DeviceModel *dev) {
    memset(dev, 0, sizeof(*dev));
}

int device_model_write(DeviceModel *dev, uint32_t offset, uint32_t value) {
    if (offset % 4 != 0 || offset >= DEVICE_BAR_SIZE) {
        return -1; /* misaligned or out of range */
    }

    switch (offset) {
    case REG_CONTROL:
        if (value & CONTROL_RESET) {
            /* Reset is dominant: any other bits in the same write are
             * ignored, matching how real devices typically treat a reset
             * request. */
            device_model_init(dev);
            return 0;
        }
        dev->control = value;
        if (value & CONTROL_ENABLE) {
            dev->status |= STATUS_READY;
        } else {
            dev->status &= ~STATUS_READY;
        }
        return 0;

    case REG_DESC_RING_HEAD:
        dev->desc_head = value;
        /* The device "processes" descriptors synchronously in this model
         * and immediately advances the tail to match, then raises an
         * interrupt if the driver asked for one -- a simplified stand-in
         * for a NIC completing a batch of DMA descriptors. */
        dev->desc_tail = dev->desc_head;
        if (dev->control & CONTROL_IRQ_ENABLE) {
            dev->status |= STATUS_IRQ_PENDING;
        }
        return 0;

    case REG_IRQ_ACK:
        dev->status &= ~STATUS_IRQ_PENDING;
        return 0;

    case REG_DEVICE_ID:
    case REG_DESC_RING_TAIL:
        return -1; /* read-only registers */

    default:
        return -1; /* unmapped register */
    }
}

int device_model_read(const DeviceModel *dev, uint32_t offset, uint32_t *out_value) {
    if (offset % 4 != 0 || offset >= DEVICE_BAR_SIZE) {
        return -1;
    }

    switch (offset) {
    case REG_DEVICE_ID:
        *out_value = DEVICE_MAGIC_ID;
        return 0;
    case REG_CONTROL:
        *out_value = dev->control;
        return 0;
    case REG_STATUS:
        *out_value = dev->status;
        return 0;
    case REG_DESC_RING_HEAD:
        *out_value = dev->desc_head;
        return 0;
    case REG_DESC_RING_TAIL:
        *out_value = dev->desc_tail;
        return 0;
    case REG_IRQ_ACK:
        return -1; /* write-only register */
    default:
        return -1; /* unmapped register */
    }
}
