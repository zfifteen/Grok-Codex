# Tool Calling Architecture

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         User Input                              │
│                  "What's in the Makefile?"                      │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         v
┌─────────────────────────────────────────────────────────────────┐
│              Add to Conversation History                        │
│   messages: [system, user1, assistant1, ..., userN]            │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         v
┌─────────────────────────────────────────────────────────────────┐
│            Send Request to Grok API                             │
│    - Full conversation history                                  │
│    - Tool definitions (read_file, write_file, etc.)            │
│    - tool_choice: "auto"                                        │
│    - stream: true                                               │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         v
┌─────────────────────────────────────────────────────────────────┐
│              Streaming Response Callback                        │
│                  (write_callback)                               │
└────────────────────────┬────────────────────────────────────────┘
                         │
         ┌───────────────┴───────────────┐
         │                               │
         v                               v
┌──────────────────┐           ┌──────────────────┐
│  Text Content    │           │   Tool Call      │
│  Display to user │           │   Detected       │
└──────────────────┘           └────────┬─────────┘
                                        │
                                        v
                               ┌──────────────────┐
                               │  Execute Tool    │
                               │  Locally         │
                               │  - read_file     │
                               │  - write_file    │
                               │  - list_dir      │
                               │  - bash          │
                               └────────┬─────────┘
                                        │
                                        v
                               ┌──────────────────┐
                               │  Capture Result  │
                               │  as String       │
                               └────────┬─────────┘
                                        │
                                        v
                               ┌──────────────────┐
                               │  Add to History: │
                               │  - Assistant msg │
                               │    (tool_calls)  │
                               │  - Tool msg      │
                               │    (result)      │
                               └────────┬─────────┘
                                        │
                                        v
                               ┌──────────────────┐
                               │  Send Follow-up  │
                               │  Request         │
                               │  (Recursive)     │
                               └────────┬─────────┘
                                        │
                                        v
                               ┌──────────────────┐
                               │  Grok Processes  │
                               │  Tool Result     │
                               └────────┬─────────┘
                                        │
                                        v
                               ┌──────────────────┐
                               │  Final Response  │
                               │  to User         │
                               └──────────────────┘
```

## Data Structures

### ConversationHistory
```c
typedef struct {
    struct json_object **messages;  // Dynamic array of message objects
    int count;                      // Number of messages
    int capacity;                   // Allocated capacity
} ConversationHistory;
```

Messages include:
- **system**: Initial instructions (always first)
- **user**: User requests
- **assistant**: Grok responses and tool call requests
- **tool**: Tool execution results

### ToolCallState
```c
typedef struct {
    char *tool_call_id;           // Unique ID from API
    char *function_name;          // Tool name (read_file, bash, etc.)
    char *arguments;              // JSON arguments (accumulated)
    size_t arguments_capacity;    // Buffer capacity
    size_t arguments_size;        // Current size
} ToolCallState;
```

### ResponseState
```c
typedef struct {
    char *data;                   // SSE buffer
    size_t size;
    size_t capacity;
    char *final_response;         // Accumulated response text
    size_t final_response_size;
    ToolCallState tool_call;      // Accumulated tool call
    int has_tool_call;            // Flag
    // ... other fields
} ResponseState;
```

## Message Flow

### Example 1: Simple Tool Use

**Turn 1:**
```json
[
  {"role": "system", "content": "Agent Mode..."},
  {"role": "user", "content": "List files in current directory"}
]
```
↓
```json
{
  "role": "assistant",
  "tool_calls": [{
    "id": "call_123",
    "function": {"name": "list_dir", "arguments": "{\"dirpath\": \".\"}"}
  }]
}
```
↓ Execute: `list_dir(".")`
↓
```json
{"role": "tool", "tool_call_id": "call_123", "content": "...file listing..."}
```
↓ Send with full history
↓
```json
{"role": "assistant", "content": "Your directory contains..."}
```

### Example 2: Multi-Step Tool Use

**User:** "Find TODOs in the code and save to todos.txt"

**Step 1:** Grok calls `bash` with `grep -n TODO *.c`
**Step 2:** Receives grep output
**Step 3:** Grok calls `write_file` with captured TODOs
**Step 4:** Receives success message
**Step 5:** Grok responds: "I found 3 TODOs and saved them to todos.txt"

## Key Functions

### Main Loop
```c
while (1) {
    // Get user input
    printf("> ");
    fgets(input, sizeof(input), stdin);
    
    // Add to history
    add_message_to_history(history, "user", input, NULL);
    
    // Send to Grok (handles tools automatically)
    send_grok_request(api_key, history);
}
```

### send_grok_request
```c
int send_grok_request(const char *api_key, ConversationHistory *history) {
    // Build request with tools
    json_object_object_add(root, "tools", create_tools_array());
    json_object_object_add(root, "tool_choice", json_object_new_string("auto"));
    
    // Stream response
    curl_easy_perform(curl);
    
    // If tool call detected
    if (state.has_tool_call) {
        char *result = execute_tool(name, args);
        add_message_to_history(history, "assistant", NULL, tool_calls);
        add_message_to_history(history, "tool", result, NULL);
        return send_grok_request(api_key, history);  // Recursive!
    }
    
    // Add final response to history
    add_message_to_history(history, "assistant", final_response, NULL);
}
```

### Tool Execution
```c
char* execute_tool(const char *tool_name, const char *arguments_json) {
    struct json_object *args = json_tokener_parse(arguments_json);
    
    if (strcmp(tool_name, "read_file") == 0) {
        const char *filepath = json_object_get_string(...);
        return tool_read_file(filepath);
    }
    // ... other tools
}
```

## Comparison to Requirements

| Requirement | Status | Implementation |
|------------|--------|----------------|
| Tool definitions in API requests | ✅ | `create_tools_array()` |
| Tool call detection in streaming | ✅ | `write_callback` delta parsing |
| Tool execution with results | ✅ | `execute_tool()` dispatcher |
| Follow-up request with results | ✅ | Recursive `send_grok_request()` |
| Conversation history | ✅ | `ConversationHistory` struct |
| Remove command parser | ✅ | Direct to API in main loop |

## Security Considerations

⚠️ **Note**: The current implementation has no security restrictions:
- Path traversal: Users can access any file system path
- Command injection: Any bash command can be executed
- No sandboxing or whitelisting

For production use, add:
- Path validation (prevent `../` traversal)
- Command whitelisting
- Execution sandboxing
- Rate limiting
- Audit logging

## Performance Characteristics

- **Memory**: Conversation history grows with each turn (~1KB per message)
- **Network**: Tool calls require additional round trips
- **Latency**: Tool execution adds local processing time
- **Streaming**: Response display is still real-time until tool call

## Advantages Over Previous Design

1. **Natural Language**: Users don't need to remember command syntax
2. **Context Aware**: Grok can use previous conversation to inform tool use
3. **Multi-Step Reasoning**: Grok can chain multiple tools together
4. **Error Handling**: Grok interprets tool errors and can retry or adapt
5. **Flexibility**: Grok decides when tools are needed vs. direct response

## Limitations

1. **Recursive Depth**: No limit on tool call chains (could stack overflow)
2. **History Size**: No truncation strategy for long conversations
3. **Error Recovery**: Limited error handling in tool execution
4. **Concurrency**: No support for parallel tool calls
5. **Testing**: Requires live API key to fully test
