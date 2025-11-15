/**
 * toralize.c
 * 
 * Implementation of the SOCKS4 Tor proxy interceptor
 * This library intercepts connect() calls and routes them through Tor
 */

#include "toralize.h"

// Function pointer to the original connect() function
static int (*original_connect)(int, const struct sockaddr *, socklen_t) = NULL;

/**
 * Our custom connect() function that intercepts all connection attempts
 * This function will be called instead of the system's connect()
 */
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    // Get the original connect function if we haven't already
    if (!original_connect) {
        original_connect = dlsym(RTLD_NEXT, "connect");
        if (!original_connect) {
            fprintf(stderr, "[toralize] Error: Failed to get original connect()\n");
            errno = EACCES;
            return -1;
        }
    }

    // Only intercept IPv4 TCP connections
    if (addr->sa_family != AF_INET) {
        // For non-IPv4, just use the original connect
        return original_connect(sockfd, addr, addrlen);
    }

    // Cast to IPv4 address structure
    struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
    
    // Extract destination IP and port
    char *dest_ip = inet_ntoa(addr_in->sin_addr);
    unsigned short dest_port = ntohs(addr_in->sin_port);
    
    fprintf(stderr, "[toralize] Intercepting connection to %s:%d\n", dest_ip, dest_port);

    // Create a new socket for connecting to the Tor proxy
    int proxy_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (proxy_fd < 0) {
        perror("[toralize] socket");
        return -1;
    }

    // Set up the Tor proxy address
    struct sockaddr_in proxy_addr;
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(PROXYPORT);
    proxy_addr.sin_addr.s_addr = inet_addr(PROXY);

    // Connect to the Tor proxy using the original connect function
    if (original_connect(proxy_fd, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) < 0) {
        perror("[toralize] connect to proxy");
        close(proxy_fd);
        return -1;
    }

    fprintf(stderr, "[toralize] Connected to Tor proxy at %s:%d\n", PROXY, PROXYPORT);

    // Build the SOCKS4 request
    socks4_request_t req;
    req.vn = 4;                           // SOCKS version 4
    req.cd = CONNECT;                     // CONNECT command
    req.dstport = addr_in->sin_port;      // Destination port (already in network byte order)
    req.dstip = addr_in->sin_addr.s_addr; // Destination IP (already in network byte order)

    // Send the SOCKS4 request
    if (send(proxy_fd, &req, sizeof(req), 0) < 0) {
        perror("[toralize] send request");
        close(proxy_fd);
        return -1;
    }

    // Send the username (null-terminated)
    if (send(proxy_fd, USERNAME, strlen(USERNAME) + 1, 0) < 0) {
        perror("[toralize] send username");
        close(proxy_fd);
        return -1;
    }

    fprintf(stderr, "[toralize] Sent SOCKS4 request\n");

    // Receive the SOCKS4 response
    socks4_response_t resp;
    if (recv(proxy_fd, &resp, sizeof(resp), 0) < sizeof(resp)) {
        perror("[toralize] recv response");
        close(proxy_fd);
        return -1;
    }

    // Check if the connection was granted
    if (resp.cd != GRANTED) {
        fprintf(stderr, "[toralize] Error: Tor proxy denied connection (code %d)\n", resp.cd);
        close(proxy_fd);
        errno = ECONNREFUSED;
        return -1;
    }

    fprintf(stderr, "[toralize] ✓ Connection granted! Routing through Tor...\n");

    // Replace the original socket with our Tor-connected socket
    // This is the magic that makes the application use the Tor connection
    if (dup2(proxy_fd, sockfd) < 0) {
        perror("[toralize] dup2");
        close(proxy_fd);
        return -1;
    }

    close(proxy_fd);
    return 0;
}

