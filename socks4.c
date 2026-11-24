// socks4.c
//
// socks4 CONNECT handshake, shared by the dylib and the test client

#include "toralize.h"

int socks4_handshake(int fd, uint32_t dstip_be, uint16_t dstport_be) {
    // request: fixed header followed by the null-terminated user id, sent in one write
    unsigned char buf[sizeof(socks4_request_t) + sizeof(USERNAME)];
    socks4_request_t req = { .vn = 4, .cd = SOCKS4_CONNECT, .dstport = dstport_be, .dstip = dstip_be };
    memcpy(buf, &req, sizeof(req));
    memcpy(buf + sizeof(req), USERNAME, sizeof(USERNAME));
    ssize_t sent = send(fd, buf, sizeof(buf), 0);
    if (sent != (ssize_t)sizeof(buf)) {
        if (sent >= 0) {
            errno = EIO;
        }
        toralize_perror("send request");
        return -1;
    }
    TORALIZE_INFO("[toralize] sent socks4 request\n");

    // the reply is 8 bytes, MSG_WAITALL keeps a split reply from looking short
    socks4_response_t resp;
    ssize_t n = recv(fd, &resp, sizeof(resp), MSG_WAITALL);
    if (n != (ssize_t)sizeof(resp)) {
        int err = n < 0 ? errno : ECONNRESET;
        fprintf(stderr, "[toralize] error: no socks4 reply from tor proxy\n");
        errno = err;
        return -1;
    }
    if (resp.vn != 0) {
        fprintf(stderr, "[toralize] error: bad socks4 reply version %d\n", resp.vn);
        errno = EPROTO;
        return -1;
    }
    if (resp.cd != SOCKS4_GRANTED) {
        fprintf(stderr, "[toralize] error: tor proxy denied connection (code %d)\n", resp.cd);
        errno = ECONNREFUSED;
        return -1;
    }
    return 0;
}
