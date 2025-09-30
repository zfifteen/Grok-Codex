# Requirements: Grok Terminal Tool Calling Implementation

**Evaluation Date**: 2025-09-30
**Specification**: OpenAI Function Calling API Pattern
**Current Status**: ❌ NOT COMPLIANT

---

## Executive Summary

The current implementation is a **command-line REPL with string-based command parsing**, not an **AI agent with tool calling capabilities**. It intercepts user input before sending to Grok, preventing the AI from autonomously deciding when and how to use tools.

### Critical Gap
Grok cannot call tools. Users must manually type special command syntax (`read_file:path`, `bash:cmd`) which bypasses the AI entirely.

---

## Current Architecture (Incorrect)

```
User Input
    ↓
String Parser (lines 411-424)
    ↓
┌───────────────┬──────────────┐
│ Special       │ Everything   │
│ Commands      │ Else         │
│               │              │
│ read_file:    │ Send to Grok │
│ write_file:   │ API          │
│ list_dir:     │              │
│ bash:         │              │
│               │              │
│ Execute       │              │
│ Locally       │              │
│ (No Grok)     │              │
└───────────────┴──────────────┘
```

**Problems**:
1. Grok never sees tool commands
2. Grok cannot decide when to use tools
3. No feedback loop (Grok can't reason about tool results)
4. User must memorize command syntax
5. No conversation history for multi-turn interactions

---

## Required Architecture (OpenAI Spec Compliant)

```
User Input
    ↓
Send to Grok API (with tool definitions)
    ↓
Grok Analyzes Intent
    ↓
┌─────────────────┬─────────────────────┐
│ Regular Message │ Tool Call Request   │
│                 │                     │
│ Display to User │ Execute Locally     │
│                 │      ↓              │
│                 │ Capture Result      │
│                 │      ↓              │
│                 │ Send Back to Grok   │
│                 │      ↓              │
│                 │ Grok Processes      │
│                 │      ↓              │
│                 │ Final Response      │
└─────────────────┴─────────────────────┘
```

---

## Required Changes

### 1. Tool Definitions in API Requests

**Location**: Lines 197-216 (JSON payload construction)

**Current**:
```c
json_object_object_add(root, "model", json_object_new_string(MODEL));
json_object_object_add(root, "messages", messages);
json_object_object_add(root, "stream", json_object_new_boolean(1));
json_object_object_add(root, "max_tokens", json_object_new_int(4096));
```

**Required Addition**:
```c
json_object_object_add(root, "tools", create_tools_array());
json_object_object_add(root, "tool_choice", json_object_new_string("auto"));
```

**Tool Definition Format** (per OpenAI spec):
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
    {
      "type": "function",
      "function": {
        "name": "write_file",
        "description": "Write content to a file on the local filesystem, overwriting if exists",
        "parameters": {
          "type": "object",
          "properties": {
            "filepath": {
              "type": "string",
              "description": "Path to the file to write"
            },
            "content": {
              "type": "string",
              "description": "Content to write to the file"
            }
          },
          "required": ["filepath", "content"]
        }
      }
    },
    {
      "type": "function",
      "function": {
        "name": "list_dir",
        "description": "List contents of a directory with file/directory type and sizes",
        "parameters": {
          "type": "object",
          "properties": {
            "dirpath": {
              "type": "string",
              "description": "Path to directory to list"
            }
          },
          "required": ["dirpath"]
        }
      }
    },
    {
      "type": "function",
      "function": {
        "name": "bash",
        "description": "Execute a bash command and return stdout, stderr, and exit code",
        "parameters": {
          "type": "object",
          "properties": {
            "command": {
              "type": "string",
              "description": "Bash command to execute"
            }
          },
          "required": ["command"]
        }
      }
    }
  ]
}
```

---

### 2. Tool Call Detection in Streaming Response

**Location**: Lines 126-155 (`write_callback` JSON parsing)

**Current**:
```c
/* Only extracts text content */
if (json_object_object_get_ex(delta, "content", &content)) {
    const char *text = json_object_get_string(content);
    printf("%s", text);
}
```

**Required Addition**:
```c
/* Check for tool calls */
struct json_object *tool_calls;
if (json_object_object_get_ex(delta, "tool_calls", &tool_calls)) {
    /* Parse tool call:
     * - tool_calls[0].id (string)
     * - tool_calls[0].type (should be "function")
     * - tool_calls[0].function.name (string)
     * - tool_calls[0].function.arguments (JSON string)
     */
    handle_tool_call(tool_calls);
}
```

**OpenAI Tool Call Response Format**:
```json
{
  "choices": [{
    "delta": {
      "tool_calls": [{
        "id": "call_abc123",
        "type": "function",
        "function": {
          "name": "read_file",
          "arguments": "{\"filepath\": \"/etc/hosts\"}"
        }
      }]
    },
    "finish_reason": "tool_calls"
  }]
}
```

**Streaming Behavior**:
- Tool calls may arrive incrementally across multiple chunks
- `arguments` JSON string may be partial and need accumulation
- `finish_reason: "tool_calls"` signals completion
- Multiple tool calls can be requested in parallel

---

### 3. Tool Execution and Result Feedback

**New Functionality Required**:

```c
/* Execute tool and capture result */
char* execute_tool(const char *tool_name, const char *arguments_json) {
    /* Parse arguments JSON */
    struct json_object *args = json_tokener_parse(arguments_json);

    /* Dispatch to appropriate handler */
    if (strcmp(tool_name, "read_file") == 0) {
        const char *filepath = json_object_get_string(
            json_object_object_get(args, "filepath"));
        return capture_read_file_output(filepath);
    }
    else if (strcmp(tool_name, "write_file") == 0) {
        /* ... */
    }
    /* etc. */

    json_object_put(args);
}

/* Send tool result back to Grok */
void send_tool_result(const char *api_key,
                      ConversationHistory *history,
                      const char *tool_call_id,
                      const char *tool_result) {
    /* Add tool result message to history */
    struct json_object *tool_msg = json_object_new_object();
    json_object_object_add(tool_msg, "role", json_object_new_string("tool"));
    json_object_object_add(tool_msg, "tool_call_id", json_object_new_string(tool_call_id));
    json_object_object_add(tool_msg, "content", json_object_new_string(tool_result));

    /* Send follow-up request with full history including tool result */
    send_grok_request_with_history(api_key, history);
}
```

**Tool Result Message Format** (per OpenAI spec):
```json
{
  "role": "tool",
  "tool_call_id": "call_abc123",
  "content": "File contents: Hello, world!\n"
}
```

---

### 4. Conversation History Management

**Current Problem**: Each API request creates fresh messages array (line 199)

**Required**:
```c
typedef struct {
    struct json_object **messages;  /* Array of message objects */
    int count;
    int capacity;
} ConversationHistory;

/* Initialize with system message */
ConversationHistory* init_conversation() {
    ConversationHistory *history = malloc(sizeof(ConversationHistory));
    history->messages = malloc(sizeof(struct json_object*) * 10);
    history->count = 0;
    history->capacity = 10;

    /* Add system instruction */
    add_message(history, "system", SYSTEM_INSTRUCTION, NULL);

    return history;
}

/* Add message to history */
void add_message(ConversationHistory *history,
                 const char *role,
                 const char *content,
                 struct json_object *tool_calls) {
    /* Reallocate if needed */
    if (history->count >= history->capacity) {
        history->capacity *= 2;
        history->messages = realloc(history->messages,
                                    sizeof(struct json_object*) * history->capacity);
    }

    /* Create message object */
    struct json_object *msg = json_object_new_object();
    json_object_object_add(msg, "role", json_object_new_string(role));

    if (content) {
        json_object_object_add(msg, "content", json_object_new_string(content));
    }

    if (tool_calls) {
        json_object_object_add(msg, "tool_calls", tool_calls);
    }

    history->messages[history->count++] = msg;
}

/* Build messages array from history */
struct json_object* build_messages_array(ConversationHistory *history) {
    struct json_object *messages = json_object_new_array();

    for (int i = 0; i < history->count; i++) {
        json_object_array_add(messages,
                             json_object_get(history->messages[i]));
    }

    return messages;
}
```

**Message Types in History**:
1. **System**: Role instructions (first message, persists)
2. **User**: User input
3. **Assistant**: Grok's text responses and tool call requests
4. **Tool**: Results from tool executions

---

### 5. Remove Command String Parser

**Location**: Lines 411-424

**Current (Delete This)**:
```c
/* Check for special commands */
if (strncmp(input, "read_file:", 10) == 0) {
    handle_read_file(input + 10);
} else if (strncmp(input, "write_file:", 11) == 0) {
    /* ... */
} else if (strncmp(input, "list_dir:", 9) == 0) {
    /* ... */
} else if (strncmp(input, "bash:", 5) == 0) {
    /* ... */
} else {
    /* Send to Grok API */
}
```

**Replace With**:
```c
/* All user input goes to Grok (except "exit") */
if (strcmp(input, "exit") == 0) {
    printf("Goodbye!\n");
    break;
}

/* Add user message to history */
add_message(conversation_history, "user", input, NULL);

/* Send to Grok API with full history */
send_grok_request(api_key, conversation_history);
```

---

### 6. Modified Tool Handler Signatures

**Current**:
```c
void handle_read_file(const char *filepath);
void handle_write_file(const char *filepath, const char *content);
void handle_list_dir(const char *dirpath);
void handle_bash_command(const char *command);
```

**Required** (return string for tool result):
```c
char* tool_read_file(const char *filepath);
char* tool_write_file(const char *filepath, const char *content);
char* tool_list_dir(const char *dirpath);
char* tool_bash_command(const char *command);
```

**Example**:
```c
char* tool_read_file(const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        char *error = malloc(256);
        snprintf(error, 256, "Error: Cannot open file '%s': %s",
                filepath, strerror(errno));
        return error;
    }

    /* Read entire file into buffer */
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *content = malloc(size + 1);
    fread(content, 1, size, fp);
    content[size] = '\0';

    fclose(fp);
    return content;  /* Caller must free */
}
```

---

## Implementation Flow

### Complete Tool Calling Cycle

1. **User sends message**: "What's in the config.json file?"

2. **Send to Grok with tools**:
```json
{
  "model": "grok-code-fast-1",
  "messages": [
    {"role": "system", "content": "Agent Mode..."},
    {"role": "user", "content": "What's in the config.json file?"}
  ],
  "tools": [...tool definitions...],
  "stream": true
}
```

3. **Grok responds with tool call**:
```json
{
  "choices": [{
    "delta": {
      "tool_calls": [{
        "id": "call_xyz789",
        "function": {
          "name": "read_file",
          "arguments": "{\"filepath\": \"config.json\"}"
        }
      }]
    },
    "finish_reason": "tool_calls"
  }]
}
```

4. **Execute tool locally**:
```c
char *result = tool_read_file("config.json");
// result = "{\"database\": \"postgres\", \"port\": 5432}"
```

5. **Add messages to history**:
   - Assistant message with tool call
   - Tool message with result

6. **Send follow-up request**:
```json
{
  "model": "grok-code-fast-1",
  "messages": [
    {"role": "system", "content": "..."},
    {"role": "user", "content": "What's in the config.json file?"},
    {"role": "assistant", "tool_calls": [{"id": "call_xyz789", ...}]},
    {"role": "tool", "tool_call_id": "call_xyz789", "content": "{\"database\": ...}"}
  ],
  "tools": [...],
  "stream": true
}
```

7. **Grok responds with analysis**:
```
The config.json file contains database configuration with PostgreSQL
on port 5432. Would you like me to explain any of these settings?
```

---

## State Machine

```
┌─────────────┐
│   WAITING   │ ← Start
│  FOR INPUT  │
└─────┬───────┘
      │ User input
      ↓
┌─────────────┐
│   SENDING   │
│  TO GROK    │
└─────┬───────┘
      │
      ↓
┌─────────────────┐
│   STREAMING     │
│   RESPONSE      │
└────┬────────┬───┘
     │        │
     │        └─────────────┐
     │ Text content         │ Tool call
     ↓                      ↓
┌─────────────┐      ┌──────────────┐
│   DISPLAY   │      │  EXECUTING   │
│   MESSAGE   │      │     TOOL     │
└─────┬───────┘      └──────┬───────┘
      │                     │ Got result
      │                     ↓
      │              ┌──────────────┐
      │              │   SENDING    │
      │              │ TOOL RESULT  │
      │              └──────┬───────┘
      │                     │
      │                     ↓
      │              ┌──────────────┐
      │              │  STREAMING   │
      │              │   RESPONSE   │
      └──────────────┴──────┬───────┘
                            │
                            ↓
                     ┌─────────────┐
                     │   WAITING   │
                     │  FOR INPUT  │
                     └─────────────┘
```

---

## Behavior Examples

### Example 1: Simple Tool Use

**User**: "Show me the current directory contents"

**Grok Decision**: Calls `list_dir` with `{"dirpath": "."}`

**System**:
1. Executes `list_dir(".")`
2. Captures output
3. Sends back to Grok

**Grok Response**: "Your current directory contains 3 files: grok_terminal.c (15KB), Makefile (2KB), and README.md (4KB)"

---

### Example 2: Multi-Step Tool Use

**User**: "Find all TODO comments in the C file and save them to todos.txt"

**Grok Decision**:
1. Calls `bash` with `{"command": "grep -n TODO grok_terminal.c"}`
2. Receives results
3. Calls `write_file` with `{"filepath": "todos.txt", "content": "..."}`

**System**: Executes both tools sequentially, sends results back

**Grok Response**: "I found 3 TODO comments and saved them to todos.txt"

---

### Example 3: Error Handling

**User**: "Read the file that-doesnt-exist.txt"

**Grok Decision**: Calls `read_file` with `{"filepath": "that-doesnt-exist.txt"}`

**System**: Returns error string: "Error: Cannot open file: No such file or directory"

**Grok Response**: "The file doesn't exist. Would you like me to create it?"

---

## Testing Requirements

### Unit Tests
1. Tool definition JSON formatting
2. Tool call parsing from streaming response
3. Tool execution with various inputs
4. Tool result formatting
5. Conversation history management

### Integration Tests
1. End-to-end tool calling cycle
2. Multiple sequential tool calls
3. Parallel tool calls (if supported)
4. Tool call with errors
5. Streaming response with interleaved content and tool calls

### Manual Test Cases
1. "List files in current directory"
2. "Read grok_terminal.c and count the functions"
3. "Run 'uname -a' and tell me the OS"
4. "Create a file called test.txt with 'Hello World'"
5. "What's in the Makefile?"

---

## Dependencies

### Required Libraries (Already Present)
- libcurl (HTTP/HTTPS)
- json-c (JSON parsing/generation)

### New Structures Required
- `ConversationHistory` - Message history management
- `ToolCall` - Accumulated tool call data during streaming
- `ToolDefinition` - Tool metadata for API requests

---

## Security Considerations

### Before Tool Calling
- User manually types dangerous commands
- No validation, executes directly

### After Tool Calling
- Grok decides what commands to run
- **Still requires same security hardening**:
  - Path traversal validation
  - Command injection prevention
  - Sandboxing/whitelisting

### Additional Risk
- AI-generated commands could be unpredictable
- Need logging of all tool executions
- Consider requiring user approval for destructive operations

---

## Performance Considerations

### Streaming with Tool Calls
- Tool call arguments may arrive across multiple chunks
- Need buffering to accumulate complete JSON
- `finish_reason: "tool_calls"` signals completion

### Conversation History Growth
- History grows with each turn
- Need truncation strategy (keep last N messages)
- Consider token counting for API limits

### Tool Execution Latency
- Tool execution blocks response streaming
- Consider timeout mechanisms
- Large file reads could be slow

---

## Migration Path

### Phase 1: Foundation
1. Implement `ConversationHistory` structure
2. Add tool definition creation functions
3. Modify `send_grok_request()` to include tools in payload

### Phase 2: Detection
1. Extend `write_callback()` to detect tool calls
2. Accumulate tool call data during streaming
3. Handle `finish_reason: "tool_calls"`

### Phase 3: Execution Loop
1. Implement tool execution dispatcher
2. Capture tool results as strings
3. Add tool result messages to history
4. Implement follow-up request sending

### Phase 4: Cleanup
1. Remove command string parser (lines 411-424)
2. Remove `display_help()` command list
3. Update welcome message

### Phase 5: Testing & Hardening
1. Add logging for all tool calls
2. Implement security validations
3. Add error handling for malformed tool calls
4. Performance tuning for large histories

---

## Success Criteria

✅ **Correct Implementation When**:
1. User can ask natural language questions
2. Grok autonomously decides to use tools
3. Tool results flow back to Grok
4. Grok reasons about tool results
5. No special command syntax required
6. Conversation history preserved across turns
7. Multi-step tool workflows function correctly

❌ **Current Status**: None of the above are true

---

## References

- **OpenAI Function Calling**: https://platform.openai.com/docs/guides/function-calling
- **xAI API Documentation**: https://docs.x.ai/api (verify tool calling support)
- **OpenAI Chat Completions**: https://platform.openai.com/docs/api-reference/chat

---

**Document Version**: 1.0
**Last Updated**: 2025-09-30
**Status**: Requirements defined, implementation pending
