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

LOCATION=$(../toralize $CURL -s https://ipapi.co/json/ 2>/dev/null)

if [ $? -eq 0 ]; then
    echo "$LOCATION" | jq -r '"ip:       \(.ip)\ncity:     \(.city)\ncountry:  \(.country_name)\nregion:   \(.region)\norg:      \(.org)"'
else
    echo "failed to get location (is jq installed?)"
fi

echo ""
echo "note: running this multiple times may show different locations"
echo "      as tor rotates through different exit nodes"

