# Implementation Plan: System Instruction with Auto-Session Management

## Objective
Initialize grok-terminal with a system instruction before first user interaction, display Grok's response as welcome message, and automatically manage conversation sessions when they grow too large.

## Implementation Steps

### 1. Add constants (after line 36)
```c
#define SYSTEM_INSTRUCTION "Agent Mode"
#define MAX_CONVERSATION_BYTES 512000  // 500 KB threshold
```

### 2. Create conversation history structure (after line 50)
```c
typedef struct {
    char **roles;           // Array of role strings ("system"/"user"/"assistant")
    char **contents;        // Array of message content strings
    int count;              // Number of messages
    int capacity;           // Allocated capacity
    size_t total_bytes;     // Running byte count (cheap tracking)
} ConversationHistory;
```

### 3. Conversation history management functions (after line 93)
- `ConversationHistory* init_conversation_history()` - allocate with initial capacity
- `void add_message(ConversationHistory *hist, const char *role, const char *content)`
  - Append message to arrays
  - Update `total_bytes += strlen(role) + strlen(content)`
  - Reallocate arrays if at capacity
- `void reset_to_last_n(ConversationHistory *hist, int n)`
  - Keep system instruction (index 0) + last N messages
  - Free and shift other messages
  - Recalculate `total_bytes` from remaining messages
- `void free_conversation_history(ConversationHistory *hist)` - full cleanup

### 4. Create `send_system_instruction()` function (after line 255)
- Signature: `char* send_system_instruction(const char *api_key)`
- Build JSON with single system-role message: `SYSTEM_INSTRUCTION`
- Use existing streaming infrastructure (`ResponseState`, `write_callback`)
- Return allocated string with Grok's welcome response
- Return NULL on error

### 5. Modify `send_grok_request()` to accept history (line 175)
- New signature: `int send_grok_request(const char *api_key, const char *user_message, ConversationHistory *history)`
- Build messages array from `history->roles[]` and `history->contents[]`
- Append new user message to messages array
- After successful streaming response, add both:
  - User message to history
  - Assistant response (from `state.final_response`) to history
- Check if `history->total_bytes > MAX_CONVERSATION_BYTES` after adding
- If exceeded:
  - Display: `printf("\n[Session continuation: conversation reset to maintain performance]\n\n")`
  - Extract last 2 exchanges (4 messages: 2 user + 2 assistant)
  - Call `reset_to_last_n(history, 4)` - keeps system instruction + last 4 messages
  - Re-send system instruction to start fresh context (in background, no display)

### 6. Update `main()` function (line 353)
- Initialize conversation history after API key validation
- Call `send_system_instruction(api_key)` → `welcome_msg`
- Display: `printf("Grok: %s\n\n", welcome_msg)`
- Add to history: system instruction + welcome response
- Free `welcome_msg`
- Update all `send_grok_request()` calls to pass history
- Free conversation history before exit

### 7. Testing strategy (new test cases)
- **Test 1**: Add 3 messages, verify `total_bytes` calculation
- **Test 2**: Add messages until exceeds 500KB, verify auto-reset triggers
- **Test 3**: After reset, verify only system instruction + last 4 messages remain
- **Test 4**: Verify `total_bytes` recalculated correctly after reset
- **Test 5**: Edge case - reset with fewer than 4 messages available
- **Test 6**: Multiple resets in single session
- **Test 7**: Memory leak check (valgrind) with full conversation lifecycle

### 8. Error handling
- `send_system_instruction()` failure → show default welcome, continue with empty history
- Memory allocation failures → graceful degradation
- Session reset failure → log error but continue with existing history

## Session Reset Flow
```
Conversation grows...
  ↓
Response completes → total_bytes = 520,000 (exceeds 500KB)
  ↓
Display: "[Session continuation: conversation reset to maintain performance]"
  ↓
Keep: [system instruction, user_msg_N-1, assistant_msg_N-1, user_msg_N, assistant_msg_N]
  ↓
total_bytes recalculated (now ~2KB)
  ↓
Continue interactive loop with fresh context
```

## Key Design Decisions
- **Cheap byte tracking**: Simple `strlen()` sum, updated incrementally (no JSON serialization overhead)
- **Last 2 exchanges**: Provides minimal context continuity across sessions
- **System instruction persists**: Always index 0, never removed
- **Silent continuation**: User informed but no interaction required
- **Test-driven**: History logic is complex, comprehensive tests essential

## File Locations
- Main implementation: `src/c/grok-terminal/grok_terminal.c`
- Build system: `src/c/grok-terminal/Makefile`
- System instruction: Hard-coded as "Agent Mode" (modifiable later)

## Next Steps
1. Implement conversation history structure and functions
2. Create `send_system_instruction()` function
3. Modify `send_grok_request()` to use history
4. Update `main()` initialization sequence
5. Write comprehensive tests for history management
6. Test session reset logic with large conversations
7. Memory leak validation with valgrind
