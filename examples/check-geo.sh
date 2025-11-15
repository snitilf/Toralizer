#!/bin/bash
#
# check-geo.sh - check geographic location of tor exit node

echo "checking tor exit node location..."
echo ""

LOCATION=$(../toralize curl -s https://ipapi.co/json/ 2>/dev/null)

if [ $? -eq 0 ]; then
    echo "$LOCATION" | jq -r '"ip:       \(.ip)\ncity:     \(.city)\ncountry:  \(.country_name)\nregion:   \(.region)\norg:      \(.org)"'
else
    echo "failed to get location (is jq installed?)"
fi

echo ""
echo "note: running this multiple times may show different locations"
echo "      as tor rotates through different exit nodes"

