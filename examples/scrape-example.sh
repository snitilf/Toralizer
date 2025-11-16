#!/bin/bash
#
# scrape-example.sh - example of web scraping through tor
#
# demonstrates how to scrape multiple pages while avoiding ip blocks

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
    echo "error: homebrew curl not found"
    echo "install it with: brew install curl"
    exit 1
fi

TARGET="http://httpbin.org/ip"
REQUESTS=5
DELAY=2

echo "scraping $REQUESTS pages through tor..."
echo "using: $CURL"
echo "target: $TARGET"
echo "delay between requests: ${DELAY}s"
echo ""

for i in $(seq 1 $REQUESTS); do
    echo "request $i/$REQUESTS:"
    
    # make request through tor
    RESPONSE=$(../toralize $CURL -s "$TARGET" 2>/dev/null)
    
    if [ $? -eq 0 ]; then
        # extract ip from response
        IP=$(echo "$RESPONSE" | jq -r '.origin' 2>/dev/null || echo "$RESPONSE")
        echo "  exit node: $IP"
        echo "  response: $(echo "$RESPONSE" | head -c 100)..."
    else
        echo "  failed"
    fi
    
    # delay between requests
    if [ $i -lt $REQUESTS ]; then
        sleep $DELAY
    fi
done

echo ""
echo "scraping complete"
echo ""
echo "note: in real scraping:"
echo "  - respect robots.txt"
echo "  - honor rate limits"
echo "  - follow terms of service"
echo "  - add random delays"
echo "  - handle errors gracefully"

