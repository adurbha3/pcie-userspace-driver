#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "netproto.h"

int main(int argc, char **argv) {
    int port = argc > 1 ? atoi(argv[1]) : 9191;
    const char *message = argc > 2 ? argv[2] : "ping";

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons((uint16_t)port);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        perror("connect");
        return 1;
    }

    if (netproto_send_frame(fd, (const uint8_t *)message, (uint32_t)strlen(message)) != 0) {
        fprintf(stderr, "send failed\n");
        return 1;
    }

    uint8_t buf[NETPROTO_MAX_PAYLOAD];
    uint32_t len;
    if (netproto_recv_frame(fd, buf, sizeof(buf), &len) != 0) {
        fprintf(stderr, "recv failed\n");
        return 1;
    }

    printf("echo: %.*s\n", (int)len, buf);
    close(fd);
    return 0;
}
