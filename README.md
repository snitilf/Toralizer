<div align="center">

# Toralizer

### Reroute a program's IPv4 TCP through Tor, without touching the program.

[Demo](#demo) · [How it works](#how-it-works) · [Quick start](#quick-start) · [Limits](#limits) · [Tests](#tests)

</div>

Toralizer is a macOS dynamic library written in C.
It hooks `connect()` through a dyld interpose entry and reroutes the IPv4 TCP connections of a command-line program through Tor, using a SOCKS4 handshake written by hand.
You inject it with `DYLD_INSERT_LIBRARIES`, so the program is never recompiled or changed.

Three limits come first:

- SIP strips `DYLD_*` variables from system binaries, so `/usr/bin/curl` is not rerouted.
  Use a Homebrew build or a program you compiled yourself.
- IPv4 TCP only.
  Non-loopback IPv6 TCP is refused, and UDP and Unix sockets are left alone.
- SOCKS4 has no hostnames, so the program resolves them itself, outside Tor.
  DNS lookups leak, and this is not a privacy tool.

The whole hook is `toralize.c`, under 200 lines.
It needs a Tor daemon on `127.0.0.1:9050`.

## Demo

```text
$ /opt/homebrew/opt/curl/bin/curl -4 -s https://check.torproject.org/api/ip
{"IsTor":false,"IP":"204.x.x.x"}

$ ./toralize /opt/homebrew/opt/curl/bin/curl -4 -s https://check.torproject.org/api/ip
[toralize] intercepting connection to 116.202.120.181:443
[toralize] connected to tor proxy at 127.0.0.1:9050
[toralize] sent socks4 request
[toralize] connection granted, routing through tor
{"IsTor":true,"IP":"147.90.235.22"}

$ ./toralize /usr/bin/curl -4 -s https://check.torproject.org/api/ip
{"IsTor":false,"IP":"204.x.x.x"}
```

The first run is the real address, masked here.
The second run goes through Tor and comes out of an exit relay.
The exit address changes between runs.
The `[toralize]` lines are the hook logging to stderr.
In the third run SIP removes the injected library from the system curl, so nothing is rerouted and the real address comes back.

Run on arm64 with Homebrew curl 8.17.0.

## How it works

1. `./toralize` sets `DYLD_INSERT_LIBRARIES` to `toralize.dylib` and runs your command.
   dyld loads the library into the process before `main`.
2. The library has an entry in its `__interpose` section that pairs `toralize_connect` with `connect`.
   dyld rebinds every call to `connect()` in the other loaded images to it.
3. For a non-loopback IPv4 TCP address, `toralize_connect` opens a fresh socket to Tor at `127.0.0.1:9050`.
   It sends a SOCKS4 CONNECT request: version 4, command 1, the destination port and address, and the user id `toralizer`.
4. It reads the 8-byte reply and accepts only version 0 with code 90.
5. It `dup2()`s the proxy socket onto the caller's file descriptor, then restores the caller's `O_NONBLOCK` and `FD_CLOEXEC`.
   The program keeps using the descriptor it already had, which now talks to the destination through Tor.

```text
program                   toralize.dylib                     tor
   |                            |                             |
   | connect(fd, dst:443)       |                             |
   |--------------------------->| socket() + __connect()      |
   |                            |---------------------------->| 127.0.0.1:9050
   |                            | 04 01 port ip "toralizer"   |
   |                            |---------------------------->|
   |                            | 00 5a ...                   |
   |                            |<----------------------------|
   |                            | dup2(proxy_fd, fd)          |
   |<---------------------------| return 0                    |
   | read/write(fd) ... carried through tor to dst:443        |
```

Some details:

- The request and reply are packed structs in `toralize.h`, with no SOCKS library.
- The replacement calls the raw `__connect` symbol from libsystem_kernel, both for the proxy socket and for connections it lets through, so no other interposer in the process sees them.
- Every failure path closes the proxy socket and sets `errno` before it returns `-1`.
- The entry is visible in the built library:

  ```text
  $ nm -m toralize.dylib | grep __interpose
  0000000000004080 (__DATA_CONST,__interpose) non-external __interpose_connect
  ```

What `connect()` does with each kind of destination:

| Destination | Result |
| --- | --- |
| IPv4 TCP, not loopback | Through Tor |
| IPv4 loopback (`127.0.0.0/8`, `0.0.0.0`), `::1`, IPv4-mapped loopback | Direct, untouched |
| IPv6 TCP, not loopback, including `::` and IPv4-mapped addresses that are not loopback | Refused with `ENETUNREACH` |
| UDP, `AF_UNIX`, any other family or socket type | Direct, untouched |
| IPv4 TCP while Tor is not running | Fails with `ECONNREFUSED`, never goes direct |

IPv6 is refused instead of passed through so a dual-stack program cannot leave around Tor by accident.
curl falls back to IPv4 on its own.

## Quick start

You need macOS with the Xcode Command Line Tools and [Homebrew](https://brew.sh).
The library is built for the host architecture, and it has only been run on arm64.

1. Install Tor and a Homebrew curl, and start Tor:

   ```sh
   brew install tor curl
   brew services start tor
   ```

   Tor needs a moment to bootstrap before port 9050 works.

2. Build the library:

   ```sh
   git clone https://github.com/snitilf/Toralizer.git
   cd Toralizer
   make
   ```

3. Run a program through it:

   ```sh
   ./toralize $(brew --prefix curl)/bin/curl -4 -s https://check.torproject.org/api/ip
   ```

   The answer has `"IsTor":true`.
   `-4` keeps curl on IPv4.

Other ways to run it:

```sh
# your real address, then the one through Tor
make test

# one of your own binaries, no Homebrew curl needed
make clients && ./toralize ./test_http

# what the wrapper does
DYLD_INSERT_LIBRARIES="$PWD/toralize.dylib" ./your-program
```

If you see `[toralize] connect to proxy: Connection refused`, Tor is not listening.
Check with `lsof -nP -iTCP:9050 -sTCP:LISTEN`.

## Limits

Besides the three above:

- The injected library is also dropped for anything started through a system binary, such as `/bin/sh -c`, and for binaries with the hardened runtime.
- Only `connect()` is hooked.
  A program that connects with `connectx()` or Network.framework is not rerouted.
- Private and LAN IPv4 addresses go to Tor too, and Tor refuses them.
- The user id is fixed, and Tor isolates streams by SOCKS user id.
  Until Tor rotates the circuit, all toralize traffic leaves from one exit address, which some sites rate-limit.
- Socket options set before `connect()` are lost, because the descriptor is replaced by the proxy socket.
- The handshake is blocking and has no timeout, even for a non-blocking socket.
- The host architecture only, with no universal binary.

## Tests

| Script | Run with | Checks | Result |
| --- | --- | --- | --- |
| `tests/check_env.sh` | `make check-env` | Xcode tools, Homebrew, clang, make, Tor installed and listening, SOCKS proxy reachable, project files, wrapper executable | 15/15 |
| `tests/test_socks4.sh` | `make test-socks4` | The standalone SOCKS4 client reaches 8.8.8.8:53, 1.1.1.1:53 and 1.1.1.1:80 through Tor | 3/3 |
| `tests/test_intercept.sh` | `make test-intercept` | A direct run of `test_http` works, then the library intercepts it, connects to the proxy, gets a granted reply, the HTTP request succeeds, and `/usr/bin/curl` is not intercepted under SIP | 6/6 |

The results are from one run with Tor up, on arm64.
The tests are shell scripts that count their own checks, not a unit test framework.
All three need a running Tor.
CI builds the library with `-Werror` on macOS, checks the interpose entry, and checks that the hook logs an interception without Tor.
It does not run these scripts.

`make clients` builds the two test clients without Tor.

## Project layout

```text
toralize.c          the interposed connect() and the SOCKS4 handshake
toralize.h          proxy address, SOCKS4 constants, packed request and reply structs
toralize            wrapper that sets DYLD_INSERT_LIBRARIES and execs your command
Makefile            builds toralize.dylib and the two test clients, runs the tests
test_socks4.c       standalone SOCKS4 client, used by `make test-socks4`
test_http.c         plain HTTP client that connects to 1.1.1.1:80, used by `make test-intercept`
tests/              check_env.sh, test_socks4.sh, test_intercept.sh
.github/workflows/  build.yml, the macOS build and hook check
LICENSE             MIT
```

## License

Toralizer is released under the [MIT License](LICENSE).
