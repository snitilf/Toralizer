# Toralizer

command-line tool that intercepts network traffic from other programs and routes it through Tor. the application doesn't need to know it's being proxied - everything happens transparently at the system level.

## how it works

uses the DYLD_INSERT_LIBRARIES technique on macOS (like LD_PRELOAD on Linux) to intercept connect() system calls and redirect them through Tor's SOCKS proxy.

## prerequisites

- macOS with Xcode Command Line Tools
- Homebrew
- Tor (installed via brew)

## installation

first, install dependencies:

```bash
brew install gcc make tor
brew services start tor
```

verify phase 0 setup (optional):

```bash
./tests/test_phase0.sh
```

then build the library:

```bash
make
```

this compiles toralize.dylib which does the actual interception.

## usage

basic syntax:

```bash
./toralize <command> [args...]
```

examples:

```bash
# check your real IP
curl http://ipinfo.io/ip

# check your IP through Tor
./toralize curl http://ipinfo.io/ip

# ssh through Tor
./toralize ssh user@example.com

# download files through Tor
./toralize wget http://example.com/file.txt
```

quick test:

```bash
make test
```

## project structure

```
toralize.h        - SOCKS4 protocol definitions
toralize.c        - connect() interception implementation
Makefile          - build configuration
toralize          - wrapper script
```

## development

clean build artifacts:

```bash
make clean
```

rebuild everything:

```bash
make clean && make
```

## security notes

- only routes TCP connections through Tor
- DNS requests may still leak (use Tor Browser for complete anonymity)
- some applications might not work correctly
- for educational and legitimate privacy purposes only

## technical details

SOCKS4 protocol flow:
1. connect to Tor proxy at 127.0.0.1:9050
2. send connection request with destination IP/port
3. receive response (code 90 means granted)
4. use dup2() to replace original socket with Tor-connected socket

macOS specifics:
- uses DYLD_INSERT_LIBRARIES environment variable
- compiled as .dylib (dynamic library)
- uses dlsym(RTLD_NEXT, "connect") to get the original function pointer

## troubleshooting

library not found:
```bash
make
```

tor not running:
```bash
brew services start tor
```

check if tor is listening:
```bash
lsof -i :9050
```

## license

MIT

## author

Filip Snitil

