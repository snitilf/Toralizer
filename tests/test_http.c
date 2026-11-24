// test_http.c
//
// plain http client for the interception tests: GET / from 1.1.1.1:80
// prints the response and exits 0 when one arrives, so it works with and without toralize

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(void) {
    const char *host_ip = "1.1.1.1";
    const uint16_t port = 80;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host_ip);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "connect %s:%u: %s\n", host_ip, port, strerror(errno));
        close(sockfd);
        return 1;
    }
    printf("connected to %s:%u\n", host_ip, port);

    const char *request = "GET / HTTP/1.1\r\nHost: one.one.one.one\r\nConnection: close\r\n\r\n";
    if (send(sockfd, request, strlen(request), 0) < 0) {
        perror("send");
        close(sockfd);
        return 1;
    }

    char buffer[4096];
    ssize_t received;
    ssize_t total = 0;
    while ((received = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[received] = '\0';
        printf("%s", buffer);
        total += received;
    }
    close(sockfd);

    // no response, or a recv error, is a failed test
    if (received < 0) {
        perror("recv");
        return 1;
    }
    if (total == 0) {
        fprintf(stderr, "no response received\n");
        return 1;
    }
    return 0;
}
