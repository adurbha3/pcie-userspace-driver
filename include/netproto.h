#ifndef NETPROTO_H
#define NETPROTO_H

#include <stdint.h>
#include <stddef.h>

/* A small length-prefixed, checksummed framing protocol over a byte stream
 * (a TCP socket, or a unix socketpair in tests). Wire format:
 *   [4-byte big-endian length][payload][1-byte XOR checksum of payload]
 * This is the same shape of problem as a real control-plane protocol for a
 * device's out-of-band management interface: turning an unreliable,
 * arbitrarily-chunked byte stream into discrete, integrity-checked
 * messages. */

#define NETPROTO_MAX_PAYLOAD 4096

/* Sends one framed message. Loops on partial writes. Returns 0 on success,
 * -1 on a socket error or an oversized payload. */
int netproto_send_frame(int fd, const uint8_t *payload, uint32_t len);

/* Receives one framed message into buf (capacity buf_cap). Loops on partial
 * reads. Returns 0 on success (sets *out_len), -1 on a socket error, a
 * connection close, an oversized frame, a buffer too small for the frame,
 * or a checksum mismatch. */
int netproto_recv_frame(int fd, uint8_t *buf, uint32_t buf_cap, uint32_t *out_len);

#endif /* NETPROTO_H */
