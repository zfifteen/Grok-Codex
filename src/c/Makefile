# Parent Makefile for shared C dependencies
# This provides common configuration for all C projects under src/c/

# Compiler settings
CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -std=c11

# Common dependencies (GMP/MPFR for large number operations)
COMMON_LIBS = -lgmp -lmpfr

# Check for required tools
.PHONY: check-tools
check-tools:
	@which $(CC) > /dev/null || (echo "Error: $(CC) not found" && exit 1)
	@pkg-config --exists gmp || (echo "Error: GMP not found" && exit 1)
	@pkg-config --exists mpfr || (echo "Error: MPFR not found (optional)" && exit 1) || true
	@echo "✓ All required tools are available"

# Build shared libraries (placeholder for future shared libs)
.PHONY: shared-libs
shared-libs: check-tools
	@echo "Building shared libraries..."
	@echo "✓ No shared libraries to build yet"

# Clean all subdirectories
.PHONY: clean-all
clean-all:
	@echo "Cleaning all C projects..."
	@for dir in */; do \
		if [ -f "$$dir/Makefile" ]; then \
			$(MAKE) -C "$$dir" clean || true; \
		fi; \
	done
	@echo "✓ Clean complete"

# Help target
.PHONY: help
help:
	@echo "Parent Makefile for C projects"
	@echo ""
	@echo "Available targets:"
	@echo "  check-tools   - Verify required tools are installed"
	@echo "  shared-libs   - Build shared libraries"
	@echo "  clean-all     - Clean all subdirectories"
	@echo "  help          - Show this help message"
	@echo ""
	@echo "Common variables:"
	@echo "  CC            - C compiler (default: gcc)"
	@echo "  CFLAGS        - Compiler flags (default: -Wall -Wextra -O2 -std=c11)"
	@echo "  COMMON_LIBS   - Common libraries: $(COMMON_LIBS)"
