#include "test_framework.h"
#include "ring_buffer.h"

static void test_push_then_pop_returns_same_value(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 4);

    TEST_ASSERT_EQ(ring_buffer_push(&rb, 123), 0);

    uint32_t value;
    TEST_ASSERT_EQ(ring_buffer_pop(&rb, &value), 0);
    TEST_ASSERT_EQ(value, 123);
}

static void test_fifo_order_is_preserved(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 4);
    ring_buffer_push(&rb, 1);
    ring_buffer_push(&rb, 2);
    ring_buffer_push(&rb, 3);

    uint32_t a, b, c;
    ring_buffer_pop(&rb, &a);
    ring_buffer_pop(&rb, &b);
    ring_buffer_pop(&rb, &c);
    TEST_ASSERT_EQ(a, 1);
    TEST_ASSERT_EQ(b, 2);
    TEST_ASSERT_EQ(c, 3);
}

static void test_pop_from_empty_fails(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 4);
    uint32_t value;
    TEST_ASSERT_EQ(ring_buffer_pop(&rb, &value), -1);
}

static void test_push_to_full_fails(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 2);
    TEST_ASSERT_EQ(ring_buffer_push(&rb, 1), 0);
    TEST_ASSERT_EQ(ring_buffer_push(&rb, 2), 0);
    TEST_ASSERT_EQ(ring_buffer_push(&rb, 3), -1);
}

static void test_wraps_around_the_underlying_array(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 3);

    ring_buffer_push(&rb, 1);
    ring_buffer_push(&rb, 2);
    uint32_t discard;
    ring_buffer_pop(&rb, &discard); /* consume 1, freeing a slot at index 0 */
    ring_buffer_push(&rb, 3);
    ring_buffer_push(&rb, 4); /* wraps head back to index 0 */

    uint32_t a, b, c;
    ring_buffer_pop(&rb, &a);
    ring_buffer_pop(&rb, &b);
    ring_buffer_pop(&rb, &c);
    TEST_ASSERT_EQ(a, 2);
    TEST_ASSERT_EQ(b, 3);
    TEST_ASSERT_EQ(c, 4);
}

static void test_is_full_and_is_empty_flags(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 1);
    TEST_ASSERT(ring_buffer_is_empty(&rb));
    TEST_ASSERT(!ring_buffer_is_full(&rb));

    ring_buffer_push(&rb, 9);
    TEST_ASSERT(!ring_buffer_is_empty(&rb));
    TEST_ASSERT(ring_buffer_is_full(&rb));
}

int main(void) {
    TEST_RUN(test_push_then_pop_returns_same_value);
    TEST_RUN(test_fifo_order_is_preserved);
    TEST_RUN(test_pop_from_empty_fails);
    TEST_RUN(test_push_to_full_fails);
    TEST_RUN(test_wraps_around_the_underlying_array);
    TEST_RUN(test_is_full_and_is_empty_flags);
    TEST_SUMMARY_AND_RETURN();
}
