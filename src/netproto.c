#include "netproto.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

static int write_all(int fd, const uint8_t *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = write(fd, buf + sent, len - sent);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            return -1;
        }
        sent += (size_t)n;
    }
    return 0;
}

static int read_all(int fd, uint8_t *buf, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t n = read(fd, buf + got, len - got);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            return -1; /* peer closed before sending a full frame */
        }
        got += (size_t)n;
    }
    return 0;
}

static uint8_t checksum_of(const uint8_t *payload, uint32_t len) {
    uint8_t sum = 0;
    for (uint32_t i = 0; i < len; i++) {
        sum ^= payload[i];
    }
    return sum;
}

int netproto_send_frame(int fd, const uint8_t *payload, uint32_t len) {
    if (len > NETPROTO_MAX_PAYLOAD) {
        return -1;
    }

    uint8_t header[4] = {
        (uint8_t)(len >> 24), (uint8_t)(len >> 16), (uint8_t)(len >> 8), (uint8_t)(len),
    };

    if (write_all(fd, header, sizeof(header)) != 0) {
        return -1;
    }
    if (len > 0 && write_all(fd, payload, len) != 0) {
        return -1;
    }

    uint8_t checksum = checksum_of(payload, len);
    if (write_all(fd, &checksum, 1) != 0) {
        return -1;
    }

    return 0;
}

int netproto_recv_frame(int fd, uint8_t *buf, uint32_t buf_cap, uint32_t *out_len) {
    uint8_t header[4];
    if (read_all(fd, header, sizeof(header)) != 0) {
        return -1;
    }

    uint32_t len = ((uint32_t)header[0] << 24) | ((uint32_t)header[1] << 16) |
                   ((uint32_t)header[2] << 8) | (uint32_t)header[3];

    if (len > NETPROTO_MAX_PAYLOAD || len > buf_cap) {
        return -1;
    }

    if (len > 0 && read_all(fd, buf, len) != 0) {
        return -1;
    }

    uint8_t received_checksum;
    if (read_all(fd, &received_checksum, 1) != 0) {
        return -1;
    }

    if (checksum_of(buf, len) != received_checksum) {
        return -1;
    }

    *out_len = len;
    return 0;
}
