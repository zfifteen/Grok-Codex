# C Projects

This directory contains C-based projects for the Grok-Codex repository.

## Structure

```
src/c/
├── Makefile          # Parent Makefile with shared dependencies
└── grok-terminal/    # Interactive Grok AI terminal
    ├── Makefile          # Project-specific Makefile
    ├── grok_terminal.c   # Main implementation
    ├── demo.sh           # Demonstration script
    ├── README.md         # Project documentation
    └── .gitignore        # Git ignore rules
```

## Parent Makefile

The parent Makefile (`src/c/Makefile`) provides:

- **Common dependencies**: GMP and MPFR libraries for large number operations
- **Shared build configuration**: Compiler settings and flags
- **Utility targets**:
  - `make check-tools` - Verify required tools are installed
  - `make shared-libs` - Build shared libraries (placeholder for future)
  - `make clean-all` - Clean all subdirectories
  - `make help` - Show help message

All projects under `src/c/` should include this parent Makefile and use `COMMON_LIBS` for linking.

## Requirements

- **Compiler**: GCC or Clang
- **Build tool**: make
- **Common libraries**:
  - GMP (GNU Multiple Precision Arithmetic Library)
  - MPFR (GNU Multiple Precision Floating-Point Reliable Library)

### Installing Requirements

#### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y build-essential libgmp-dev libmpfr-dev
```

#### macOS:
```bash
brew install gmp mpfr
```

## Projects

### grok-terminal

Interactive terminal session with Grok AI via the xAI streaming API.

**Features**:
- Real-time conversational AI with streaming responses
- Verbose output buffering with rolling window display
- Filesystem operations (read, write, list)
- Bash command execution
- Minimal dependencies (uses only system libraries + GMP/MPFR)

**Documentation**: See [grok-terminal/README.md](grok-terminal/README.md)

**Quick Start**:
```bash
cd grok-terminal
make
export GROK_API_KEY='your-api-key-here'
./grok-terminal
```

## Adding New Projects

To add a new C project under this directory:

1. Create a new subdirectory: `mkdir my-project`
2. Create a Makefile that includes the parent:
   ```makefile
   # Include parent Makefile for common dependencies
   include ../Makefile
   
   # Your project-specific settings
   PROJECT = my-project
   TARGET = $(PROJECT)
   SRC = main.c
   
   # Use COMMON_LIBS for linking
   $(TARGET): $(SRC)
       $(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(COMMON_LIBS)
   ```
3. Implement your program
4. Add a README.md with documentation

## Build Guidelines

### Compiler Flags

The parent Makefile sets these defaults:
- `-Wall -Wextra` - Enable all warnings
- `-O2` - Optimization level 2
- `-std=c11` - Use C11 standard

Projects can add their own flags by extending `CFLAGS`.

### Dependencies

- **Always use GMP/MPFR**: As specified in the parent Makefile, all projects should link against these libraries via `COMMON_LIBS`
- **No new dependencies**: Prefer using system libraries when possible
- **Document dependencies**: If a project needs additional libraries, document them clearly in its README

### Makefiles

Project Makefiles should:
- Include the parent Makefile: `include ../Makefile`
- Define clear targets: `all`, `clean`, `help`
- Check dependencies: Add a `check-deps` target if needed
- Use parent variables: `$(CC)`, `$(CFLAGS)`, `$(COMMON_LIBS)`

## Testing

Each project should provide:
- A way to test the build: `make test` or a demo script
- Documentation on how to run tests
- Clear instructions for verifying functionality

## Documentation

Each project must have:
- A `README.md` with:
  - Project description
  - Features list
  - Installation instructions
  - Usage examples
  - Build instructions
  - Testing guidelines
- Inline code comments for complex logic
- A `.gitignore` to exclude build artifacts

## License

All projects under this directory follow the repository's license. See the parent LICENSE file for details.
