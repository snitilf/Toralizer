# toralizer (macOS): builds toralize.dylib, which reroutes IPv4 TCP connect() calls through tor

CC = clang
CFLAGS = -std=c11 -O2 -Wall -Wextra -I. $(EXTRA_CFLAGS)

TARGET = toralize.dylib
SOURCES = toralize.c socks4.c
HEADERS = toralize.h

TEST_CLIENT = tests/test_socks4
TEST_HTTP = tests/test_http
TEST_ROUTING = tests/test_routing

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -dynamiclib -o $@ $(SOURCES)

$(TEST_CLIENT): $(TEST_CLIENT).c socks4.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(TEST_CLIENT).c socks4.c

$(TEST_HTTP): $(TEST_HTTP).c
	$(CC) $(CFLAGS) -o $@ $<

$(TEST_ROUTING): $(TEST_ROUTING).c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $<

# the two test clients, built without tor
clients: $(TEST_CLIENT) $(TEST_HTTP)

# routing table against a mock proxy, needs neither tor nor network
test-routing: $(TARGET) $(TEST_ROUTING)
	@./toralize ./$(TEST_ROUTING)

# the targets below need tor listening on 127.0.0.1:9050
check-env:
	@./tests/check_env.sh

test-socks4: $(TEST_CLIENT)
	@./tests/test_socks4.sh

test-intercept: $(TARGET) $(TEST_HTTP)
	@./tests/test_intercept.sh

# everything: test-routing runs without tor, the rest need tor on 127.0.0.1:9050
test:
	@$(MAKE) --no-print-directory test-routing
	@$(MAKE) --no-print-directory check-env
	@$(MAKE) --no-print-directory test-socks4
	@$(MAKE) --no-print-directory test-intercept

# real address vs tor address, with homebrew curl (/usr/bin/curl is SIP-protected)
BREW_CURL = $(shell brew --prefix curl 2>/dev/null)/bin/curl
TEST_URL = https://check.torproject.org/api/ip

demo: $(TARGET)
	@test -x "$(BREW_CURL)" || { echo "homebrew curl not found, run: brew install curl"; exit 1; }
	@echo "your real ip:"
	@"$(BREW_CURL)" -4 -s -w "\n" $(TEST_URL)
	@echo "your ip through tor:"
	@./toralize "$(BREW_CURL)" -4 -s -w "\n" $(TEST_URL)

clean:
	rm -f $(TARGET) $(TEST_CLIENT) $(TEST_HTTP) $(TEST_ROUTING)

.PHONY: all clients clean test demo test-routing check-env test-socks4 test-intercept
