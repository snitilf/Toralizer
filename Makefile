# makefile for toralizer (macOS)
# builds a dynamic library (.dylib) that intercepts network connections

# compiler and flags
CC = clang
CFLAGS = -Wall -fPIC
LDFLAGS = -dynamiclib

# target library name
TARGET = toralize.dylib

# source files
SOURCES = toralize.c
HEADERS = toralize.h

# build the dynamic library
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

# clean build artifacts
clean:
	@echo "cleaning build artifacts..."
	rm -f $(TARGET)
	@echo "clean complete"

# test by showing your real ip vs tor ip
test: $(TARGET)
	@echo "testing toralizer..."
	@echo ""
	@echo "your real ip:"
	@curl -s http://ipinfo.io/ip
	@echo ""
	@echo "your ip through tor:"
	@./toralize curl -s http://ipinfo.io/ip
	@echo ""

.PHONY: all clean test

