#include "test_framework.h"
#include "mmio.h"
#include "device_regs.h"

#include <unistd.h>

static void test_read_device_id_through_the_mapped_bar(void) {
    MmioHandle h;
    TEST_ASSERT_EQ(mmio_open(&h, "/pcie_test_a", 1), 0);

    uint32_t value;
    TEST_ASSERT_EQ(mmio_read32(&h, REG_DEVICE_ID, &value), 0);
    TEST_ASSERT_EQ(value, DEVICE_MAGIC_ID);

    mmio_close(&h);
}

static void test_second_handle_sees_device_id_before_any_write(void) {
    /* Regression test: DEVICE_ID is a fixed, read-only register that is
     * never produced by a write (device_model_write has no case that sets
     * it), so it used to stay at the mmap's zero-initialized value until
     * mmio_open started mirroring the device model's initial readable
     * state into the BAR immediately at creation time. */
    MmioHandle creator, observer;
    TEST_ASSERT_EQ(mmio_open(&creator, "/pcie_test_e", 1), 0);
    TEST_ASSERT_EQ(mmio_open(&observer, "/pcie_test_e", 0), 0);

    uint32_t value;
    TEST_ASSERT_EQ(mmio_read32(&observer, REG_DEVICE_ID, &value), 0);
    TEST_ASSERT_EQ(value, DEVICE_MAGIC_ID);

    mmio_close(&observer);
    mmio_close(&creator);
}

static void test_write_is_rejected_for_read_only_register(void) {
    MmioHandle h;
    mmio_open(&h, "/pcie_test_b", 1);

    TEST_ASSERT_EQ(mmio_write32(&h, REG_DEVICE_ID, 0), -1);

    mmio_close(&h);
}

static void test_second_handle_on_same_shm_name_observes_writes_from_the_first(void) {
    /* This is the real point of the test: two separate MmioHandle structs,
     * each with their own mmap() call, attached to the same POSIX shared
     * memory segment, standing in for two processes (or driver instances)
     * sharing one device's BAR. If this passes, the shared-memory mapping
     * is doing real work, not just the in-process DeviceModel struct. */
    MmioHandle writer, reader;
    TEST_ASSERT_EQ(mmio_open(&writer, "/pcie_test_c", 1), 0);
    TEST_ASSERT_EQ(mmio_open(&reader, "/pcie_test_c", 0), 0);

    TEST_ASSERT_EQ(mmio_write32(&writer, REG_CONTROL, CONTROL_ENABLE), 0);

    uint32_t status;
    TEST_ASSERT_EQ(mmio_read32(&reader, REG_STATUS, &status), 0);
    TEST_ASSERT(status & STATUS_READY);

    mmio_close(&reader);
    mmio_close(&writer);
}

static void test_desc_ring_head_write_is_visible_to_other_handle(void) {
    MmioHandle writer, reader;
    mmio_open(&writer, "/pcie_test_d", 1);
    mmio_open(&reader, "/pcie_test_d", 0);

    mmio_write32(&writer, REG_DESC_RING_HEAD, 99);

    uint32_t tail;
    mmio_read32(&reader, REG_DESC_RING_TAIL, &tail);
    TEST_ASSERT_EQ(tail, 99);

    mmio_close(&reader);
    mmio_close(&writer);
}

int main(void) {
    TEST_RUN(test_read_device_id_through_the_mapped_bar);
    TEST_RUN(test_second_handle_sees_device_id_before_any_write);
    TEST_RUN(test_write_is_rejected_for_read_only_register);
    TEST_RUN(test_second_handle_on_same_shm_name_observes_writes_from_the_first);
    TEST_RUN(test_desc_ring_head_write_is_visible_to_other_handle);
    TEST_SUMMARY_AND_RETURN();
}
