// toralize.c
// 
// implementation of the socks4 tor proxy interceptor
// this library intercepts connect() calls and routes them through tor

#include "toralize.h"

// function pointer to the original connect() function
static int (*original_connect)(int, const struct sockaddr *, socklen_t) = NULL;

// our custom connect() function that intercepts all connection attempts
// this function will be called instead of the system's connect()
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    // get the original connect function if we haven't already
    if (!original_connect) {
        original_connect = dlsym(RTLD_NEXT, "connect");
        if (!original_connect) {
            fprintf(stderr, "[toralize] error: failed to get original connect()\n");
            errno = EACCES;
            return -1;
        }
    }

    // only intercept ipv4 tcp connections
    if (addr->sa_family != AF_INET) {
        // for non-ipv4, just use the original connect
        return original_connect(sockfd, addr, addrlen);
    }

    // cast to ipv4 address structure
    struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
    
    // extract destination ip and port
    char *dest_ip = inet_ntoa(addr_in->sin_addr);
    unsigned short dest_port = ntohs(addr_in->sin_port);
    
    fprintf(stderr, "[toralize] intercepting connection to %s:%d\n", dest_ip, dest_port);

    // create a new socket for connecting to the tor proxy
    int proxy_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (proxy_fd < 0) {
        perror("[toralize] socket");
        return -1;
    }

    // set up the tor proxy address
    struct sockaddr_in proxy_addr;
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(PROXYPORT);
    proxy_addr.sin_addr.s_addr = inet_addr(PROXY);

    // connect to the tor proxy using the original connect function
    if (original_connect(proxy_fd, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) < 0) {
        perror("[toralize] connect to proxy");
        close(proxy_fd);
        return -1;
    }

    fprintf(stderr, "[toralize] connected to tor proxy at %s:%d\n", PROXY, PROXYPORT);

    // build the socks4 request
    socks4_request_t req;
    req.vn = 4;                           // socks version 4
    req.cd = CONNECT;                     // connect command
    req.dstport = addr_in->sin_port;      // destination port (already in network byte order)
    req.dstip = addr_in->sin_addr.s_addr; // destination ip (already in network byte order)

    // send the socks4 request
    if (send(proxy_fd, &req, sizeof(req), 0) < 0) {
        perror("[toralize] send request");
        close(proxy_fd);
        return -1;
    }

    // send the username (null-terminated)
    if (send(proxy_fd, USERNAME, strlen(USERNAME) + 1, 0) < 0) {
        perror("[toralize] send username");
        close(proxy_fd);
        return -1;
    }

    fprintf(stderr, "[toralize] sent socks4 request\n");

    // receive the socks4 response
    socks4_response_t resp;
    if (recv(proxy_fd, &resp, sizeof(resp), 0) < sizeof(resp)) {
        perror("[toralize] recv response");
        close(proxy_fd);
        return -1;
    }

    // check if the connection was granted
    if (resp.cd != GRANTED) {
        fprintf(stderr, "[toralize] error: tor proxy denied connection (code %d)\n", resp.cd);
        close(proxy_fd);
        errno = ECONNREFUSED;
        return -1;
    }

    fprintf(stderr, "[toralize] connection granted, routing through tor\n");

    // replace the original socket with our tor-connected socket
    // this is the magic that makes the application use the tor connection
    if (dup2(proxy_fd, sockfd) < 0) {
        perror("[toralize] dup2");
        close(proxy_fd);
        return -1;
    }

    close(proxy_fd);
    return 0;
}

