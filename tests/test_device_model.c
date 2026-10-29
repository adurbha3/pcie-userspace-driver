#include "test_framework.h"
#include "device_model.h"
#include "device_regs.h"

static void test_reads_device_id_and_initial_state(void) {
    DeviceModel dev;
    device_model_init(&dev);

    uint32_t value;
    TEST_ASSERT_EQ(device_model_read(&dev, REG_DEVICE_ID, &value), 0);
    TEST_ASSERT_EQ(value, DEVICE_MAGIC_ID);

    TEST_ASSERT_EQ(device_model_read(&dev, REG_STATUS, &value), 0);
    TEST_ASSERT_EQ(value, 0);
}

static void test_enable_sets_ready(void) {
    DeviceModel dev;
    device_model_init(&dev);

    TEST_ASSERT_EQ(device_model_write(&dev, REG_CONTROL, CONTROL_ENABLE), 0);

    uint32_t status;
    device_model_read(&dev, REG_STATUS, &status);
    TEST_ASSERT(status & STATUS_READY);
}

static void test_disable_clears_ready(void) {
    DeviceModel dev;
    device_model_init(&dev);
    device_model_write(&dev, REG_CONTROL, CONTROL_ENABLE);

    device_model_write(&dev, REG_CONTROL, 0);

    uint32_t status;
    device_model_read(&dev, REG_STATUS, &status);
    TEST_ASSERT(!(status & STATUS_READY));
}

static void test_reset_dominates_other_control_bits(void) {
    DeviceModel dev;
    device_model_init(&dev);
    device_model_write(&dev, REG_CONTROL, CONTROL_ENABLE | CONTROL_IRQ_ENABLE);
    device_model_write(&dev, REG_DESC_RING_HEAD, 5);

    /* Reset requested together with enable -- reset should win. */
    device_model_write(&dev, REG_CONTROL, CONTROL_RESET | CONTROL_ENABLE);

    uint32_t status, control, head;
    device_model_read(&dev, REG_STATUS, &status);
    device_model_read(&dev, REG_CONTROL, &control);
    device_model_read(&dev, REG_DESC_RING_HEAD, &head);
    TEST_ASSERT_EQ(status, 0);
    TEST_ASSERT_EQ(control, 0);
    TEST_ASSERT_EQ(head, 0);
}

static void test_advancing_desc_head_advances_tail_and_raises_irq(void) {
    DeviceModel dev;
    device_model_init(&dev);
    device_model_write(&dev, REG_CONTROL, CONTROL_ENABLE | CONTROL_IRQ_ENABLE);

    device_model_write(&dev, REG_DESC_RING_HEAD, 42);

    uint32_t tail, status;
    device_model_read(&dev, REG_DESC_RING_TAIL, &tail);
    device_model_read(&dev, REG_STATUS, &status);
    TEST_ASSERT_EQ(tail, 42);
    TEST_ASSERT(status & STATUS_IRQ_PENDING);
}

static void test_desc_head_does_not_raise_irq_when_irq_disabled(void) {
    DeviceModel dev;
    device_model_init(&dev);
    device_model_write(&dev, REG_CONTROL, CONTROL_ENABLE); /* no IRQ_ENABLE */

    device_model_write(&dev, REG_DESC_RING_HEAD, 7);

    uint32_t status;
    device_model_read(&dev, REG_STATUS, &status);
    TEST_ASSERT(!(status & STATUS_IRQ_PENDING));
}

static void test_irq_ack_clears_irq_pending(void) {
    DeviceModel dev;
    device_model_init(&dev);
    device_model_write(&dev, REG_CONTROL, CONTROL_ENABLE | CONTROL_IRQ_ENABLE);
    device_model_write(&dev, REG_DESC_RING_HEAD, 1);

    device_model_write(&dev, REG_IRQ_ACK, 1);

    uint32_t status;
    device_model_read(&dev, REG_STATUS, &status);
    TEST_ASSERT(!(status & STATUS_IRQ_PENDING));
}

static void test_writes_to_read_only_registers_are_rejected(void) {
    DeviceModel dev;
    device_model_init(&dev);
    TEST_ASSERT_EQ(device_model_write(&dev, REG_DEVICE_ID, 0xFF), -1);
    TEST_ASSERT_EQ(device_model_write(&dev, REG_DESC_RING_TAIL, 0xFF), -1);
}

static void test_reads_of_write_only_register_are_rejected(void) {
    DeviceModel dev;
    device_model_init(&dev);
    uint32_t value;
    TEST_ASSERT_EQ(device_model_read(&dev, REG_IRQ_ACK, &value), -1);
}

static void test_unmapped_and_misaligned_offsets_are_rejected(void) {
    DeviceModel dev;
    device_model_init(&dev);
    uint32_t value;

    TEST_ASSERT_EQ(device_model_write(&dev, 0x18, 1), -1); /* unmapped */
    TEST_ASSERT_EQ(device_model_read(&dev, 0x18, &value), -1);

    TEST_ASSERT_EQ(device_model_write(&dev, 0x05, 1), -1); /* misaligned */
    TEST_ASSERT_EQ(device_model_read(&dev, 0x05, &value), -1);

    TEST_ASSERT_EQ(device_model_write(&dev, DEVICE_BAR_SIZE, 1), -1); /* out of range */
}

int main(void) {
    TEST_RUN(test_reads_device_id_and_initial_state);
    TEST_RUN(test_enable_sets_ready);
    TEST_RUN(test_disable_clears_ready);
    TEST_RUN(test_reset_dominates_other_control_bits);
    TEST_RUN(test_advancing_desc_head_advances_tail_and_raises_irq);
    TEST_RUN(test_desc_head_does_not_raise_irq_when_irq_disabled);
    TEST_RUN(test_irq_ack_clears_irq_pending);
    TEST_RUN(test_writes_to_read_only_registers_are_rejected);
    TEST_RUN(test_reads_of_write_only_register_are_rejected);
    TEST_RUN(test_unmapped_and_misaligned_offsets_are_rejected);
    TEST_SUMMARY_AND_RETURN();
}
