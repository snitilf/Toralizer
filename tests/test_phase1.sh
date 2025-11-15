#!/bin/bash
#
# test_phase1.sh - test the standalone socks4 client
#
# this verifies that our socks4 protocol implementation works
# before integrating it into the dynamic library

# colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "========================================"
echo "phase 1: socks4 protocol test"
echo "========================================"
echo ""

# check if test client exists
if [ ! -f "./test_socks4" ]; then
    echo -e "${RED}error: test_socks4 not found${NC}"
    echo "run: make test_socks4"
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

# test 1: connect to google dns (8.8.8.8:53)
echo "test 1: connecting to 8.8.8.8:53 (google dns) through tor"
echo "----------------------------------------"
./test_socks4 8.8.8.8 53
TEST1_RESULT=$?
echo ""

if [ $TEST1_RESULT -eq 0 ]; then
    echo -e "${GREEN}[PASS]${NC} test 1: google dns connection"
else
    echo -e "${RED}[FAIL]${NC} test 1: google dns connection"
fi
echo ""
echo ""

# test 2: connect to cloudflare dns (1.1.1.1:53)
echo "test 2: connecting to 1.1.1.1:53 (cloudflare dns) through tor"
echo "----------------------------------------"
./test_socks4 1.1.1.1 53
TEST2_RESULT=$?
echo ""

if [ $TEST2_RESULT -eq 0 ]; then
    echo -e "${GREEN}[PASS]${NC} test 2: cloudflare dns connection"
else
    echo -e "${RED}[FAIL]${NC} test 2: cloudflare dns connection"
fi
echo ""
echo ""

# test 3: connect to http port (1.1.1.1:80)
echo "test 3: connecting to 1.1.1.1:80 (http) through tor"
echo "----------------------------------------"
./test_socks4 1.1.1.1 80
TEST3_RESULT=$?
echo ""

if [ $TEST3_RESULT -eq 0 ]; then
    echo -e "${GREEN}[PASS]${NC} test 3: http port connection"
else
    echo -e "${RED}[FAIL]${NC} test 3: http port connection"
fi
echo ""

# summary
echo "========================================"
echo "test summary"
echo "========================================"

PASSED=0
FAILED=0

if [ $TEST1_RESULT -eq 0 ]; then ((PASSED++)); else ((FAILED++)); fi
if [ $TEST2_RESULT -eq 0 ]; then ((PASSED++)); else ((FAILED++)); fi
if [ $TEST3_RESULT -eq 0 ]; then ((PASSED++)); else ((FAILED++)); fi

echo -e "${GREEN}passed: $PASSED/3${NC}"
echo -e "${RED}failed: $FAILED/3${NC}"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}all tests passed! socks4 protocol implementation works.${NC}"
    echo ""
    echo "phase 1 complete. ready to proceed to phase 2:"
    echo "  make clean"
    echo "  make"
    echo "  make test"
    exit 0
else
    echo -e "${RED}some tests failed.${NC}"
    echo ""
    echo "troubleshooting:"
    echo "- make sure tor is running: brew services restart tor"
    echo "- check tor logs: brew services info tor"
    echo "- verify tor config allows SOCKS connections"
    exit 1
fi

