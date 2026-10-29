#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "netproto.h"

/* A minimal out-of-band management-style control channel over real TCP:
 * accepts one connection, reads one framed request, and echoes it back
 * framed, then exits. Real bind()/listen()/accept() over a real socket,
 * not a socketpair -- the netproto framing itself is unit-tested
 * separately against a socketpair in tests/test_netproto.c. */

int main(int argc, char **argv) {
    int port = argc > 1 ? atoi(argv[1]) : 9191;

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons((uint16_t)port);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        perror("bind");
        return 1;
    }
    if (listen(listen_fd, 1) != 0) {
        perror("listen");
        return 1;
    }

    printf("listening on 127.0.0.1:%d\n", port);
    fflush(stdout);

    int client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept");
        return 1;
    }

    uint8_t buf[NETPROTO_MAX_PAYLOAD];
    uint32_t len;
    if (netproto_recv_frame(client_fd, buf, sizeof(buf), &len) != 0) {
        fprintf(stderr, "failed to receive frame\n");
        close(client_fd);
        close(listen_fd);
        return 1;
    }

    printf("received %u bytes, echoing back\n", len);
    netproto_send_frame(client_fd, buf, len);

    close(client_fd);
    close(listen_fd);
    return 0;
}
