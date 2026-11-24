# toralizer (macOS): builds toralize.dylib, which reroutes IPv4 TCP connect() calls through tor

CC = clang
CFLAGS = -std=c11 -O2 -Wall -Wextra -I. $(EXTRA_CFLAGS)

TARGET = toralize.dylib
SOURCES = toralize.c socks4.c
HEADERS = toralize.h

TEST_CLIENT = test_socks4
TEST_HTTP = test_http

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -dynamiclib -o $@ $(SOURCES)

$(TEST_CLIENT): $(TEST_CLIENT).c socks4.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(TEST_CLIENT).c socks4.c

$(TEST_HTTP): $(TEST_HTTP).c
	$(CC) $(CFLAGS) -o $@ $<

# the two test clients, built without tor
clients: $(TEST_CLIENT) $(TEST_HTTP)

# the targets below need tor listening on 127.0.0.1:9050
check-env:
	@./tests/check_env.sh

test-socks4: $(TEST_CLIENT)
	@./tests/test_socks4.sh

test-intercept: $(TARGET) $(TEST_HTTP)
	@./tests/test_intercept.sh

# real address vs tor address, with homebrew curl (/usr/bin/curl is SIP-protected)
BREW_CURL = $(shell brew --prefix curl 2>/dev/null)/bin/curl
TEST_URL = https://check.torproject.org/api/ip

test: $(TARGET)
	@test -x "$(BREW_CURL)" || { echo "homebrew curl not found, run: brew install curl"; exit 1; }
	@echo "your real ip:"
	@"$(BREW_CURL)" -4 -s -w "\n" $(TEST_URL)
	@echo "your ip through tor:"
	@./toralize "$(BREW_CURL)" -4 -s -w "\n" $(TEST_URL)

clean:
	rm -f $(TARGET) $(TEST_CLIENT) $(TEST_HTTP)

.PHONY: all clients clean test check-env test-socks4 test-intercept
