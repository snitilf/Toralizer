// toralize.c
//
// dyld interpose library that reroutes IPv4 TCP connect() calls through
// tor's SOCKS4 proxy (127.0.0.1:9050)
//
// routing policy:
//   AF_INET stream, non-loopback    -> tor (socks4 CONNECT)
//   AF_INET loopback / 0.0.0.0      -> real connect
//   AF_INET6 stream, loopback       -> real connect
//   AF_INET6 stream, anything else  -> refused with ENETUNREACH (fail closed)
//   non-stream sockets, other families, malformed arguments -> real connect

#include "toralize.h"
#include <fcntl.h>
#include <stddef.h>

// raw connect stub from libsystem_kernel, called directly so that no other
// interposer in the process sees the proxy's own connect or the pass-throughs
extern int __connect(int, const struct sockaddr *, socklen_t);

static int toralize_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

// dyld interpose table entry: dyld rewrites every binding of `replacee` to
// `replacement` in all loaded images except this one
#define DYLD_INTERPOSE(_replacement, _replacee) \
    __attribute__((used)) static struct { \
        const void* replacement; \
        const void* replacee; \
    } _interpose_##_replacee __attribute__((section("__DATA,__interpose"))) = { \
        (const void*)(unsigned long)&_replacement, \
        (const void*)(unsigned long)&_replacee \
    };

DYLD_INTERPOSE(toralize_connect, connect)

// close the proxy socket and return -1 with errno as the caller left it
// errno is saved across close() so close() cannot overwrite it
static int fail(int proxy_fd) {
    int err = errno;
    close(proxy_fd);
    errno = err;
    return -1;
}

// connect `sockfd` to dstip:dstport through tor
// dstip and dstport are in network byte order
static int connect_via_tor(int sockfd, uint32_t dstip, uint16_t dstport) {
    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &dstip, ipstr, sizeof(ipstr));
    fprintf(stderr, "[toralize] intercepting connection to %s:%d\n", ipstr, ntohs(dstport));

    // remember the caller's fd state, dup2 below replaces it with the proxy socket's
    int status_flags = fcntl(sockfd, F_GETFL);
    int fd_flags = fcntl(sockfd, F_GETFD);
    if (status_flags < 0 || fd_flags < 0) {
        return -1;
    }

    // the handshake runs on a fresh blocking socket
    int proxy_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (proxy_fd < 0) {
        toralize_perror("socket");
        return -1;
    }

    struct sockaddr_in proxy_addr;
    memset(&proxy_addr, 0, sizeof(proxy_addr));
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(PROXYPORT);
    proxy_addr.sin_addr.s_addr = inet_addr(PROXY);

    if (__connect(proxy_fd, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) < 0) {
        toralize_perror("connect to proxy");
        return fail(proxy_fd);
    }
    fprintf(stderr, "[toralize] connected to tor proxy at %s:%d\n", PROXY, PROXYPORT);

    if (socks4_handshake(proxy_fd, dstip, dstport) < 0) {
        return fail(proxy_fd);
    }
    fprintf(stderr, "[toralize] connection granted, routing through tor\n");

    // put the tor-connected socket under the caller's fd number
    if (dup2(proxy_fd, sockfd) < 0) {
        toralize_perror("dup2");
        return fail(proxy_fd);
    }
    close(proxy_fd);

    // dup2 gave the fd the proxy socket's flags, put back the caller's
    // (O_NONBLOCK in particular) and its close-on-exec setting
    if (fcntl(sockfd, F_SETFL, status_flags) < 0 || fcntl(sockfd, F_SETFD, fd_flags) < 0) {
        // no fail() here: the fd already is the tor socket and the caller keeps it
        toralize_perror("restore fd flags");
        return -1;
    }
    return 0;
}

static int is_loopback_v4(uint32_t ip_be) {
    uint32_t ip = ntohl(ip_be);
    return (ip >> 24) == 127 || ip == INADDR_ANY;
}

static int toralize_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    // anything we cannot classify goes to the real connect, which reports the error itself
    if (addr == NULL || addrlen < offsetof(struct sockaddr, sa_data)) {
        return __connect(sockfd, addr, addrlen);
    }
    if (addr->sa_family != AF_INET && addr->sa_family != AF_INET6) {
        return __connect(sockfd, addr, addrlen);
    }

    // only stream sockets are proxied, UDP "connect" and the like pass through
    int type;
    socklen_t type_len = sizeof(type);
    if (getsockopt(sockfd, SOL_SOCKET, SO_TYPE, &type, &type_len) < 0 || type != SOCK_STREAM) {
        return __connect(sockfd, addr, addrlen);
    }

    if (addr->sa_family == AF_INET) {
        if (addrlen < sizeof(struct sockaddr_in)) {
            return __connect(sockfd, addr, addrlen);
        }
        const struct sockaddr_in *in = (const struct sockaddr_in *)addr;
        if (is_loopback_v4(in->sin_addr.s_addr)) {
            return __connect(sockfd, addr, addrlen);
        }
        return connect_via_tor(sockfd, in->sin_addr.s_addr, in->sin_port);
    }

    // AF_INET6: tor's SOCKS4 cannot carry it, so never connect directly
    if (addrlen < sizeof(struct sockaddr_in6)) {
        return __connect(sockfd, addr, addrlen);
    }
    const struct sockaddr_in6 *in6 = (const struct sockaddr_in6 *)addr;
    if (IN6_IS_ADDR_LOOPBACK(&in6->sin6_addr)) {
        return __connect(sockfd, addr, addrlen);
    }
    if (IN6_IS_ADDR_V4MAPPED(&in6->sin6_addr)) {
        uint32_t mapped;
        memcpy(&mapped, &in6->sin6_addr.s6_addr[12], sizeof(mapped));
        if (is_loopback_v4(mapped)) {
            return __connect(sockfd, addr, addrlen);
        }
    }
    fprintf(stderr, "[toralize] refusing IPv6 connection (tor socks4 is IPv4 only)\n");
    errno = ENETUNREACH;
    return -1;
}
