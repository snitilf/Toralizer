// toralize.h
// 
// header file for Toralizer - tool to route network traffic through Tor
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
#include <dlfcn.h>
#include <errno.h>

// tor proxy configuration
#define PROXY "127.0.0.1"
#define PROXYPORT 9050

// socks4 protocol constants
#define CONNECT 1
#define GRANTED 90

// username for socks4 (can be empty)
#define USERNAME "toralizer"

// socks4 request structure
// sent to tor proxy to request a connection
typedef struct {
    unsigned char vn;       // socks version (4)
    unsigned char cd;       // command code (1 = CONNECT)
    unsigned short dstport; // destination port (network byte order)
    unsigned int dstip;     // destination IP (network byte order)
    // followed by: username (null-terminated string)
} __attribute__((packed)) socks4_request_t;

// socks4 response structure
// received from tor proxy after connection request
typedef struct {
    unsigned char vn;       // reply version (0)
    unsigned char cd;       // reply code (90 = granted)
    unsigned short _;       // ignored
    unsigned int __;        // ignored
} __attribute__((packed)) socks4_response_t;

#endif // TORALIZE_H

