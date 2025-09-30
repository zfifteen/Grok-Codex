# Tool Calling Implementation Test Documentation

## Overview

This document describes the new tool calling implementation based on the OpenAI Function Calling API pattern.

## What Changed

### Before (Command-based REPL):
Users had to type special commands:
- `read_file:path/to/file`
- `write_file:path/to/file:content`
- `list_dir:path/to/dir`
- `bash:command`

These bypassed Grok entirely - the AI never saw these commands.

### After (Tool Calling Pattern):
Users type natural language requests, and Grok decides when to use tools:
- "What files are in the current directory?"
- "Read the config.json file and tell me what's in it"
- "Run 'ls -la' and show me the output"
- "Create a file called test.txt with 'Hello World'"

## Architecture Changes

### 1. Conversation History Management
- Added `ConversationHistory` struct to maintain context across turns
- Messages array grows dynamically as conversation continues
- System instruction is always the first message

### 2. Tool Definitions
Tools are sent to the API with each request:
```json
{
  "tools": [
    {
      "type": "function",
      "function": {
        "name": "read_file",
        "description": "Read and return the contents of a file from the local filesystem",
        "parameters": {
          "type": "object",
          "properties": {
            "filepath": {
              "type": "string",
              "description": "Absolute or relative path to the file to read"
            }
          },
          "required": ["filepath"]
        }
      }
    },
    // ... more tools
  ],
  "tool_choice": "auto"
}
```

### 3. Streaming Response Handling
The `write_callback` function now detects:
- Regular text content (displays to user)
- Tool calls in the delta (accumulates tool call ID, function name, and arguments)

### 4. Tool Execution Loop
When a tool call is detected:
1. Execute the tool locally
2. Add assistant message (with tool_calls) to history
3. Add tool result message to history
4. Send follow-up request with complete history
5. Grok processes the tool result and responds with final answer

## Example Flow

### User Input:
```
> What's in the Makefile?
```

### Step 1: Initial Request
```json
{
  "model": "grok-code-fast-1",
  "messages": [
    {"role": "system", "content": "Agent Mode..."},
    {"role": "user", "content": "What's in the Makefile?"}
  ],
  "tools": [...],
  "stream": true
}
```

### Step 2: Grok Response (Tool Call)
```
[Tool call: read_file]
```

Grok decides to call:
```json
{
  "tool_calls": [{
    "id": "call_xyz789",
    "function": {
      "name": "read_file",
      "arguments": "{\"filepath\": \"Makefile\"}"
    }
  }]
}
```

### Step 3: Tool Execution
The system executes `tool_read_file("Makefile")` and captures the result.

### Step 4: Follow-up Request
```json
{
  "model": "grok-code-fast-1",
  "messages": [
    {"role": "system", "content": "Agent Mode..."},
    {"role": "user", "content": "What's in the Makefile?"},
    {"role": "assistant", "tool_calls": [...]},
    {"role": "tool", "tool_call_id": "call_xyz789", "content": "# Grok Terminal Makefile\n..."}
  ],
  "tools": [...],
  "stream": true
}
```

### Step 5: Grok Final Response
```
Grok: The Makefile contains build instructions for the grok-terminal project.
It includes targets for building, cleaning, checking dependencies, and running tests.
The project depends on libcurl, json-c, gmp, and mpfr libraries...
```

## Testing Without API Key

To test the implementation changes without an actual API key:

1. **Code Structure**: Review the source code to verify:
   - `ConversationHistory` struct is defined
   - `create_tools_array()` generates proper OpenAI tool definitions
   - `write_callback` detects tool_calls in delta
   - `execute_tool()` dispatcher routes to correct tool functions
   - Tool functions return strings instead of printing

2. **Build Test**: Verify compilation succeeds:
   ```bash
   make clean && make
   ```

3. **Static Analysis**: Check the tool definitions match OpenAI spec:
   - Each tool has "type": "function"
   - Each function has name, description, and parameters
   - Parameters follow JSON Schema format with properties and required fields

## Code Quality Improvements

In addition to implementing tool calling, several improvements were made:

1. **Memory Management**: All tool execution functions properly allocate and return strings
2. **Error Handling**: Tools return error messages as strings for Grok to interpret
3. **Forward Declarations**: Added function declarations to avoid implicit declarations
4. **Resource Cleanup**: Conversation history is properly freed on exit

## Verification Checklist

- [x] ConversationHistory structure implemented
- [x] Tool definitions match OpenAI Function Calling spec
- [x] Streaming callback detects tool_calls
- [x] Tool execution returns strings
- [x] Tool results sent back to API
- [x] Conversation history maintained
- [x] Main loop simplified (no command parsing)
- [x] Code compiles without errors
- [ ] Runtime test with valid API key (requires user testing)

## Next Steps for Testing

To fully test this implementation, a user with a valid Grok API key should:

1. Export their API key: `export GROK_API_KEY='your-key-here'`
2. Run the terminal: `./grok-terminal`
3. Try natural language requests that should trigger tools:
   - "List the files in the current directory"
   - "Read the README.md file"
   - "Run 'whoami' command"
   - "Create a file called test.txt with content 'Hello'"
4. Verify Grok autonomously decides to use tools
5. Verify tool results are processed and incorporated into responses
6. Test multi-turn conversations with context

## Files Modified

- `src/c/grok-terminal/grok_terminal.c`: Complete rewrite of tool handling from command parser to OpenAI Function Calling pattern

## Lines Changed

- Added: ~520 lines (structures, tool definitions, execution logic)
- Removed: ~50 lines (command parser, old handlers)
- Modified: ~30 lines (main loop, API request function)
