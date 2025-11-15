/**
 * toralize.h
 * 
 * Header file for Toralizer - A tool to route network traffic through Tor
 * Uses DYLD_INSERT_LIBRARIES on macOS to intercept connect() calls
 */

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

// Tor proxy configuration
#define PROXY "127.0.0.1"
#define PROXYPORT 9050

// SOCKS4 protocol constants
#define CONNECT 1
#define GRANTED 90

// Username for SOCKS4 (can be empty)
#define USERNAME "toralizer"

/**
 * SOCKS4 request structure
 * Sent to Tor proxy to request a connection
 */
typedef struct {
    unsigned char vn;       // SOCKS version (4)
    unsigned char cd;       // Command code (1 = CONNECT)
    unsigned short dstport; // Destination port (network byte order)
    unsigned int dstip;     // Destination IP (network byte order)
    // Followed by: username (null-terminated string)
} __attribute__((packed)) socks4_request_t;

/**
 * SOCKS4 response structure
 * Received from Tor proxy after connection request
 */
typedef struct {
    unsigned char vn;       // Reply version (0)
    unsigned char cd;       // Reply code (90 = granted)
    unsigned short _;       // Ignored
    unsigned int __;        // Ignored
} __attribute__((packed)) socks4_response_t;

#endif // TORALIZE_H

