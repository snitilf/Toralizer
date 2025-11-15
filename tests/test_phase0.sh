#!/bin/bash
#
# test_phase0.sh - verify that phase 0 setup is complete
#
# this script checks:
# - xcode command line tools
# - homebrew installation
# - required packages (gcc, make, tor)
# - tor service running
# - project structure

# colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # no color

PASS=0
FAIL=0

echo "========================================"
echo "phase 0: prerequisites & setup test"
echo "========================================"
echo ""

# function to print test results
pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((PASS++))
}

fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((FAIL++))
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

# test 1: check xcode command line tools
echo "checking xcode command line tools..."
if xcode-select -p &> /dev/null; then
    XCODE_PATH=$(xcode-select -p)
    pass "xcode command line tools installed at: $XCODE_PATH"
else
    fail "xcode command line tools not found"
    echo "     run: xcode-select --install"
fi
echo ""

# test 2: check homebrew
echo "checking homebrew..."
if command -v brew &> /dev/null; then
    BREW_VERSION=$(brew --version | head -n 1)
    pass "homebrew installed: $BREW_VERSION"
else
    fail "homebrew not found"
    echo "     visit: https://brew.sh"
fi
echo ""

# test 3: check gcc
echo "checking gcc..."
if command -v gcc &> /dev/null; then
    GCC_VERSION=$(gcc --version | head -n 1)
    pass "gcc available: $GCC_VERSION"
else
    fail "gcc not found"
    echo "     run: brew install gcc"
fi
echo ""

# test 4: check clang (macOS compiler)
echo "checking clang..."
if command -v clang &> /dev/null; then
    CLANG_VERSION=$(clang --version | head -n 1)
    pass "clang available: $CLANG_VERSION"
else
    fail "clang not found"
fi
echo ""

# test 5: check make
echo "checking make..."
if command -v make &> /dev/null; then
    MAKE_VERSION=$(make --version | head -n 1)
    pass "make available: $MAKE_VERSION"
else
    fail "make not found"
    echo "     run: brew install make"
fi
echo ""

# test 6: check tor
echo "checking tor..."
if command -v tor &> /dev/null; then
    TOR_VERSION=$(tor --version | head -n 1)
    pass "tor installed: $TOR_VERSION"
else
    fail "tor not found"
    echo "     run: brew install tor"
fi
echo ""

# test 7: check if tor service is running
echo "checking tor service..."
if lsof -i :9050 &> /dev/null; then
    TOR_PID=$(lsof -t -i :9050)
    pass "tor service running on port 9050 (PID: $TOR_PID)"
else
    fail "tor service not running on port 9050"
    echo "     run: brew services start tor"
fi
echo ""

# test 8: test tor connectivity
echo "testing tor connectivity..."
if nc -z 127.0.0.1 9050 2>/dev/null; then
    pass "can connect to tor SOCKS proxy at 127.0.0.1:9050"
else
    fail "cannot connect to tor SOCKS proxy"
fi
echo ""

# test 9: check project structure
echo "checking project structure..."

# get project root directory (parent of tests directory)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

PROJECT_FILES=(
    "toralize.h"
    "toralize.c"
    "Makefile"
    "toralize"
    "README.md"
    ".gitignore"
)

MISSING_FILES=0
for file in "${PROJECT_FILES[@]}"; do
    if [ -f "$PROJECT_ROOT/$file" ]; then
        pass "found: $file"
    else
        fail "missing: $file"
        ((MISSING_FILES++))
    fi
done

if [ $MISSING_FILES -eq 0 ]; then
    echo ""
    pass "all project files present"
fi
echo ""

# test 10: check if toralize is executable
echo "checking toralize permissions..."
if [ -x "$PROJECT_ROOT/toralize" ]; then
    pass "toralize script is executable"
else
    fail "toralize script is not executable"
    echo "     run: chmod +x toralize"
fi
echo ""

# final summary
echo "========================================"
echo "test summary"
echo "========================================"
echo -e "${GREEN}passed: $PASS${NC}"
echo -e "${RED}failed: $FAIL${NC}"
echo ""

if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}all tests passed! phase 0 is complete.${NC}"
    echo ""
    echo "you can now proceed to build and test:"
    echo "  make"
    echo "  make test"
    exit 0
else
    echo -e "${RED}some tests failed. please fix the issues above.${NC}"
    exit 1
fi

