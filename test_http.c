// test_http.c
//
// simple http client to test dyld_insert_libraries interception
// this is not a system binary, so it won't be blocked by SIP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {
    // we'll connect to cloudflare (1.1.1.1:80) to test
    const char *host_ip = "1.1.1.1";
    int port = 80;

    printf("test http client\n");
    printf("connecting to %s:%d (cloudflare)\n", host_ip, port);
    printf("(this should be intercepted by toralize)\n\n");

    // create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket failed");
        return 1;
    }

    // set up address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host_ip);

    // connect (this will be intercepted by toralize.dylib)
    printf("calling connect()...\n");
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        close(sockfd);
        return 1;
    }

    printf("connected successfully!\n\n");

    // send a simple http get request
    const char *request = "GET / HTTP/1.1\r\nHost: one.one.one.one\r\nConnection: close\r\n\r\n";
    if (send(sockfd, request, strlen(request), 0) < 0) {
        perror("send failed");
        close(sockfd);
        return 1;
    }

    printf("sent http request, waiting for response...\n\n");

    // receive response
    char buffer[4096];
    ssize_t received;
    while ((received = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[received] = '\0';
        printf("%s", buffer);
    }

    close(sockfd);
    return 0;
}

