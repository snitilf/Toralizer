# tests

test scripts to verify toralizer setup and functionality.

## test_phase0.sh

verifies that phase 0 (prerequisites and setup) is complete.

checks:
- xcode command line tools installed
- homebrew installed
- gcc/clang available
- make available
- tor installed
- tor service running on port 9050
- tor SOCKS proxy accessible
- all project files present
- toralize script is executable

usage:
```bash
./tests/test_phase0.sh
```

expected output:
- all tests should pass (16/16)
- exit code 0 if successful
- exit code 1 if any failures

## test_phase1.sh

tests the standalone socks4 client to verify protocol implementation.

checks:
- socks4 connection to google dns (8.8.8.8:53)
- socks4 connection to cloudflare dns (1.1.1.1:53)
- socks4 connection to http port (1.1.1.1:80)

usage:
```bash
make phase1
```

## test_phase2.sh

tests the dynamic library interception with DYLD_INSERT_LIBRARIES.

checks:
- direct connection works (baseline)
- interception messages appear
- connection routes through tor proxy
- socks4 handshake succeeds
- http requests complete successfully
- documents sip limitations with system binaries

usage:
```bash
make phase2
```

## running all tests

```bash
# phase 0: prerequisites
./tests/test_phase0.sh

# phase 1: socks4 protocol
make phase1

# phase 2: dynamic library
make phase2
```

