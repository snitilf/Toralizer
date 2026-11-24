#!/bin/bash
# preflight for the Tor tests: build tools, tor installed and listening, Homebrew curl
# prints one line per check and exits 1 if any is missing, it does not score anything
cd "$(dirname "$0")/.." || exit 1

missing=0

ok() { echo "ok: $1"; }
miss() { echo "missing: $1"; missing=1; }

if command -v clang >/dev/null 2>&1; then
    ok "clang"
else
    miss "clang, run: xcode-select --install"
fi

if command -v make >/dev/null 2>&1; then
    ok "make"
else
    miss "make, run: xcode-select --install"
fi

if command -v tor >/dev/null 2>&1; then
    ok "tor installed"
else
    miss "tor, run: brew install tor"
fi

if lsof -nP -iTCP:9050 -sTCP:LISTEN >/dev/null 2>&1; then
    ok "tor listening on 127.0.0.1:9050"
else
    miss "tor is not running on port 9050, run: brew services start tor"
fi

brew_curl="$(brew --prefix curl 2>/dev/null)/bin/curl"
if [ -x "$brew_curl" ]; then
    ok "homebrew curl at $brew_curl"
else
    miss "homebrew curl, run: brew install curl"
fi

if [ "$missing" -eq 0 ]; then
    echo "environment ready"
    exit 0
fi
echo "environment not ready"
exit 1
