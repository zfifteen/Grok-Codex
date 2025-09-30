# Grok Terminal - Usage Examples

This document provides comprehensive examples of using the Grok Terminal program.

## Table of Contents

1. [Basic Setup](#basic-setup)
2. [File Operations](#file-operations)
3. [Directory Operations](#directory-operations)
4. [Bash Commands](#bash-commands)
5. [Conversational AI](#conversational-ai)
6. [Advanced Usage](#advanced-usage)

## Basic Setup

### Setting up the API Key

```bash
# Using GROK_API_KEY
export GROK_API_KEY='your-xai-api-key-here'

# Or using XAI_API_KEY
export XAI_API_KEY='your-xai-api-key-here'
```

### Starting the Terminal

```bash
cd src/c/grok-terminal
./grok-terminal
```

You'll see a welcome message with available commands.

## File Operations

### Reading Files

Read and display file contents:

```
> read_file:/etc/hostname
--- Content of /etc/hostname ---
runnervm
--- End of file ---
```

### Writing Files

Write content to a file:

```
> write_file:/tmp/myfile.txt:Hello, this is test content
✓ Written to /tmp/myfile.txt
```

Note: The format is `write_file:<path>:<content>`

### Verifying Written Files

```
> read_file:/tmp/myfile.txt
--- Content of /tmp/myfile.txt ---
Hello, this is test content
--- End of file ---
```

## Directory Operations

### Listing Directory Contents

List files and subdirectories:

```
> list_dir:/tmp
--- Contents of /tmp ---
  [FILE] myfile.txt (27 bytes)
  [DIR]  systemd-private-xyz/
  [FILE] test.txt (12 bytes)
--- End of listing ---
```

### Listing Current Directory

```
> list_dir:.
--- Contents of . ---
  [FILE] Makefile (2397 bytes)
  [FILE] grok_terminal.c (14204 bytes)
  [FILE] README.md (6480 bytes)
  [FILE] demo.sh (2940 bytes)
--- End of listing ---
```

## Bash Commands

### Simple Commands

Execute any bash command:

```
> bash:echo "Hello from Grok Terminal"
--- Executing: echo "Hello from Grok Terminal" ---
Hello from Grok Terminal
--- Exit code: 0 ---
```

### System Information

```
> bash:uname -a
--- Executing: uname -a ---
Linux runnervm 6.11.0-1018-azure #18~24.04.1-Ubuntu SMP ...
--- Exit code: 0 ---
```

### Multiple Commands

```
> bash:echo "Current user: $(whoami)" && echo "Current directory: $(pwd)"
--- Executing: echo "Current user: $(whoami)" && echo "Current directory: $(pwd)" ---
Current user: runner
Current directory: /home/runner/work/Grok-Codex/Grok-Codex/src/c/grok-terminal
--- Exit code: 0 ---
```

### File Operations via Bash

```
> bash:ls -lh grok-terminal
--- Executing: ls -lh grok-terminal ---
-rwxr-xr-x 1 runner runner 27K Sep 30 14:34 grok-terminal
--- Exit code: 0 ---
```

## Conversational AI

### Simple Questions

Ask Grok AI anything (requires valid API key):

```
> What is the capital of France?
Grok: The capital of France is Paris. It's not only the political and administrative 
center but also the cultural heart of the country...

```

### Code Questions

```
> Can you explain what a hash table is?
Grok: A hash table, also known as a hash map, is a data structure that implements 
an associative array abstract data type...

```

### Technical Assistance

```
> How do I compile a C program with libcurl?
Grok: To compile a C program that uses libcurl, you need to:
1. Install libcurl development headers...
2. Use pkg-config to get the right flags...
3. Compile with: gcc myprogram.c $(pkg-config --cflags --libs libcurl)...

```

## Advanced Usage

### Combining Operations

You can chain multiple operations in a session:

```
> bash:mkdir -p /tmp/test_project
--- Executing: mkdir -p /tmp/test_project ---
--- Exit code: 0 ---

> write_file:/tmp/test_project/README.md:# My Project\n\nThis is a test project.
✓ Written to /tmp/test_project/README.md

> list_dir:/tmp/test_project
--- Contents of /tmp/test_project ---
  [FILE] README.md (41 bytes)
--- End of listing ---

> read_file:/tmp/test_project/README.md
--- Content of /tmp/test_project/README.md ---
# My Project

This is a test project.
--- End of file ---
```

### Using with Scripts

Create a script that feeds commands to grok-terminal:

```bash
#!/bin/bash
# automation.sh

export GROK_API_KEY='your-key-here'

./grok-terminal <<EOF
bash:date > /tmp/timestamp.txt
read_file:/tmp/timestamp.txt
bash:echo "Processing complete"
exit
EOF
```

Run it:
```bash
chmod +x automation.sh
./automation.sh
```

### Debugging Session

```
> bash:gcc -v 2>&1 | grep version
--- Executing: gcc -v 2>&1 | grep version ---
gcc version 13.2.0 (Ubuntu 13.2.0-23ubuntu4)
--- Exit code: 0 ---

> bash:pkg-config --modversion libcurl
--- Executing: pkg-config --modversion libcurl ---
8.5.0
--- Exit code: 0 ---

> bash:echo "Build environment OK"
--- Executing: echo "Build environment OK" ---
Build environment OK
--- Exit code: 0 ---
```

## Streaming Behavior

When sending messages to Grok AI with a valid API key, the terminal:

1. **Displays tokens in real-time** as they arrive from the API
2. **Buffers verbose outputs** (like thinking steps) in a rolling 5-line window
3. **Shows final response** completely after streaming completes
4. **Minimizes scrolling** to keep the terminal clean

Example with verbose output:
```
> Can you analyze this code and suggest improvements?
[Thinking 1]: Analyzing the provided code structure...
[Thinking 2]: Identifying potential optimization points...
[Thinking 3]: Checking for memory safety issues...
[Thinking 4]: Evaluating error handling patterns...
[Thinking 5]: Preparing recommendations...

Grok: Based on my analysis, here are the key improvements:
1. Add bounds checking for array access
2. Use safer string functions...
3. Implement proper error handling...

```

## Tips and Best Practices

### 1. File Paths

Always use absolute paths or paths relative to the current directory:

```
> read_file:./README.md        # Good: relative path
> read_file:README.md           # Also works
> read_file:/tmp/file.txt       # Good: absolute path
```

### 2. File Writing

For multi-line content, use escape sequences:

```
> write_file:/tmp/multi.txt:Line 1\nLine 2\nLine 3
✓ Written to /tmp/multi.txt
```

### 3. Bash Commands

Remember that bash commands execute in the current environment:

```
> bash:cd /tmp && ls
# This lists /tmp contents but doesn't change grok-terminal's working directory
```

### 4. Security

- Never expose your API key in scripts committed to version control
- Be cautious with bash commands from untrusted input
- File operations respect filesystem permissions

### 5. Error Handling

The program provides clear error messages:

```
> read_file:/nonexistent/file.txt
Error: Cannot open file '/nonexistent/file.txt'

> bash:false
--- Executing: false ---
--- Exit code: 1 ---
```

## Exit Commands

To exit the terminal:

```
> exit
Goodbye!
```

Or use Ctrl+D (EOF) at the prompt.

## Troubleshooting

### API Key Not Set

```
Error: GROK_API_KEY or XAI_API_KEY environment variable not set
Export your API key: export GROK_API_KEY='your-key-here'
```

**Solution**: Set the environment variable before starting the terminal.

### File Permission Errors

```
Error: Cannot write to file '/protected/file.txt'
```

**Solution**: Check file permissions or use a different path.

### Command Not Found

```
bash: command-not-found: command not found
```

**Solution**: The command isn't available in your system. Install it first.

## Performance Notes

- File operations are synchronous and complete immediately
- Bash commands execute and wait for completion
- API calls stream responses in real-time
- Maximum input size: 4KB
- Maximum response buffer: 1MB

## See Also

- [README.md](README.md) - Main documentation
- [demo.sh](demo.sh) - Automated demonstration
- [xAI API Documentation](https://docs.x.ai/) - Official API docs
