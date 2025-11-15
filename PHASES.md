# toralizer development phases

this document tracks the implementation phases of Toralizer because i have a bad memory.

## phase 0: prerequisites & setup

**goal**: set up development environment

**completed**:
- installed xcode command line tools
- installed gcc, make, tor via homebrew
- started tor service (listening on 127.0.0.1:9050)
- created project structure (toralize.h, toralize.c, Makefile, wrapper script)
- created test script: tests/test_phase0.sh

**test results**: 16/16 tests passed

## phase 1: standalone socks4 client

**goal**: prove socks4 protocol implementation works

**completed**:
- built test_socks4.c - standalone client that connects through tor
- tested socks4 handshake with multiple destinations
- verified socks4 request/response protocol
- created test script: tests/test_phase1.sh

**test results**: 3/3 tests passed
- google dns (8.8.8.8:53): granted
- cloudflare dns (1.1.1.1:53): granted
- http port (1.1.1.1:80): granted

## phase 2: dynamic library with dyld_interpose

**goal**: convert socks4 logic into intercepting library

**completed**:
- implemented DYLD_INTERPOSE macro for proper macOS interposition
- built toralize.dylib with connect() interception
- used __connect system call to avoid recursion
- created test_http.c for testing (non-sip binary)
- created test script: tests/test_phase2.sh
- documented sip limitations

**key implementation details**:
- used DYLD_INTERPOSE macro instead of simple function replacement
- placed interposition data in __DATA,__interpose section
- called __connect() system call to connect to tor proxy
- properly sent socks4 header + username separately

**test results**: 5/5 tests passed
- direct connection works
- interception messages appear
- tor proxy connection succeeds
- socks4 handshake completes
- http requests work through tor

## known limitations

**system integrity protection (sip)**:
- system binaries cannot be intercepted
- /usr/bin/curl, /usr/bin/ssh, etc. are protected
- solution: use homebrew or custom-compiled binaries

## usage

```bash
# test with custom binary
./toralize ./test_http

# test with homebrew binary
brew install curl-openssl
./toralize /opt/homebrew/bin/curl http://ipinfo.io/ip
```

## project files

```
toralizer/
├── toralize.h              # socks4 protocol definitions
├── toralize.c              # dyld_interpose implementation
├── toralize.dylib          # compiled dynamic library
├── toralize                # wrapper script
├── Makefile                # build system
├── test_socks4.c           # phase 1 test client
├── test_http.c             # phase 2 test client
└── tests/
    ├── test_phase0.sh      # prerequisites verification
    ├── test_phase1.sh      # socks4 protocol tests
    └── test_phase2.sh      # dynamic library tests
```

## running tests

```bash
# verify prerequisites
./tests/test_phase0.sh

# test socks4 protocol
make phase1

# test dynamic library
make phase2
```

## technical notes

**dyld_interpose vs ld_preload**:
- macOS uses DYLD_INTERPOSE, not LD_PRELOAD
- requires special section: __DATA,__interpose
- more reliable than simple function replacement

**socks4 protocol**:
- 8-byte header: version, command, port, ip
- followed by null-terminated username
- response: version, code (90=granted), 6 ignored bytes

**socket hijacking**:
- create new socket for tor proxy
- perform socks4 handshake
- use dup2() to replace original socket fd
- application uses tor connection transparently

