#!/bin/bash
#
# test-anonymity.sh - verify toralizer is working correctly

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# try to find a non-sip curl
if [ -f "/opt/homebrew/bin/curl" ]; then
    CURL="/opt/homebrew/bin/curl"
elif [ -f "/usr/local/bin/curl" ]; then
    CURL="/usr/local/bin/curl"
else
    echo -e "${YELLOW}warning: using system curl (protected by sip)${NC}"
    echo "this test requires homebrew curl to work properly"
    echo ""
    echo "install it with:"
    echo "  brew install curl-openssl"
    echo ""
    echo "or run ./test-with-binary.sh which uses test_http (not sip-protected)"
    echo ""
    exit 1
fi

echo "========================================"
echo "toralizer anonymity test"
echo "========================================"
echo "using: $CURL"
echo ""

# test 1: get real ip
echo "[1] getting your real ip address..."
REAL_IP=$($CURL -s http://ipinfo.io/ip 2>/dev/null)
if [ -z "$REAL_IP" ]; then
    echo -e "${RED}failed to get real ip${NC}"
    exit 1
fi
echo "    real ip: $REAL_IP"
echo ""

# test 2: get tor ip
echo "[2] getting tor exit node ip..."
TOR_IP=$(../toralize $CURL -s http://ipinfo.io/ip 2>/dev/null)
if [ -z "$TOR_IP" ]; then
    echo -e "${RED}failed to get tor ip${NC}"
    exit 1
fi
echo "    tor ip: $TOR_IP"
echo ""

# test 3: compare
echo "[3] comparing addresses..."
if [ "$REAL_IP" == "$TOR_IP" ]; then
    echo -e "${RED}WARNING: ips are the same!${NC}"
    echo "    toralizer may not be working"
    echo "    check: brew services list | grep tor"
    exit 1
else
    echo -e "${GREEN}SUCCESS: ips are different${NC}"
    echo "    your traffic is being routed through tor"
fi
echo ""

# test 4: multiple requests
echo "[4] testing multiple requests (checking for rotation)..."
IPS=()
for i in {1..3}; do
    IP=$(../toralize $CURL -s http://ipinfo.io/ip 2>/dev/null)
    IPS+=("$IP")
    echo "    request $i: $IP"
    sleep 2
done
echo ""

UNIQUE=$(printf '%s\n' "${IPS[@]}" | sort -u | wc -l)
echo "    unique ips: $UNIQUE/3"
if [ "$UNIQUE" -gt 1 ]; then
    echo -e "${GREEN}exit nodes are rotating${NC}"
else
    echo -e "${YELLOW}same exit node used (normal for quick requests)${NC}"
fi

echo ""
echo "========================================"
echo "anonymity test complete"
echo "========================================"

