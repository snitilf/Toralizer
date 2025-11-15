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

## adding more tests

future test scripts will be added here for:
- phase 1: socks4 client functionality
- phase 2: dynamic library interception
- integration tests with real applications

