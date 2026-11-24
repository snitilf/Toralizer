#!/bin/bash
# runs real programs under ./toralize and checks the hook fires and traffic leaves through Tor
# needs Tor on 127.0.0.1:9050 and network access
cd "$(dirname "$0")/.." || exit 1

for f in toralize.dylib tests/test_http; do
    if [ ! -e "$f" ]; then
        echo "$f not found, run: make && make clients" >&2
        exit 1
    fi
done
if ! lsof -nP -iTCP:9050 -sTCP:LISTEN >/dev/null 2>&1; then
    echo "tor is not running on port 9050, run: brew services start tor" >&2
    exit 1
fi

IP_URL=https://check.torproject.org/api/ip
passed=0
total=0

# check <name> <exit status of the condition>, shows `detail` after a failure
check() {
    total=$((total + 1))
    if [ "$2" -eq 0 ]; then
        echo "pass: $1"
        passed=$((passed + 1))
    else
        echo "FAIL: $1"
        [ -n "${detail:-}" ] && echo "$detail" | head -5 | sed 's/^/      /'
    fi
}

contains() { printf '%s\n' "$1" | grep -qF -- "$2"; }

# direct and through the hook, with the log lines kept
detail=$(tests/test_http 2>&1); rc=$?
check "direct http request works" "$rc"

out=$(./toralize ./tests/test_http 2>&1); rc=$?
detail=$out
contains "$out" "[toralize] intercepting connection"; check "hook intercepts the connect" $?
contains "$out" "[toralize] connected to tor proxy"; check "hook reaches the tor proxy" $?
contains "$out" "[toralize] connection granted, routing through tor"; check "tor grants the connection" $?
check "http request through tor works" "$rc"

# /usr/bin/curl is SIP protected: the library is dropped, so the real address shows and the wrapper warns
out=$(./toralize /usr/bin/curl -4 -s --max-time 30 "$IP_URL" 2>&1)
detail=$out
contains "$out" '"IsTor":false'; check "system curl is not rerouted (SIP)" $?
contains "$out" "is protected by SIP"; check "wrapper warns about the SIP binary" $?
! contains "$out" "intercepting connection" && ! contains "$out" "connected to tor proxy"
check "no interception lines for the SIP binary" $?

# a Homebrew build is not protected, so check.torproject.org must see an exit relay
brew_curl="$(brew --prefix curl 2>/dev/null)/bin/curl"
if [ -x "$brew_curl" ]; then
    out=$(./toralize "$brew_curl" -4 -s --max-time 60 "$IP_URL" 2>&1)
    detail=$out
    contains "$out" '"IsTor":true'; check "homebrew curl exits through tor (IsTor true)" $?
else
    echo "skip: homebrew curl exits through tor (homebrew curl not found, run: brew install curl)"
fi

echo "$passed/$total intercept checks passed"
[ "$passed" -eq "$total" ]
