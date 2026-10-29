# PCIe Userspace Driver

A userspace, memory-mapped-I/O device driver for an emulated PCIe-style
network offload device, built the same way real UIO/VFIO-style userspace
drivers are: by mapping a device's register space (BAR) directly into the
process and reading/writing 32-bit registers against that mapping, with no
kernel module on the data path.

Since there is no real PCIe hardware to target here, the "device" is a
software model of a plausible register interface (modeled after the shape
of a real NIC's BAR0: control/status registers plus a DMA descriptor ring
head/tail), the same thing a driver author writes against a datasheet
before real silicon is available. The register space is backed by POSIX
shared memory rather than a real `/sys/bus/pci/devices/.../resource0`
mapping, so multiple processes can attach to the same simulated device.

**What this project does not claim**: it does not load a real Linux kernel
module (that needs a target kernel's exact header set, which was not
available for this environment's kernel and would mean inserting code into
a shared machine's running kernel — too much blast radius for a portfolio
project) or talk to real PCIe hardware. Everything here is genuine C systems
programming — memory-mapped I/O, POSIX shared memory, socket programming,
bit-level register semantics — run and tested on real Linux (via Docker,
since the host this was built on is macOS), not simulated behavior dressed
up to look otherwise.

## Components

- **`device_model.c`** — pure, dependency-free register state machine: no
  I/O, fully unit-testable on its own. Enforces real hardware-style
  semantics (reset dominates other control bits, read-only/write-only
  registers reject the wrong direction, unmapped and misaligned offsets are
  rejected).
- **`mmio.c`** — the userspace driver layer: opens a POSIX-shared-memory-backed
  BAR, maps it with `mmap()`, and exposes `mmio_read32`/`mmio_write32`
  against the mapping. Multiple `MmioHandle`s can attach to the same shared
  memory segment to simulate separate processes observing one device.
- **`ring_buffer.c`** — a fixed-capacity circular descriptor ring, the same
  core data structure a NIC driver uses to hand DMA descriptors to hardware.
- **`netproto.c`** — a small length-prefixed, checksummed framing protocol
  over a byte stream, for an out-of-band management-style control channel.
  Includes a real TCP client/server (`tcp_client.c`/`tcp_server.c`) built on
  it, not just the framing logic in isolation.
- **`demo_driver.c`** — an end-to-end demo: open the device, enable it,
  queue descriptors into a software ring, hand them to the device, observe
  the resulting interrupt, and acknowledge it.

## Building and testing

Built and tested on real Linux (the host is macOS, so this runs in a
`debian:bookworm` container):

```bash
docker run --rm -v "$(pwd)":/work -w /work debian:bookworm \
  bash -c "apt-get update -qq && apt-get install -y -qq gcc make && make test && make integration-test"
```

`make test` runs 62 assertions across four unit-test binaries (device
model, ring buffer, mmio/shared-memory, and the framing protocol).
`make integration-test` additionally runs a real TCP client/server round
trip over loopback and the full end-to-end driver demo.

## Real bugs found and fixed while building this

- **`-std=c11` hid POSIX declarations.** Strict C11 mode hides `ftruncate`,
  `shm_open`, and friends behind glibc's feature-test macros, so the first
  build failed with "implicit declaration of function `ftruncate`." Fixed
  by compiling with `-D_POSIX_C_SOURCE=200809L`.
- **`DEVICE_ID` read as 0 from a second handle.** `REG_DEVICE_ID` is a fixed
  constant computed on read, never produced by any write, so the
  mirror-into-shared-memory step (which only ran after a write) never had a
  chance to populate it — a second process attaching to the device before
  any write happened saw a BAR full of zeros instead of the device's actual
  identity. Fixed by mirroring the model's initial readable register state
  into shared memory at creation time, not only after writes, matching how
  real hardware has fixed registers readable at power-on.
