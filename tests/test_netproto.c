#include "test_framework.h"
#include "netproto.h"

#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void make_pair(int fds[2]) {
    /* A unix socketpair gives real stream-socket read()/write() semantics
     * (including partial reads/writes), the same contract netproto.c is
     * written against for a TCP socket, without needing to bind a real
     * port in tests. */
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
}

static void test_round_trips_a_simple_message(void) {
    int fds[2];
    make_pair(fds);

    const char *msg = "hello device";
    TEST_ASSERT_EQ(netproto_send_frame(fds[0], (const uint8_t *)msg, (uint32_t)strlen(msg)), 0);

    uint8_t buf[64];
    uint32_t len;
    TEST_ASSERT_EQ(netproto_recv_frame(fds[1], buf, sizeof(buf), &len), 0);
    TEST_ASSERT_EQ(len, strlen(msg));
    TEST_ASSERT_EQ(memcmp(buf, msg, len), 0);

    close(fds[0]);
    close(fds[1]);
}

static void test_round_trips_an_empty_message(void) {
    int fds[2];
    make_pair(fds);

    TEST_ASSERT_EQ(netproto_send_frame(fds[0], NULL, 0), 0);

    uint8_t buf[16];
    uint32_t len;
    TEST_ASSERT_EQ(netproto_recv_frame(fds[1], buf, sizeof(buf), &len), 0);
    TEST_ASSERT_EQ(len, 0);

    close(fds[0]);
    close(fds[1]);
}

static void test_rejects_a_frame_too_large_for_the_caller_buffer(void) {
    int fds[2];
    make_pair(fds);

    uint8_t payload[32];
    memset(payload, 'A', sizeof(payload));
    netproto_send_frame(fds[0], payload, sizeof(payload));

    uint8_t small_buf[8];
    uint32_t len;
    TEST_ASSERT_EQ(netproto_recv_frame(fds[1], small_buf, sizeof(small_buf), &len), -1);

    close(fds[0]);
    close(fds[1]);
}

static void test_detects_a_corrupted_checksum(void) {
    int fds[2];
    make_pair(fds);

    const char *msg = "trust but verify";
    netproto_send_frame(fds[0], (const uint8_t *)msg, (uint32_t)strlen(msg));

    /* Flip a bit in the payload after it is "on the wire" by reading it back
     * raw, corrupting it, and re-sending a hand-built frame with the
     * original (now-mismatched) checksum appended, to prove recv actually
     * checks the checksum rather than trusting the length header alone. */
    int fds2[2];
    make_pair(fds2);

    uint32_t len = (uint32_t)strlen(msg);
    uint8_t header[4] = {
        (uint8_t)(len >> 24), (uint8_t)(len >> 16), (uint8_t)(len >> 8), (uint8_t)len,
    };
    uint8_t corrupted[32];
    memcpy(corrupted, msg, len);
    corrupted[0] ^= 0xFF; /* flip the first byte after framing */
    uint8_t checksum = 0;
    for (uint32_t i = 0; i < len; i++) {
        checksum ^= ((const uint8_t *)msg)[i]; /* checksum of the ORIGINAL payload */
    }

    write(fds2[0], header, sizeof(header));
    write(fds2[0], corrupted, len);
    write(fds2[0], &checksum, 1);

    uint8_t buf[64];
    uint32_t out_len;
    TEST_ASSERT_EQ(netproto_recv_frame(fds2[1], buf, sizeof(buf), &out_len), -1);

    close(fds[0]);
    close(fds[1]);
    close(fds2[0]);
    close(fds2[1]);
}

int main(void) {
    TEST_RUN(test_round_trips_a_simple_message);
    TEST_RUN(test_round_trips_an_empty_message);
    TEST_RUN(test_rejects_a_frame_too_large_for_the_caller_buffer);
    TEST_RUN(test_detects_a_corrupted_checksum);
    TEST_SUMMARY_AND_RETURN();
}
