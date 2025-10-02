# Simplified Makefile for RSA-4096 Solver Prototype
# Builds rsa_solver using Z5D factorization shortcut
# Deps from parent /src/c/Makefile (MPFR/GMP/OpenSSL)

CC = clang
CFLAGS = -Wall -Wextra -Wno-deprecated-declarations -O2 -std=c99 -fopenmp
OPENSSL_PREFIX = $(shell brew --prefix openssl 2>/dev/null || echo "/usr/local/opt/openssl")
GMP_PREFIX = $(shell brew --prefix gmp 2>/dev/null || echo "/usr/local/opt/gmp")
MPFR_PREFIX = $(shell brew --prefix mpfr 2>/dev/null || echo "/usr/local/opt/mpfr")
LIBOMP_PREFIX = $(shell brew --prefix libomp 2>/dev/null || echo "/usr/local/opt/libomp")
INCLUDES = $(shell pkg-config --cflags openssl) -I$(GMP_PREFIX)/include -I$(MPFR_PREFIX)/include -I$(LIBOMP_PREFIX)/include
LDFLAGS = -L$(OPENSSL_PREFIX)/lib -L$(GMP_PREFIX)/lib -L$(MPFR_PREFIX)/lib -L$(LIBOMP_PREFIX)/lib
LIBS = -lmpfr -lgmp $(shell pkg-config --libs openssl) -lm -lomp

# Default target: build rsa_solver
all: rsa_solver epsilon_tuning

# Build the solver binary
rsa_solver: rsa_solver.c z5d_factorization_shortcut.c z5d_factorization_shortcut.h
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ rsa_solver.c z5d_factorization_shortcut.c $(LIBS)

# Build epsilon tuning binary
epsilon_tuning: epsilon_tuning.c z5d_factorization_shortcut.c z5d_factorization_shortcut.h
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ epsilon_tuning.c z5d_factorization_shortcut.c $(LIBS)

# Clean build artifacts
clean:
	rm -f rsa_solver epsilon_tuning *.o

# Test: Build and run demo
test: all
	./demo.sh

.PHONY: all clean test