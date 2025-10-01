# Memory Crash Fix - Buffer Overflow Resolution

## Problem

The grok-terminal was experiencing memory corruption crashes with the error:
```
malloc: Incorrect checksum for freed object 0x14b80f000: probably modified after being freed.
Corrupt value: 0x5f4e49422824207c
```

This error indicates heap metadata corruption, typically caused by writing past the end of an allocated buffer.

## Root Cause

The code had multiple **off-by-one buffer overflow bugs** in the buffer expansion checks. The pattern was:

```c
// BUGGY CODE
if (size + new_data_len >= capacity) {
    capacity *= 2;
    buffer = realloc(buffer, capacity);
}
memcpy(buffer + size, new_data, new_data_len);
size += new_data_len;
buffer[size] = '\0';  // BUG: Can write at position 'capacity' when size + new_data_len == capacity
```

When `size + new_data_len == capacity`, the condition `>= capacity` is false, so no reallocation occurs. But after the memcpy and incrementing size, the null terminator write at `buffer[size]` writes to position `capacity`, which is one byte past the allocated buffer. This corrupts the heap metadata stored immediately after the buffer.

## Fixed Locations

### 1. write_callback() - Line 671
**File**: `grok_terminal.c`

**Before**:
```c
if (state->size + total_size >= state->capacity) {
    return total_size;
}
```

**After**:
```c
if (state->size + total_size + 1 > state->capacity) {
    return total_size;
}
```

**Explanation**: Ensures space for the null terminator at `state->data[state->size]`.

### 2. write_callback() - Final Response Buffer - Line 712
**Before**:
```c
if (state->final_response_size + text_len < MAX_RESPONSE_SIZE) {
```

**After**:
```c
if (state->final_response_size + text_len + 1 <= MAX_RESPONSE_SIZE) {
```

**Explanation**: Ensures space for the null terminator after appending text.

### 3. write_callback() - Tool Arguments Buffer - Line 780
**Before**:
```c
if (state->tool_call.arguments_size + args_len >= state->tool_call.arguments_capacity) {
```

**After**:
```c
if (state->tool_call.arguments_size + args_len + 1 > state->tool_call.arguments_capacity) {
```

**Explanation**: Ensures space for the null terminator after appending arguments. This was likely the primary source of the crash since tool arguments can be accumulated across multiple streaming chunks.

### 4. tool_bash_command() - Output Buffer - Line 1113
**Before**:
```c
if (size + line_len >= capacity) {
```

**After**:
```c
if (size + line_len + 1 > capacity) {
```

**Explanation**: Ensures space for the null terminator after appending command output lines.

## Verification

All buffer expansion checks now correctly account for the null terminator:

1. **Pattern**: `if (size + new_len + 1 > capacity)` - Ensures we have space for data + null terminator
2. **Alternative**: `if (size + new_len + 1 <= capacity)` for inclusion checks
3. **Rationale**: After `memcpy(buffer + size, data, len)` and `size += len`, writing `buffer[size] = '\0'` requires that `size < capacity`

## Build System Updates

The Makefile was also updated to build the binary in a `bin/` folder:

- **Binary location**: `bin/grok-terminal` (previously `grok-terminal`)
- **Clean target**: Removes entire `bin/` directory
- **Demo script**: Updated to reference `../bin/grok-terminal`
- **.gitignore**: Added `bin/` to ignore list
- **README.md**: Updated all references to use `./bin/grok-terminal`

## Testing

To test the fixes:

1. Build the program: `make`
2. Run with API key: `export GROK_API_KEY='your-key' && ./bin/grok-terminal`
3. Test tool calling features extensively to verify buffer handling

## Additional Notes

All other memory safety patterns in the code were verified:

- ✅ All `realloc()` calls properly check return value before using new pointer
- ✅ All `strdup()` calls check for NULL return
- ✅ All `malloc()` calls in `init_response_state()` check for NULL
- ✅ The `tool_list_dir()` function uses safe `snprintf()` calls
- ✅ The exit message buffer in `tool_bash_command()` explicitly allocates `size + len + 1`

## Impact

These fixes resolve the memory corruption that was causing crashes during extended use or when processing large streaming responses with tool calls. The heap metadata is no longer corrupted by off-by-one writes.
