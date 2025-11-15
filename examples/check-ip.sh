#!/bin/bash
#
# check-ip.sh - compare your real ip vs tor exit node ip

# try to find a non-sip curl
if [ -f "/opt/homebrew/bin/curl" ]; then
    CURL="/opt/homebrew/bin/curl"
elif [ -f "/usr/local/bin/curl" ]; then
    CURL="/usr/local/bin/curl"
else
    echo "warning: using system curl (may not work due to sip)"
    echo "install homebrew curl: brew install curl-openssl"
    echo ""
    CURL="curl"
fi

echo "comparing ip addresses..."
echo "using: $CURL"
echo ""
echo "your real ip:    $($CURL -s http://ipinfo.io/ip 2>/dev/null || echo 'failed')"
echo "tor exit node:   $(../toralize $CURL -s http://ipinfo.io/ip 2>/dev/null || echo 'failed')"
echo ""
echo "if the ips are different, toralizer is working!"
echo ""
echo "note: if ips are the same, install homebrew curl:"
echo "      brew install curl-openssl"

