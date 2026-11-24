#!/bin/bash
# asks Tor for a SOCKS4 grant to three destinations with tests/test_socks4, needs Tor on 127.0.0.1:9050
# the ports are 80 and 443 because many Tor exit policies reject port 53
cd "$(dirname "$0")/.." || exit 1

if [ ! -x tests/test_socks4 ]; then
    echo "tests/test_socks4 not found, run: make clients" >&2
    exit 1
fi
if ! lsof -nP -iTCP:9050 -sTCP:LISTEN >/dev/null 2>&1; then
    echo "tor is not running on port 9050, run: brew services start tor" >&2
    exit 1
fi

export TORALIZE_QUIET=1
passed=0
total=0
for dest in "8.8.8.8 443" "1.1.1.1 443" "1.1.1.1 80"; do
    total=$((total + 1))
    # word splitting of $dest into host and port is intended
    # shellcheck disable=SC2086
    if out=$(tests/test_socks4 $dest 2>&1); then
        echo "pass: $out"
        passed=$((passed + 1))
    else
        echo "FAIL: $dest: $out"
    fi
done

echo "$passed/$total socks4 checks passed"
[ "$passed" -eq "$total" ]
