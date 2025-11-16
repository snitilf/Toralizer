# Toralizer

command-line tool that intercepts network traffic from other programs and routes it through Tor. the application doesn't need to know it's being proxied - everything happens transparently at the system level.

**quick start**: see `QUICKSTART.md` for 5-minute setup guide

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

verify setup and build:

```bash
# phase 0: verify prerequisites
./tests/test_phase0.sh

# phase 1: test socks4 protocol
make phase1

# phase 2: build and test dynamic library
make phase2
```

or just build directly:

```bash
make
```

this compiles toralize.dylib which intercepts connect() calls.

## usage

basic syntax:

```bash
./toralize <command> [args...]
```

quick examples:

```bash
# check your IP through tor
./toralize curl http://ipinfo.io/ip

# run custom programs through tor
./toralize ./test_http

# use with homebrew binaries
./toralize /opt/homebrew/bin/curl https://example.com
```

for detailed usage guide with real-world examples, see:
- **USAGE.md** - comprehensive guide with 10+ use cases
- **examples/** - ready-to-run example scripts

quick tests:

```bash
# verify everything works
make test

# or run example scripts
cd examples
./check-ip.sh
./test-anonymity.sh
```

## project structure

```
toralize.h        - socks4 protocol definitions
toralize.c        - connect() interception implementation
toralize.dylib    - compiled dynamic library
toralize          - wrapper script
Makefile          - build configuration
USAGE.md          - comprehensive usage guide
PHASES.md         - implementation tracking
examples/         - practical example scripts
tests/            - test suites for all phases
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

## important limitations

macOS system integrity protection (sip) prevents DYLD_INSERT_LIBRARIES from working with system binaries like /usr/bin/curl, /usr/bin/ssh, etc.

toralizer works with:
- custom-compiled applications
- user-installed binaries (via homebrew in /opt/homebrew or /usr/local)
- scripts and programs you build yourself

toralizer does NOT work with:
- system binaries in /usr/bin, /bin, /sbin
- any binary protected by sip

to use with homebrew-installed tools:
```bash
# install a non-system version
brew install curl

# example scripts automatically detect it
cd examples
./check-ip.sh
```

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

filip snítil

