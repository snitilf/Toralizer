// test_routing.c
//
// checks the routing table from the README against the real dylib, without tor and without network
// run it through the wrapper (`make test-routing`) so toralize.dylib is injected
//
// a mock socks4 proxy on an ephemeral 127.0.0.1 port plays tor, and TORALIZE_PROXY_PORT points the
// dylib at it, so a running tor on 9050 does not matter
// "remote" destinations are the TEST-NET addresses 192.0.2.1 and 2001:db8::1, nothing ever reaches them

#include "toralize.h"
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>

#define DST_IP "192.0.2.1"
#define DST_PORT 8080

enum mock_mode { GRANT, DENY, SHORT_REPLY, BAD_VERSION, NO_MOCK };

static int passed;
static int failed;

static void check(const char *name, int ok) {
    printf("%s: %s\n", ok ? "pass" : "FAIL", name);
    if (ok) {
        passed++;
    } else {
        failed++;
    }
}

static void on_alarm(int sig) {
    (void)sig;
    static const char msg[] = "FAIL: timed out, is toralize.dylib injected?\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    _exit(1);
}

// listening socket on 127.0.0.1 or ::1 with an ephemeral port, returns the fd or -1
static int listen_on(int family, int *port) {
    int fd = socket(family, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    struct sockaddr_storage ss;
    memset(&ss, 0, sizeof(ss));
    socklen_t len;
    if (family == AF_INET) {
        struct sockaddr_in *a = (struct sockaddr_in *)&ss;
        a->sin_family = AF_INET;
        a->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        len = sizeof(*a);
    } else {
        struct sockaddr_in6 *a = (struct sockaddr_in6 *)&ss;
        a->sin6_family = AF_INET6;
        a->sin6_addr = in6addr_loopback;
        len = sizeof(*a);
    }
    if (bind(fd, (struct sockaddr *)&ss, len) < 0 || listen(fd, 4) < 0 ||
        getsockname(fd, (struct sockaddr *)&ss, &len) < 0) {
        close(fd);
        return -1;
    }
    *port = ntohs(family == AF_INET ? ((struct sockaddr_in *)&ss)->sin_port
                                    : ((struct sockaddr_in6 *)&ss)->sin6_port);
    return fd;
}

static struct sockaddr_in v4(const char *ip, int port) {
    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_port = htons((uint16_t)port);
    inet_pton(AF_INET, ip, &a.sin_addr);
    return a;
}

static struct sockaddr_in6 v6(const char *ip, int port) {
    struct sockaddr_in6 a;
    memset(&a, 0, sizeof(a));
    a.sin6_family = AF_INET6;
    a.sin6_port = htons((uint16_t)port);
    inet_pton(AF_INET6, ip, &a.sin6_addr);
    return a;
}

// the mock proxy: serves one connection on `lfd` and exits 0 only if it got a valid request
// for DST_IP:DST_PORT with the toralizer user id
static void mock_proxy(int lfd, enum mock_mode mode) {
    alarm(10);
    int fd = accept(lfd, NULL, NULL);
    if (fd < 0) {
        _exit(1);
    }
    unsigned char req[sizeof(socks4_request_t) + sizeof(USERNAME)];
    int ok = recv(fd, req, sizeof(req), MSG_WAITALL) == (ssize_t)sizeof(req);
    if (ok) {
        socks4_request_t hdr;
        memcpy(&hdr, req, sizeof(hdr));
        struct sockaddr_in want = v4(DST_IP, DST_PORT);
        ok = hdr.vn == 4 && hdr.cd == SOCKS4_CONNECT && hdr.dstport == want.sin_port &&
             hdr.dstip == want.sin_addr.s_addr && memcmp(req + sizeof(hdr), USERNAME, sizeof(USERNAME)) == 0;
    }
    socks4_response_t resp = { .vn = 0, .cd = SOCKS4_GRANTED };
    size_t resp_len = sizeof(resp);
    if (mode == DENY) {
        resp.cd = 91;
    } else if (mode == SHORT_REPLY) {
        resp_len = 4;
    } else if (mode == BAD_VERSION) {
        resp.vn = 1;
    }
    send(fd, &resp, resp_len, 0);
    if (mode == GRANT) {
        char buf[64];
        ssize_t n;
        while ((n = recv(fd, buf, sizeof(buf), 0)) > 0) {
            send(fd, buf, (size_t)n, 0);
        }
    }
    close(fd);
    _exit(ok ? 0 : 1);
}

// a fresh mock proxy: the listener is open before the fork, so the client cannot race it
struct proxy {
    int lfd;
    pid_t pid;
};

static struct proxy proxy_start(enum mock_mode mode) {
    struct proxy p = { -1, -1 };
    int port;
    p.lfd = listen_on(AF_INET, &port);
    if (p.lfd < 0) {
        perror("mock proxy");
        exit(1);
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", port);
    setenv("TORALIZE_PROXY_PORT", buf, 1);
    if (mode != NO_MOCK) {
        fflush(stdout);
        p.pid = fork();
        if (p.pid == 0) {
            mock_proxy(p.lfd, mode);
        }
    }
    return p;
}

// stops the mock, returns 1 if it validated the request (or if there was no mock to ask)
static int proxy_stop(struct proxy p) {
    int ok = 1;
    if (p.pid > 0) {
        int status;
        waitpid(p.pid, &status, 0);
        ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
    close(p.lfd);
    return ok;
}

// 1 if nothing has connected to the proxy listener
static int proxy_untouched(struct proxy p) {
    fcntl(p.lfd, F_SETFL, O_NONBLOCK);
    int fd = accept(p.lfd, NULL, NULL);
    if (fd >= 0) {
        close(fd);
        return 0;
    }
    return errno == EAGAIN;
}

// connect `fd` (any family) and return -1/errno as seen by the caller
static int do_connect(int fd, const void *addr, socklen_t len) {
    errno = 0;
    return connect(fd, (const struct sockaddr *)addr, len);
}

// one proxied connect against a mock in `mode`, expects `want_errno` (0 for a grant)
static void proxied(const char *name, enum mock_mode mode, int want_errno) {
    struct proxy p = proxy_start(mode);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in dst = v4(DST_IP, DST_PORT);
    int rc = do_connect(fd, &dst, sizeof(dst));
    int err = errno;
    int ok = want_errno == 0 ? rc == 0 : (rc == -1 && err == want_errno);
    if (ok && want_errno == 0) {
        char buf[8] = "";
        ok = send(fd, "ping", 4, 0) == 4 && recv(fd, buf, 4, MSG_WAITALL) == 4 && memcmp(buf, "ping", 4) == 0;
    }
    close(fd);
    ok = proxy_stop(p) && ok;
    check(name, ok);
}

// flags before and after a granted connect
static void flags_case(const char *name, int nonblock, int cloexec) {
    struct proxy p = proxy_start(GRANT);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (nonblock) {
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    }
    if (cloexec) {
        fcntl(fd, F_SETFD, FD_CLOEXEC);
    }
    struct sockaddr_in dst = v4(DST_IP, DST_PORT);
    int rc = do_connect(fd, &dst, sizeof(dst));
    int ok = rc == 0 && !!(fcntl(fd, F_GETFL) & O_NONBLOCK) == nonblock &&
             !!(fcntl(fd, F_GETFD) & FD_CLOEXEC) == cloexec;
    close(fd);
    check(name, proxy_stop(p) && ok);
}

// a connect that must not touch the proxy and must reach its real target
static void direct(const char *name, int fd, const void *addr, socklen_t len, int target_lfd) {
    struct proxy p = proxy_start(NO_MOCK);
    int rc = do_connect(fd, addr, len);
    int err = errno;
    int ok = rc == 0 && proxy_untouched(p);
    if (ok && target_lfd >= 0) {
        int peer = accept(target_lfd, NULL, NULL);
        ok = peer >= 0;
        if (peer >= 0) {
            close(peer);
        }
    }
    if (rc != 0) {
        printf("  connect failed: %s\n", strerror(err));
    }
    close(fd);
    proxy_stop(p);
    check(name, ok);
}

// a refused IPv6 connect, no proxy traffic
static void refused_v6(const char *name, const char *ip) {
    struct proxy p = proxy_start(NO_MOCK);
    int fd = socket(AF_INET6, SOCK_STREAM, 0);
    struct sockaddr_in6 dst = v6(ip, DST_PORT);
    int rc = do_connect(fd, &dst, sizeof(dst));
    int ok = rc == -1 && errno == ENETUNREACH && proxy_untouched(p);
    close(fd);
    proxy_stop(p);
    check(name, ok);
}

int main(void) {
    const char *inserted = getenv("DYLD_INSERT_LIBRARIES");
    if (inserted == NULL || strstr(inserted, "toralize.dylib") == NULL) {
        fprintf(stderr, "run through the wrapper: ./toralize ./tests/test_routing (or make test-routing)\n");
        return 2;
    }
    signal(SIGALRM, on_alarm);
    signal(SIGPIPE, SIG_IGN);
    alarm(60);

    // the dylib logs refusals and failures to stderr, keep them for a failing run only
    FILE *log = tmpfile();
    int real_stderr = dup(STDERR_FILENO);
    if (log != NULL) {
        dup2(fileno(log), STDERR_FILENO);
    }

    // IPv4 TCP, not loopback: through the proxy
    proxied("ipv4 tcp goes through the proxy, request is valid, data flows", GRANT, 0);
    proxied("proxy code 91 gives ECONNREFUSED", DENY, ECONNREFUSED);
    proxied("short proxy reply gives ECONNRESET", SHORT_REPLY, ECONNRESET);
    proxied("bad proxy reply version gives EPROTO", BAD_VERSION, EPROTO);
    flags_case("O_NONBLOCK and FD_CLOEXEC are restored", 1, 1);
    flags_case("clear flags stay clear", 0, 0);

    // no proxy listening: a port that was just closed
    {
        struct proxy p = proxy_start(NO_MOCK);
        close(p.lfd);
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in dst = v4(DST_IP, DST_PORT);
        int rc = do_connect(fd, &dst, sizeof(dst));
        check("no proxy listening gives ECONNREFUSED, never goes direct", rc == -1 && errno == ECONNREFUSED);
        close(fd);
    }

    // loopback IPv4 and IPv6: direct, with a real listener to receive it
    {
        int port;
        int lfd = listen_on(AF_INET, &port);
        struct sockaddr_in a = v4("127.0.0.1", port);
        direct("ipv4 loopback goes direct", socket(AF_INET, SOCK_STREAM, 0), &a, sizeof(a), lfd);

        a = v4("0.0.0.0", port);
        direct("0.0.0.0 goes direct", socket(AF_INET, SOCK_STREAM, 0), &a, sizeof(a), lfd);

        struct sockaddr_in6 m = v6("::ffff:127.0.0.1", port);
        direct("ipv4-mapped loopback goes direct", socket(AF_INET6, SOCK_STREAM, 0), &m, sizeof(m), lfd);
        close(lfd);

        lfd = listen_on(AF_INET6, &port);
        if (lfd >= 0) {
            struct sockaddr_in6 l = v6("::1", port);
            direct("::1 goes direct", socket(AF_INET6, SOCK_STREAM, 0), &l, sizeof(l), lfd);
            close(lfd);
        } else {
            printf("skip: ::1 goes direct (no IPv6 loopback)\n");
        }
    }

    // AF_UNIX: direct
    {
        struct sockaddr_un un;
        memset(&un, 0, sizeof(un));
        un.sun_family = AF_UNIX;
        snprintf(un.sun_path, sizeof(un.sun_path), "/tmp/toralize-routing-%d.sock", (int)getpid());
        unlink(un.sun_path);
        int lfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (bind(lfd, (struct sockaddr *)&un, sizeof(un)) < 0 || listen(lfd, 1) < 0) {
            perror("unix listener");
            return 1;
        }
        direct("AF_UNIX goes direct", socket(AF_UNIX, SOCK_STREAM, 0), &un, sizeof(un), lfd);
        close(lfd);
        unlink(un.sun_path);
    }

    // UDP: connect() only sets the peer, the proxy must not be involved
    {
        struct proxy p = proxy_start(NO_MOCK);
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        struct sockaddr_in dst = v4(DST_IP, 53);
        int rc = do_connect(fd, &dst, sizeof(dst));
        int ok = proxy_untouched(p) && (rc == 0 || errno == ENETUNREACH || errno == EHOSTUNREACH);
        close(fd);
        proxy_stop(p);
        check("udp goes direct", ok);
    }

    // IPv6 TCP, not loopback: refused
    refused_v6("ipv6 tcp is refused with ENETUNREACH", "2001:db8::1");
    refused_v6(":: is refused with ENETUNREACH", "::");
    refused_v6("ipv4-mapped non-loopback is refused with ENETUNREACH", "::ffff:192.0.2.1");

    printf("%d/%d routing checks passed\n", passed, passed + failed);
    if (failed > 0 && log != NULL) {
        fflush(log);
        rewind(log);
        char buf[512];
        size_t n;
        printf("dylib stderr:\n");
        fflush(stdout);
        while ((n = fread(buf, 1, sizeof(buf), log)) > 0) {
            write(real_stderr, buf, n);
        }
    }
    return failed == 0 ? 0 : 1;
}
