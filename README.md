<div align="center">

# Toralizer

### Reroute a program's IPv4 TCP through Tor, without touching the program.

[![build](https://img.shields.io/github/actions/workflow/status/snitilf/Toralizer/build.yml?branch=main&label=build)](https://github.com/snitilf/Toralizer/actions/workflows/build.yml)
[![license](https://img.shields.io/github/license/snitilf/Toralizer?label=license)](LICENSE)

[How it works](#how-it-works) · [Quick start](#quick-start) · [Limits](#limits) · [Tests](#tests)

</div>

Toralizer is a macOS dynamic library in C that hooks `connect()` through a dyld interpose entry.
dyld is the macOS dynamic linker, and an interpose entry tells it to swap one function for another.
The library reroutes the IPv4 TCP connections of command-line programs through Tor with a hand-written SOCKS4 handshake.
SOCKS4 is a simple proxy protocol that Tor accepts.
You inject the library with `DYLD_INSERT_LIBRARIES`, so the program is never recompiled or changed.

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/demo/demo-dark.svg">
  <source media="(prefers-color-scheme: light)" srcset="docs/demo/demo-light.svg">
  <img src="docs/demo/demo-light.svg" alt="A terminal session. Homebrew curl run directly reports IsTor false. The same command with ./toralize in front logs four [toralize] lines and reports IsTor true with a Tor exit address. ./toralize with /usr/bin/curl prints a SIP warning and reports IsTor false again.">
</picture>

Captured with `docs/demo/record.py` on macOS 27.2.1 (arm64), with the real address masked.

Read these limits first:

- System Integrity Protection (SIP) removes `DYLD_*` variables from system binaries, so `/usr/bin/curl` is not rerouted.
  Use a Homebrew build or a program that you compiled yourself.
  For a protected program, the wrapper prints a warning and still runs it.
- Only IPv4 TCP is rerouted.
  The library refuses non-loopback IPv6 TCP and leaves UDP and Unix sockets alone.
- SOCKS4 has no hostnames, so the program resolves them itself, outside Tor.
  DNS lookups are not proxied, so this is not a privacy tool.

The hook is `toralize.c` and `socks4.c`, about 200 lines together.
It needs Tor to run on `127.0.0.1:9050`.

## How it works

1. `./toralize` puts `toralize.dylib` in front of any existing `DYLD_INSERT_LIBRARIES` and runs your command.
   dyld loads the library into the process before `main`.
2. The `__interpose` section of the library pairs `toralize_connect` with `connect`.
   dyld rebinds every call to `connect()` in the other loaded images to `toralize_connect`.
3. For a non-loopback IPv4 TCP address, `toralize_connect` opens a new blocking socket to Tor at `127.0.0.1:9050`.
   It sends a SOCKS4 CONNECT request: version 4, command 1, the destination port and address, and the user id `toralizer`.
   The socket blocks, so the handshake also works when the caller uses a non-blocking socket.
4. It reads the 8-byte reply and accepts only version 0 with code 90.
5. It calls `dup2()` to put the proxy socket on the descriptor of the caller, then restores `O_NONBLOCK` and `FD_CLOEXEC`.
   The program keeps its descriptor, and the descriptor now reaches the destination through Tor.

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

The table shows what `connect()` does with each kind of destination:

| Destination | Result |
| --- | --- |
| IPv4 TCP, not loopback | Through Tor |
| IPv4 loopback (`127.0.0.0/8`, `0.0.0.0`), `::1`, IPv4-mapped loopback | Direct, untouched |
| IPv6 TCP, not loopback, including `::` and IPv4-mapped addresses that are not loopback | Refused with `ENETUNREACH` |
| UDP, `AF_UNIX`, any other family or socket type | Direct, untouched |
| IPv4 TCP while Tor is not running | Fails with `ECONNREFUSED`, never goes direct |

The library refuses IPv6 so that a dual-stack program cannot bypass Tor by accident, and curl falls back to IPv4 on its own.

## Internals

- Interpose: the entry is visible in the built library.

  ```text
  $ nm -m toralize.dylib | grep __interpose
  0000000000004098 (__DATA_CONST,__interpose) non-external __interpose_connect
  ```

- Protocol: the SOCKS4 request and reply are packed structs in `toralize.h`, with no SOCKS library.
  `socks4_handshake()` in `socks4.c` sends the request in one write, and the test client calls the same function.
- Raw connect: the hook calls the `__connect` stub from libsystem_kernel for the proxy socket and for the connections that it passes through.
  No other interposer in the process sees them.
- Errors: a failure after the proxy socket opens and before `dup2()` closes that socket, sets `errno`, and returns `-1`.
  If `socket()` fails or the hook cannot read the flags of the caller, it returns `-1` before a proxy socket exists.
  If the flag restore fails after `dup2()`, the call returns `-1`, but the descriptor already is the Tor socket and stays open.

## Quick start

You need macOS with the Xcode Command Line Tools and [Homebrew](https://brew.sh).
The library builds for the host architecture.
It was run only on arm64.

1. Install Tor and a Homebrew curl, and start Tor:

   ```sh
   brew install tor curl
   brew services start tor
   ```

   Port 9050 works after Tor finishes its startup, which takes a moment.

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

   The answer contains `"IsTor":true`.
   The `-4` flag keeps curl on IPv4.

Other ways to run it:

```sh
# one of your own binaries, no Homebrew curl needed
make clients && ./toralize ./tests/test_http

# what the wrapper does
DYLD_INSERT_LIBRARIES="$PWD/toralize.dylib" ./your-program
```

Two environment variables change the behavior:

- `TORALIZE_QUIET=1` hides the informational `[toralize]` lines.
  Errors and refusals still print to stderr.
- `TORALIZE_PROXY_PORT` selects another port of `127.0.0.1` for Tor, for example `9150` for Tor Browser.
  A value that is not a port number makes the connection fail with `EINVAL`.

If you see `[toralize] connect to proxy: Connection refused`, Tor is not listening.
To see what listens on port 9050, run `lsof -nP -iTCP:9050 -sTCP:LISTEN`.

## Limits

Besides the three limits above:

- A program that a system binary starts, such as `/bin/sh -c`, loses the library.
  A binary with the hardened runtime (a macOS code-signing option) loses it too.
- Only `connect()` is hooked.
  A program that connects with `connectx()` or Network.framework is not rerouted.
- Private and LAN IPv4 addresses go to Tor too, and Tor refuses them.
- The user id is fixed, and Tor isolates streams by SOCKS user id.
  Until Tor rotates the circuit, all toralize traffic leaves from one exit address, and some sites rate-limit that address.
- Socket options that a program sets before `connect()` are lost, because the proxy socket replaces the descriptor.
- The handshake blocks and has no timeout, even for a non-blocking socket.
- The library supports the host architecture only, with no universal binary.

## Tests

| Test | Run with | Needs Tor | What it tests | Result |
| --- | --- | --- | --- | --- |
| `tests/test_routing.c` | `make test-routing` | no | Every row of the routing table, against a mock SOCKS4 proxy | 16/16 |
| `tests/check_env.sh` | `make check-env` | yes | Preflight: clang, make, Tor installed and listening, Homebrew curl | ready |
| `tests/test_socks4.sh` | `make test-socks4` | yes | SOCKS4 client gets a grant for 3 destinations | 3/3 |
| `tests/test_intercept.sh` | `make test-intercept` | yes | Hook fires, Tor grants, HTTP works, Homebrew curl exits through Tor, system curl stays direct with a warning | 9/9 |

`test_routing` is a C program that runs under `./toralize`.
It starts a mock SOCKS4 proxy on a free local port and points the library to it with `TORALIZE_PROXY_PORT`, so it needs no Tor and no network.
It makes sure that a grant works and data flows, and that proxy code 91, a short reply, and a bad version give `ECONNREFUSED`, `ECONNRESET`, and `EPROTO`.
It also makes sure that a missing proxy fails closed and that non-loopback IPv6 gets `ENETUNREACH`.
Loopback, UDP, and `AF_UNIX` connections never reach the proxy, and `O_NONBLOCK` and `FD_CLOEXEC` survive.

The other three tests are shell scripts that count their own results, and they need a running Tor.
Their results come from one run on arm64 with Tor up.

CI on macOS builds the library with `-Werror` and runs `make test-routing`.
It makes sure that the interpose entry exists, that the hook logs an interception without Tor, and that the demo SVGs match `docs/demo/session.json`.
It does not run the Tor scripts.

`make test` runs `test-routing`, then the three Tor scripts in the order of the table, and stops at the first failure.
`make clients` builds the two test clients without Tor.

<details>
<summary>Project layout</summary>

```text
toralize.c            the interposed connect(), routing and proxy socket handling
socks4.c              the SOCKS4 handshake, shared by the library and the test client
toralize.h            proxy address, SOCKS4 constants, packed request and reply structs
toralize              wrapper that sets DYLD_INSERT_LIBRARIES and execs your command
Makefile              builds toralize.dylib and the test programs, runs the tests, `make demo` prints the real and the Tor address
tests/                check_env.sh, test_socks4.sh, test_intercept.sh
tests/test_routing.c  routing table test with a mock SOCKS4 proxy, used by `make test-routing`
tests/test_socks4.c   standalone SOCKS4 client, used by `make test-socks4`
tests/test_http.c     plain HTTP client that connects to 1.1.1.1:80, used by `make test-intercept`
docs/demo/            record.py captures the demo run into session.json, render.py draws it as the two SVGs (`make demo-record`, `make demo-render`)
.github/workflows/    build.yml, the macOS build and hook check
LICENSE               MIT
```

</details>

## License

Toralizer is released under the [MIT License](LICENSE).
