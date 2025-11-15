#!/bin/bash
#
# test_phase2.sh - test the dyld_insert_libraries dynamic library
#
# this verifies that our dynamic library properly intercepts connect() calls

# colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "========================================"
echo "phase 2: dynamic library interception test"
echo "========================================"
echo ""

# check if library exists
if [ ! -f "./toralize.dylib" ]; then
    echo -e "${RED}error: toralize.dylib not found${NC}"
    echo "run: make"
    exit 1
fi

# check if test_http exists
if [ ! -f "./test_http" ]; then
    echo -e "${RED}error: test_http not found${NC}"
    echo "run: make test_http"
    exit 1
fi

# check if tor is running
echo "checking tor service..."
if ! lsof -i :9050 &> /dev/null; then
    echo -e "${RED}error: tor is not running on port 9050${NC}"
    echo "run: brew services start tor"
    exit 1
fi
echo -e "${GREEN}tor is running${NC}"
echo ""

# test 1: verify direct connection works (baseline)
echo "test 1: direct connection (no interception)"
echo "----------------------------------------"
./test_http 2>&1 | head -8
TEST1_RESULT=$?
echo ""

if [ $TEST1_RESULT -eq 0 ]; then
    echo -e "${GREEN}[PASS]${NC} test 1: direct connection works"
else
    echo -e "${RED}[FAIL]${NC} test 1: direct connection failed"
fi
echo ""
echo ""

# test 2: verify interception happens
echo "test 2: connection with toralize (interception)"
echo "----------------------------------------"
OUTPUT=$(./toralize ./test_http 2>&1)
TEST2_RESULT=$?

echo "$OUTPUT" | head -20
echo ""

# check if interception messages are present
if echo "$OUTPUT" | grep -q "\[toralize\] intercepting connection"; then
    echo -e "${GREEN}[PASS]${NC} test 2a: interception message found"
    TEST2A_PASS=1
else
    echo -e "${RED}[FAIL]${NC} test 2a: interception message not found"
    TEST2A_PASS=0
fi

if echo "$OUTPUT" | grep -q "\[toralize\] connected to tor proxy"; then
    echo -e "${GREEN}[PASS]${NC} test 2b: connected to tor proxy"
    TEST2B_PASS=1
else
    echo -e "${RED}[FAIL]${NC} test 2b: did not connect to tor proxy"
    TEST2B_PASS=0
fi

if echo "$OUTPUT" | grep -q "\[toralize\] connection granted, routing through tor"; then
    echo -e "${GREEN}[PASS]${NC} test 2c: socks4 connection granted"
    TEST2C_PASS=1
else
    echo -e "${RED}[FAIL]${NC} test 2c: socks4 connection not granted"
    TEST2C_PASS=0
fi

if [ $TEST2_RESULT -eq 0 ]; then
    echo -e "${GREEN}[PASS]${NC} test 2d: http request succeeded"
    TEST2D_PASS=1
else
    echo -e "${RED}[FAIL]${NC} test 2d: http request failed"
    TEST2D_PASS=0
fi

echo ""

# test 3: note about system binaries
echo "test 3: system binaries (sip protected)"
echo "----------------------------------------"
echo "note: system binaries like /usr/bin/curl are protected by"
echo "      macOS system integrity protection (sip) and cannot"
echo "      be intercepted with DYLD_INSERT_LIBRARIES."
echo ""
echo "testing with curl (expected to NOT show interception):"
./toralize curl -s http://1.1.1.1 2>&1 | head -5
echo ""
echo -e "${YELLOW}[INFO]${NC} this is expected behavior on modern macOS"
echo -e "${YELLOW}[INFO]${NC} use custom-compiled binaries for testing"
echo ""

# summary
echo "========================================"
echo "test summary"
echo "========================================"

TOTAL_TESTS=5
PASSED=$((TEST2A_PASS + TEST2B_PASS + TEST2C_PASS + TEST2D_PASS))
if [ $TEST1_RESULT -eq 0 ]; then ((PASSED++)); fi

echo -e "${GREEN}passed: $PASSED/$TOTAL_TESTS${NC}"
FAILED=$((TOTAL_TESTS - PASSED))
echo -e "${RED}failed: $FAILED/$TOTAL_TESTS${NC}"
echo ""

if [ $PASSED -eq $TOTAL_TESTS ]; then
    echo -e "${GREEN}all tests passed! phase 2 complete.${NC}"
    echo ""
    echo "the dynamic library successfully intercepts connect() calls"
    echo "and routes them through tor using the socks4 protocol."
    echo ""
    echo "limitations:"
    echo "- system binaries (protected by sip) cannot be intercepted"
    echo "- use with custom-compiled applications or scripts"
    exit 0
else
    echo -e "${RED}some tests failed.${NC}"
    exit 1
fi

