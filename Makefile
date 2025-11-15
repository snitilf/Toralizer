# Makefile for Toralizer (macOS)
# Builds a dynamic library (.dylib) that intercepts network connections

# Compiler and flags
CC = clang
CFLAGS = -Wall -fPIC
LDFLAGS = -dynamiclib

# Target library name
TARGET = toralize.dylib

# Source files
SOURCES = toralize.c
HEADERS = toralize.h

# Build the dynamic library
all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	@echo "Building Toralizer dynamic library..."
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(SOURCES)
	@echo "✓ Built $(TARGET)"
	@echo ""
	@echo "Usage:"
	@echo "  ./toralize <command>"
	@echo ""
	@echo "Example:"
	@echo "  ./toralize curl http://ipinfo.io/ip"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(TARGET)
	@echo "✓ Clean complete"

# Test by showing your real IP vs Tor IP
test: $(TARGET)
	@echo "Testing Toralizer..."
	@echo ""
	@echo "Your real IP:"
	@curl -s http://ipinfo.io/ip
	@echo ""
	@echo "Your IP through Tor:"
	@./toralize curl -s http://ipinfo.io/ip
	@echo ""

.PHONY: all clean test

