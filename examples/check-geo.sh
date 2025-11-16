#!/bin/bash
#
# check-geo.sh - check geographic location of tor exit node

# try to find a non-sip curl
if [ -f "/opt/homebrew/bin/curl" ]; then
    CURL="/opt/homebrew/bin/curl"
elif [ -f "/opt/homebrew/opt/curl/bin/curl" ]; then
    CURL="/opt/homebrew/opt/curl/bin/curl"
elif [ -f "/usr/local/bin/curl" ]; then
    CURL="/usr/local/bin/curl"
elif [ -f "/usr/local/opt/curl/bin/curl" ]; then
    CURL="/usr/local/opt/curl/bin/curl"
else
    echo "warning: using system curl (may not work due to sip)"
    echo "install homebrew curl: brew install curl"
    echo "then link it: brew link --force curl"
    echo ""
    CURL="curl"
fi

echo "checking tor exit node location..."
echo "using: $CURL"
echo ""

# Get IP from httpbin
IP=$(../toralize $CURL -s http://httpbin.org/ip 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+' | head -1)

if [ -z "$IP" ]; then
    echo "failed to get tor exit node ip"
    exit 1
fi

echo "tor exit node ip: $IP"
echo ""
echo "note: getting geolocation data (using ip-api.com)..."

# Get geolocation data using the IP (without going through Tor to avoid blocking)
LOCATION=$($CURL -s "http://ip-api.com/json/$IP" 2>/dev/null)

if [ $? -eq 0 ] && [ -n "$LOCATION" ]; then
    if command -v jq &> /dev/null; then
        echo "$LOCATION" | jq -r '"city:     \(.city)\ncountry:  \(.country)\nregion:   \(.regionName)\nisp:      \(.isp)\norg:      \(.org)"'
    else
        echo "$LOCATION"
        echo ""
        echo "tip: install jq for pretty output (brew install jq)"
    fi
else
    echo "failed to get location data"
fi

echo ""
echo "note: running this multiple times may show different locations"
echo "      as tor rotates through different exit nodes"

