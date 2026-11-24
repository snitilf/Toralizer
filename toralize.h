// toralize.h
//
// header file for Toralizer - tool to route IPv4 TCP through Tor
// uses DYLD_INSERT_LIBRARIES on macOS to intercept connect() calls

#ifndef TORALIZE_H
#define TORALIZE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdint.h>

// tor proxy configuration
#define PROXY "127.0.0.1"
#define PROXYPORT 9050

// socks4 protocol constants
#define SOCKS4_CONNECT 1
#define SOCKS4_GRANTED 90

// username for socks4 (can be empty)
#define USERNAME "toralizer"

// socks4 request structure
// sent to tor proxy to request a connection
typedef struct {
    uint8_t vn;       // socks version (4)
    uint8_t cd;       // command code (1 = CONNECT)
    uint16_t dstport; // destination port (network byte order)
    uint32_t dstip;   // destination IP (network byte order)
    // followed by: username (null-terminated string)
} __attribute__((packed)) socks4_request_t;

// socks4 response structure
// received from tor proxy after connection request
typedef struct {
    uint8_t vn;              // reply version (0)
    uint8_t cd;              // reply code (90 = granted)
    uint16_t dstport_unused; // ignored
    uint32_t dstip_unused;   // ignored
} __attribute__((packed)) socks4_response_t;

_Static_assert(sizeof(socks4_request_t) == 8, "socks4 request header is 8 bytes on the wire");
_Static_assert(sizeof(socks4_response_t) == 8, "socks4 reply is 8 bytes on the wire");

// informational log lines, silenced by TORALIZE_QUIET (any value except empty or "0")
// errors and refusals always print
static inline int toralize_quiet(void) {
    const char *v = getenv("TORALIZE_QUIET");
    return v != NULL && v[0] != '\0' && strcmp(v, "0") != 0;
}
#define TORALIZE_INFO(...) \
    do { \
        if (!toralize_quiet()) { \
            fprintf(stderr, __VA_ARGS__); \
        } \
    } while (0)

// port of the tor proxy: PROXYPORT, or TORALIZE_PROXY_PORT when set (9150 for Tor Browser)
// returns 0 when the variable is set to something that is not a port number
static inline int toralize_proxy_port(void) {
    const char *v = getenv("TORALIZE_PROXY_PORT");
    if (v == NULL || v[0] == '\0') {
        return PROXYPORT;
    }
    char *end;
    long port = strtol(v, &end, 10);
    return (*end == '\0' && port >= 1 && port <= 65535) ? (int)port : 0;
}

// log "[toralize] <what>: <strerror(errno)>" to stderr, errno is left as it was
static inline void toralize_perror(const char *what) {
    int err = errno;
    fprintf(stderr, "[toralize] %s: %s\n", what, strerror(err));
    errno = err;
}

// socks4.c: run the socks4 CONNECT handshake on `fd`, already connected to the proxy
// dstip_be and dstport_be are the destination in network byte order
// returns 0 once the proxy grants the request, then `fd` carries the tunneled stream
// returns -1 with errno set on any failure, and leaves `fd` open for the caller to close
// failures are logged to stderr with the "[toralize]" prefix
int socks4_handshake(int fd, uint32_t dstip_be, uint16_t dstport_be);

#endif // TORALIZE_H

