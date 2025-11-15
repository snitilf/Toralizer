# makefile for toralizer (macOS)
# builds a dynamic library (.dylib) that intercepts network connections

# compiler and flags
CC = clang
CFLAGS = -Wall -fPIC
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
	@echo "  ./toralize <command>"
	@echo ""
	@echo "example:"
	@echo "  ./toralize curl http://ipinfo.io/ip"

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

# test by showing your real ip vs tor ip (phase 2)
test: $(TARGET)
	@echo "testing toralizer..."
	@echo ""
	@echo "your real ip:"
	@curl -s http://ipinfo.io/ip
	@echo ""
	@echo "your ip through tor:"
	@./toralize curl -s http://ipinfo.io/ip
	@echo ""

.PHONY: all clean test phase1 phase2

