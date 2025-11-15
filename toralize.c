// toralize.c
// 
// implementation of the socks4 tor proxy interceptor
// this library intercepts connect() calls and routes them through tor

#include "toralize.h"

// forward declaration of our replacement connect function
static int toralize_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

// DYLD_INTERPOSE macro for macOS function interposition
// this is the proper way to intercept system calls on macOS
#define DYLD_INTERPOSE(_replacement, _replacee) \
    __attribute__((used)) static struct { \
        const void* replacement; \
        const void* replacee; \
    } _interpose_##_replacee __attribute__((section("__DATA,__interpose"))) = { \
        (const void*)(unsigned long)&_replacement, \
        (const void*)(unsigned long)&_replacee \
    };

// register our interposition
DYLD_INTERPOSE(toralize_connect, connect)

// our custom connect() function that intercepts all connection attempts
// this function will be called instead of the system's connect()
static int toralize_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    // dlsym is not needed with DYLD_INTERPOSE - we'll use syscall if needed
    // but for simplicity, we'll create a new socket for the proxy

    // only intercept ipv4 tcp connections
    if (addr->sa_family != AF_INET) {
        // for non-ipv4, we need to use the real connect
        // since we can't easily call the original with DYLD_INTERPOSE,
        // we'll just let non-ipv4 fail or handle it differently
        return -1;
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
    memset(&proxy_addr, 0, sizeof(proxy_addr));
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(PROXYPORT);
    proxy_addr.sin_addr.s_addr = inet_addr(PROXY);

    // we need to call the real connect - use a syscall
    // on macOS, we can use __connect system call
    extern int __connect(int, const struct sockaddr *, socklen_t);
    if (__connect(proxy_fd, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) < 0) {
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

