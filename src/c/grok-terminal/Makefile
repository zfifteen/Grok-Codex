# Grok Terminal Makefile
# Interactive terminal session with Grok AI via xAI streaming API

# Include parent Makefile for common dependencies
include ../Makefile

# Project-specific settings
PROJECT = grok-terminal
TARGET = $(PROJECT)
SRC = grok_terminal.c

# Additional dependencies (no new deps beyond parent - using system libs)
# libcurl and json-c should be available via pkg-config
CURL_CFLAGS := $(shell pkg-config --cflags libcurl 2>/dev/null || echo "")
CURL_LIBS := $(shell pkg-config --libs libcurl 2>/dev/null || echo "-lcurl")
JSON_CFLAGS := $(shell pkg-config --cflags json-c 2>/dev/null || echo "")
JSON_LIBS := $(shell pkg-config --libs json-c 2>/dev/null || echo "-ljson-c")

# Combined flags
PROJECT_CFLAGS = $(CFLAGS) $(CURL_CFLAGS) $(JSON_CFLAGS)
PROJECT_LIBS = $(CURL_LIBS) $(JSON_LIBS) $(COMMON_LIBS)

# Build target
.PHONY: all
all: check-deps
	@$(MAKE) $(TARGET)

# Check for project dependencies
.PHONY: check-deps
check-deps:
	@echo "Checking dependencies for $(PROJECT)..."
	@pkg-config --exists libcurl || (echo "Error: libcurl not found. Install with: sudo apt-get install libcurl4-openssl-dev" && exit 1)
	@pkg-config --exists json-c || (echo "Error: json-c not found. Install with: sudo apt-get install libjson-c-dev" && exit 1)
	@echo "✓ All dependencies available"

# Build the executable
$(TARGET): $(SRC)
	@echo "Building $(TARGET)..."
	$(CC) $(PROJECT_CFLAGS) -o $(TARGET) $(SRC) $(PROJECT_LIBS)
	@echo "✓ Build complete: $(TARGET)"

# Clean target
.PHONY: clean
clean:
	@echo "Cleaning $(PROJECT)..."
	rm -f $(TARGET)
	@echo "✓ Clean complete"

# Test target (runs the demo script)
.PHONY: test
test: all
	@echo "Running demo script..."
	@./demo.sh

# Install dependencies (Ubuntu/Debian)
.PHONY: install-deps
install-deps:
	@echo "Installing dependencies..."
	@echo "This requires sudo privileges..."
	sudo apt-get update
	sudo apt-get install -y libcurl4-openssl-dev libjson-c-dev libgmp-dev libmpfr-dev
	@echo "✓ Dependencies installed"

# Help
.PHONY: help
help:
	@echo "Grok Terminal - Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all           - Build the executable (default)"
	@echo "  check-deps    - Verify dependencies are installed"
	@echo "  clean         - Remove built files"
	@echo "  test          - Run demo script"
	@echo "  install-deps  - Install required dependencies (requires sudo)"
	@echo "  help          - Show this help message"
