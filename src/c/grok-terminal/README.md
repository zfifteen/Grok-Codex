# Grok Terminal

A lightweight C program that establishes an interactive terminal session with the Grok AI via the xAI streaming API.

## Features

- **Real-time Conversational AI**: Stream responses from Grok AI with real-time display
- **Verbose Output Management**: Buffers thinking steps with a rolling 5-line window to keep the terminal clean
- **Filesystem Operations**: Read, write, and list files/directories
- **Bash Command Execution**: Run bash commands directly from the terminal
- **Minimal Dependencies**: Uses only GMP/MPFR (as required by parent Makefile), libcurl, and json-c
- **Elegant Terminal UX**: Mimics the experience of tools like Codex and Claude Code

## Project Structure

```
src/c/grok-terminal/
├── Makefile           # Project-specific Makefile (includes parent)
├── grok_terminal.c    # Main program source
├── demo.sh            # Demonstration script
└── README.md          # This file

src/c/
└── Makefile           # Parent Makefile with shared dependencies (GMP/MPFR)
```

## Requirements

- **Operating System**: macOS or Linux (tested on Ubuntu 24.04)
- **Compiler**: GCC or Clang
- **Dependencies**:
  - libcurl (for HTTP requests with streaming)
  - json-c (for JSON parsing)
  - GMP (GNU Multiple Precision library - required by parent)
  - MPFR (Multiple Precision Floating-Point Reliable library - optional)

## Installation

### 1. Install Dependencies

#### Ubuntu/Debian:
```bash
cd src/c/grok-terminal
make install-deps
```

Or manually:
```bash
sudo apt-get update
sudo apt-get install -y libcurl4-openssl-dev libjson-c-dev libgmp-dev libmpfr-dev
```

#### macOS:
```bash
brew install curl json-c gmp mpfr
```

### 2. Build the Program

```bash
cd src/c/grok-terminal
make
```

This will:
- Check for required dependencies (via parent Makefile)
- Compile `grok_terminal.c` into the `grok-terminal` executable
- Link against libcurl, json-c, and GMP/MPFR

## Usage

### Set Your API Key

Export your xAI API key as an environment variable:

```bash
export GROK_API_KEY='your-xai-api-key-here'
```

Or:
```bash
export XAI_API_KEY='your-xai-api-key-here'
```

### Run the Program

```bash
./grok-terminal
```

### Interactive Commands

Once running, you can use the following commands:

- **Chat with Grok**: Simply type your message and press Enter
  ```
  > Hello, Grok! Tell me about yourself.
  ```

- **Read File**: Read and display file contents
  ```
  > read_file:/path/to/file.txt
  ```

- **Write File**: Write content to a file
  ```
  > write_file:/path/to/file.txt:Your content here
  ```

- **List Directory**: List contents of a directory
  ```
  > list_dir:/path/to/directory
  ```

- **Execute Bash Command**: Run a bash command
  ```
  > bash:ls -la
  ```

- **Exit**: Quit the program
  ```
  > exit
  ```

## Demo Script

Run the included demo script to see all features in action:

```bash
./demo.sh
```

**Note**: The demo script only demonstrates filesystem and bash operations. To test the Grok AI integration, you need a valid API key.

## API Configuration

The program uses the xAI API with the following defaults:
- **Endpoint**: `https://api.x.ai/v1/chat/completions`
- **Model**: `grok-code-fast-1`
- **Streaming**: Enabled (Server-Sent Events)
- **Max Tokens**: 4096

## Verbose Output Management

When Grok generates verbose outputs (like thinking steps or tool results), the terminal:
1. **During Streaming**: Shows only the last 5 lines in a rolling window
2. **After Completion**: Displays the full final response

This prevents excessive scrolling and keeps the terminal clean, similar to Codex and Claude Code.

## Architecture

### Main Components

1. **API Integration** (`send_grok_request`):
   - Constructs JSON payloads for the xAI API
   - Handles streaming HTTP requests with libcurl
   - Parses Server-Sent Events (SSE) responses

2. **Response Handler** (`write_callback`):
   - Processes incoming SSE chunks incrementally
   - Extracts content deltas from JSON responses
   - Displays tokens in real-time for immediate feedback

3. **Filesystem Operations**:
   - `handle_read_file`: Read and display file contents
   - `handle_write_file`: Write content to files
   - `handle_list_dir`: List directory contents

4. **Bash Execution** (`handle_bash_command`):
   - Executes bash commands via `popen`
   - Captures and displays output

5. **Interactive Loop** (`main`):
   - Prompts for user input
   - Routes commands to appropriate handlers
   - Manages session lifecycle

### Memory Management

- Uses dynamic buffers with fixed capacity (1MB max)
- Proper cleanup with `free_response_state`
- GMP library available for large number operations

## Building and Testing

### Build
```bash
make
```

### Clean
```bash
make clean
```

### Test with Demo
```bash
make test
```

### Check Dependencies
```bash
make check-deps
```

## Security Considerations

- API keys are read from environment variables (not stored in code)
- HTTPS with certificate verification enabled
- Bash commands execute with user privileges (be cautious with user input)
- File operations respect filesystem permissions

## Performance

- **Lightweight**: Single-file implementation under 500 lines
- **Efficient**: Incremental parsing of streaming responses
- **No Threads**: Uses synchronous curl for simplicity
- **Minimal Allocations**: Fixed-size buffers where possible

## Troubleshooting

### "Error: libcurl not found"
Install libcurl development headers:
```bash
sudo apt-get install libcurl4-openssl-dev
```

### "Error: json-c not found"
Install json-c development headers:
```bash
sudo apt-get install libjson-c-dev
```

### "Error: GROK_API_KEY not set"
Export your API key:
```bash
export GROK_API_KEY='your-key-here'
```

### HTTP Errors
Check that your API key is valid and you have network connectivity:
```bash
curl -X POST https://api.x.ai/v1/chat/completions \
  -H "Authorization: Bearer $GROK_API_KEY" \
  -H "Content-Type: application/json" \
  -d '{"model":"grok-code-fast-1","messages":[{"role":"user","content":"test"}],"stream":false}'
```

## License

This project is part of the Grok-Codex repository. See the parent LICENSE file for details.

## Contributing

This is a demonstration project. For issues or enhancements, please follow the repository's contribution guidelines.

## Acknowledgments

- Built using the xAI API (https://api.x.ai)
- Inspired by Codex and Claude Code terminal experiences
- Uses libcurl for HTTP, json-c for JSON parsing, and GMP/MPFR for large numbers
