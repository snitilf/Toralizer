// test_socks4.c
//
// standalone socks4 client: asks tor on 127.0.0.1:9050 to connect to <ipv4> <port>
// runs the same socks4_handshake() as the dylib
// exit status is 0 when tor grants the connection, 1 when it does not, 2 for bad usage

#include "toralize.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <ipv4> <port>\n", argv[0]);
        return 2;
    }

    struct in_addr ip;
    if (inet_pton(AF_INET, argv[1], &ip) != 1) {
        fprintf(stderr, "error: invalid IPv4 address: %s\n", argv[1]);
        return 2;
    }
    char *end;
    long port = strtol(argv[2], &end, 10);
    if (*argv[2] == '\0' || *end != '\0' || port < 1 || port > 65535) {
        fprintf(stderr, "error: invalid port: %s\n", argv[2]);
        return 2;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in proxy_addr;
    memset(&proxy_addr, 0, sizeof(proxy_addr));
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(PROXYPORT);
    proxy_addr.sin_addr.s_addr = inet_addr(PROXY);
    if (connect(fd, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) < 0) {
        fprintf(stderr, "error: connect to tor proxy at %s:%d: %s\n", PROXY, PROXYPORT, strerror(errno));
        close(fd);
        return 1;
    }

    if (socks4_handshake(fd, ip.s_addr, htons((uint16_t)port)) < 0) {
        fprintf(stderr, "socks4 to %s:%ld failed: %s\n", argv[1], port, strerror(errno));
        close(fd);
        return 1;
    }
    printf("socks4 granted %s:%ld\n", argv[1], port);
    close(fd);
    return 0;
}
