#!/bin/bash
#
# check-ip.sh - compare your real ip vs tor exit node ip

echo "comparing ip addresses..."
echo ""
echo "your real ip:    $(curl -s http://ipinfo.io/ip 2>/dev/null || echo 'failed')"
echo "tor exit node:   $(../toralize curl -s http://ipinfo.io/ip 2>/dev/null || echo 'failed')"
echo ""
echo "if the ips are different, toralizer is working!"

