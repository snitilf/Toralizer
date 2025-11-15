// test_socks4.c
//
// standalone socks4 client to test tor proxy connectivity
// this is phase 1 - proving the socks4 protocol implementation works

#include "toralize.h"

int main(int argc, char *argv[]) {
    // check command line arguments
    if (argc != 3) {
        fprintf(stderr, "usage: %s <destination_ip> <destination_port>\n", argv[0]);
        fprintf(stderr, "\nexample:\n");
        fprintf(stderr, "  %s 8.8.8.8 53\n", argv[0]);
        fprintf(stderr, "  %s 1.1.1.1 80\n", argv[0]);
        return 1;
    }

    // parse command line arguments
    char *dest_ip_str = argv[1];
    int dest_port_int = atoi(argv[2]);

    if (dest_port_int <= 0 || dest_port_int > 65535) {
        fprintf(stderr, "error: invalid port number: %s\n", argv[2]);
        return 1;
    }

    printf("=== socks4 test client ===\n");
    printf("destination: %s:%d\n", dest_ip_str, dest_port_int);
    printf("tor proxy: %s:%d\n\n", PROXY, PROXYPORT);

    // step 1: create a tcp socket
    printf("[1] creating tcp socket...\n");
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("error: socket creation failed");
        return 1;
    }
    printf("    socket created (fd=%d)\n\n", sockfd);

    // step 2: connect to tor proxy
    printf("[2] connecting to tor proxy at %s:%d...\n", PROXY, PROXYPORT);
    struct sockaddr_in proxy_addr;
    memset(&proxy_addr, 0, sizeof(proxy_addr));
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(PROXYPORT);
    proxy_addr.sin_addr.s_addr = inet_addr(PROXY);

    if (connect(sockfd, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) < 0) {
        perror("error: connection to tor proxy failed");
        printf("\nmake sure tor is running:\n");
        printf("  brew services start tor\n");
        close(sockfd);
        return 1;
    }
    printf("    connected to tor proxy\n\n");

    // step 3: build socks4 request
    printf("[3] building socks4 request...\n");
    socks4_request_t req;
    req.vn = 4;                                    // socks version 4
    req.cd = 1;                                    // command: connect
    req.dstport = htons(dest_port_int);            // destination port (network byte order)
    req.dstip = inet_addr(dest_ip_str);            // destination ip (network byte order)

    printf("    version: 4\n");
    printf("    command: 1 (connect)\n");
    printf("    dstport: %d (0x%04x in network order)\n", dest_port_int, req.dstport);
    printf("    dstip: %s (0x%08x in network order)\n", dest_ip_str, req.dstip);
    printf("\n");

    // step 4: send socks4 request
    printf("[4] sending socks4 request...\n");
    // send the fixed header
    ssize_t sent = send(sockfd, &req, sizeof(req), 0);
    if (sent != sizeof(req)) {
        perror("error: failed to send socks4 request header");
        close(sockfd);
        return 1;
    }
    printf("    sent %zd bytes (header)\n", sent);
    
    // send the username (null-terminated string)
    const char *username = USERNAME;
    ssize_t username_len = strlen(username) + 1; // include null terminator
    sent = send(sockfd, username, username_len, 0);
    if (sent != username_len) {
        perror("error: failed to send username");
        close(sockfd);
        return 1;
    }
    printf("    sent %zd bytes (username)\n", sent);
    printf("    total: %zu bytes\n\n", sizeof(req) + username_len);

    // step 5: receive socks4 response
    printf("[5] waiting for socks4 response...\n");
    socks4_response_t resp;
    ssize_t received = recv(sockfd, &resp, sizeof(resp), 0);
    if (received != sizeof(resp)) {
        perror("error: failed to receive socks4 response");
        close(sockfd);
        return 1;
    }
    printf("    received %zd bytes\n", received);
    printf("    response version: %d\n", resp.vn);
    printf("    response code: %d\n\n", resp.cd);

    // step 6: check response code
    printf("[6] checking response...\n");
    if (resp.cd == 90) {
        printf("    SUCCESS! response code 90 = request granted\n");
        printf("    connection established through tor\n\n");
        printf("=== test passed ===\n");
        printf("socks4 protocol implementation is working correctly\n");
        close(sockfd);
        return 0;
    } else {
        printf("    FAILED! response code %d = request denied\n\n", resp.cd);
        printf("socks4 error codes:\n");
        printf("  90 = request granted\n");
        printf("  91 = request rejected or failed\n");
        printf("  92 = request failed (no identd running)\n");
        printf("  93 = request failed (identd verification failed)\n");
        close(sockfd);
        return 1;
    }
}

