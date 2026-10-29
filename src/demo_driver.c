#include <stdio.h>
#include "mmio.h"
#include "device_regs.h"
#include "ring_buffer.h"

/* End-to-end demo: open the emulated device's BAR over shared memory (the
 * same mmio.h API a real userspace PCIe driver would use against
 * /sys/bus/pci/devices/.../resource0), enable it, push a batch of
 * descriptors into a software ring buffer, hand the ring's head position to
 * the device via its DESC_RING_HEAD register, observe the resulting
 * interrupt, and acknowledge it. */

int main(void) {
    MmioHandle dev;
    if (mmio_open(&dev, "/pcie_demo_device", 1) != 0) {
        fprintf(stderr, "failed to open device\n");
        return 1;
    }

    uint32_t device_id;
    mmio_read32(&dev, REG_DEVICE_ID, &device_id);
    printf("device id: 0x%08X\n", device_id);

    mmio_write32(&dev, REG_CONTROL, CONTROL_ENABLE | CONTROL_IRQ_ENABLE);

    uint32_t status;
    mmio_read32(&dev, REG_STATUS, &status);
    printf("status after enable: 0x%02X (ready=%d)\n", status, (status & STATUS_READY) != 0);

    RingBuffer ring;
    ring_buffer_init(&ring, 16);
    for (uint32_t i = 0; i < 5; i++) {
        ring_buffer_push(&ring, 0x1000 + i);
    }
    printf("queued %zu descriptors in software ring\n", ring.count);

    /* Hand the producer position to the device. */
    mmio_write32(&dev, REG_DESC_RING_HEAD, (uint32_t)ring.head);

    uint32_t tail;
    mmio_read32(&dev, REG_DESC_RING_TAIL, &tail);
    mmio_read32(&dev, REG_STATUS, &status);
    printf("device consumed up to tail=%u, irq_pending=%d\n", tail,
           (status & STATUS_IRQ_PENDING) != 0);

    mmio_write32(&dev, REG_IRQ_ACK, 1);
    mmio_read32(&dev, REG_STATUS, &status);
    printf("status after irq ack: 0x%02X (irq_pending=%d)\n", status,
           (status & STATUS_IRQ_PENDING) != 0);

    mmio_close(&dev);
    return 0;
}
