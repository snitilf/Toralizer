# makefile for toralizer (macOS)
# builds a dynamic library (.dylib) that redirects IPv4 TCP connect() calls through tor

# compiler and flags
CC = clang
CFLAGS = -Wall -Wextra -fPIC
LDFLAGS = -dynamiclib

# target library name
TARGET = toralize.dylib
TEST_CLIENT = test_socks4
TEST_HTTP = test_http

# source files
SOURCES = toralize.c
HEADERS = toralize.h
TEST_SOURCES = test_socks4.c
TEST_HTTP_SOURCES = test_http.c

# build the dynamic library (phase 2)
all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	@echo "building toralizer dynamic library..."
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(SOURCES)
	@echo "built $(TARGET)"
	@echo ""
	@echo "usage:"
	@echo "  ./toralize <program> [args...]"
	@echo ""
	@echo "the program must not be SIP-protected (so not /usr/bin/curl)"
	@echo "example, with homebrew curl and tor on 127.0.0.1:9050:"
	@echo "  ./toralize \"\$$(brew --prefix curl)/bin/curl\" -4 -s https://check.torproject.org/api/ip"

# build the test client (phase 1)
$(TEST_CLIENT): $(TEST_SOURCES) $(HEADERS)
	@echo "building socks4 test client..."
	$(CC) $(CFLAGS) -o $(TEST_CLIENT) $(TEST_SOURCES)
	@echo "built $(TEST_CLIENT)"
	@echo ""
	@echo "usage:"
	@echo "  ./$(TEST_CLIENT) <destination_ip> <destination_port>"
	@echo ""
	@echo "example:"
	@echo "  ./$(TEST_CLIENT) 8.8.8.8 53"

# build the http test client (phase 2)
$(TEST_HTTP): $(TEST_HTTP_SOURCES)
	@echo "building http test client..."
	$(CC) $(CFLAGS) -o $(TEST_HTTP) $(TEST_HTTP_SOURCES)
	@echo "built $(TEST_HTTP)"
	@echo ""
	@echo "usage:"
	@echo "  ./$(TEST_HTTP)                   # direct connection"
	@echo "  ./toralize ./$(TEST_HTTP)        # through tor"

# phase 1: test socks4 protocol implementation
phase1: $(TEST_CLIENT)
	@echo ""
	@echo "=== running phase 1 tests ==="
	@echo ""
	@./tests/test_phase1.sh

# phase 2: test dynamic library interception
phase2: $(TARGET) $(TEST_HTTP)
	@echo ""
	@echo "=== running phase 2 tests ==="
	@echo ""
	@./tests/test_phase2.sh

# clean build artifacts
clean:
	@echo "cleaning build artifacts..."
	rm -f $(TARGET) $(TEST_CLIENT) $(TEST_HTTP)
	@echo "clean complete"

# show your real ip vs your ip through tor (needs tor on 127.0.0.1:9050)
# uses homebrew curl because /usr/bin/curl is SIP-protected and ignores DYLD_INSERT_LIBRARIES
BREW_CURL = $(shell brew --prefix curl 2>/dev/null)/bin/curl
TEST_URL = https://check.torproject.org/api/ip

test: $(TARGET)
	@test -x "$(BREW_CURL)" || { echo "homebrew curl not found, run: brew install curl"; exit 1; }
	@echo "your real ip:"
	@"$(BREW_CURL)" -4 -s -w "\n" $(TEST_URL)
	@echo "your ip through tor:"
	@./toralize "$(BREW_CURL)" -4 -s -w "\n" $(TEST_URL)

.PHONY: all clean test phase1 phase2

