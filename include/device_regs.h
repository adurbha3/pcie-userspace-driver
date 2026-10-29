#ifndef DEVICE_REGS_H
#define DEVICE_REGS_H

#include <stdint.h>

/* Register layout for an emulated PCIe-style network offload device, modeled
 * after the shape of a real NIC's BAR0 register space: a control/status
 * register pair plus a descriptor ring head/tail used for DMA-style
 * producer/consumer signaling between software and hardware. */

#define REG_DEVICE_ID      0x00u  /* read-only: fixed magic value */
#define REG_CONTROL        0x04u  /* read/write: ENABLE, RESET, IRQ_ENABLE */
#define REG_STATUS         0x08u  /* read-only: READY, ERROR, IRQ_PENDING */
#define REG_DESC_RING_HEAD 0x0Cu  /* read/write: producer index */
#define REG_DESC_RING_TAIL 0x10u  /* read-only: consumer index (device-owned) */
#define REG_IRQ_ACK        0x14u  /* write-only: write 1 to clear IRQ_PENDING */

#define DEVICE_BAR_SIZE    0x1000u /* 4 KiB register space, like a real BAR0 */

#define DEVICE_MAGIC_ID    0xD00DFEEDu

#define CONTROL_ENABLE     (1u << 0)
#define CONTROL_RESET      (1u << 1)
#define CONTROL_IRQ_ENABLE (1u << 2)

#define STATUS_READY       (1u << 0)
#define STATUS_ERROR       (1u << 1)
#define STATUS_IRQ_PENDING (1u << 2)

#endif /* DEVICE_REGS_H */
