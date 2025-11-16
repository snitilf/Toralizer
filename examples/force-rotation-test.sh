#!/bin/bash
#
# force-rotation-test.sh - demonstrate forced circuit rotation

# find homebrew curl
if [ -f "/opt/homebrew/opt/curl/bin/curl" ]; then
    CURL="/opt/homebrew/opt/curl/bin/curl"
elif [ -f "/opt/homebrew/bin/curl" ]; then
    CURL="/opt/homebrew/bin/curl"
else
    echo "Error: Homebrew curl not found"
    exit 1
fi

echo "Testing with forced Tor circuit rotation..."
echo ""

for i in {1..3}; do
    echo "Request $i:"
    IP=$(../toralize $CURL -s http://ipinfo.io/ip)
    echo "  IP: $IP"
    
    if [ $i -lt 3 ]; then
        echo "  Forcing new Tor circuit..."
        # Send NEWNYM signal to Tor to get a new circuit
        # Note: Tor rate-limits this to once per 10 seconds
        echo -e 'AUTHENTICATE ""\r\nSIGNAL NEWNYM\r\nQUIT' | nc 127.0.0.1 9051 2>/dev/null
        echo "  Waiting for new circuit..."
        sleep 12  # Wait for rate limit + circuit build time
        echo ""
    fi
done

echo ""
echo "Note: Tor rate-limits circuit rotation to prevent abuse"
echo "      (max once every 10 seconds)"

