#!/bin/bash
#
# test-with-binary.sh - demonstrates toralizer with custom binary
#
# this uses test_http which is NOT protected by sip, so it always works

echo "========================================"
echo "toralizer test with custom binary"
echo "========================================"
echo ""

# check if test_http exists
if [ ! -f "../test_http" ]; then
    echo "building test_http..."
    cd ..
    make test_http
    cd examples
fi

echo "[1] direct connection (no tor):"
echo "--------------------"
../test_http 2>&1 | grep -E "(connecting|connected|HTTP)" | head -5
echo ""

echo "[2] connection through tor:"
echo "--------------------"
../toralize ../test_http 2>&1 | grep -E "(\[toralize\]|connecting|connected|HTTP)" | head -10
echo ""

echo "========================================"
echo "test complete"
echo "========================================"
echo ""
echo "you should see [toralize] messages in the second test,"
echo "indicating that the connection was intercepted and"
echo "routed through the tor network."

