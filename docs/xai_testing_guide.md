# xAI API Testing Guide

This document describes how to test xAI API compatibility with Codex using the provided test scripts.

## Prerequisites

1. Valid xAI API key from [xAI Console](https://console.x.ai)
2. Python 3.6+ (for Python test script)
3. curl and jq (for shell test script)
4. Network access to api.x.ai

## Setting Up Your API Key

Export your xAI API key as an environment variable:

```bash
export XAI_API_KEY="your-xai-api-key-here"
```

## Running Tests

### Python Test Script

The Python script provides comprehensive testing with detailed output:

```bash
python3 scripts/test_xai_api.py
```

**Test Coverage:**
- Model listing endpoint (`/models`)
- Chat completion with test token
- Streaming response handling
- Error handling (401, 400)
- Response structure validation

### Curl Test Script

The curl script provides simple command-line testing:

```bash
bash scripts/test_xai_curl.sh
```

**Test Coverage:**
- Same as Python script but with curl commands
- Output formatted with jq for readability

## Expected Results

### Successful Test Output

When tests pass, you should see:

```
Testing xAI API compatibility...
Base URL: https://api.x.ai/v1
API Key: xai-ABC123*********

1. Testing model listing...
   Status: 200
   Found X models
   - grok-code-fast-1
   - grok-2-1212
   - ...

2. Testing chat completion...
   Status: 200
   Response: Hello! The literal token you requested is: COMPAT_TEST_12345
   ✓ Test token found in response
   Response structure:
   - Has choices: True
   - Has usage: True
   - Has model: True

3. Testing streaming support...
   Status: 200
   Streaming response:
   - Chunk 1: '1'
   - Chunk 2: '\n2'
   - Chunk 3: '\n3'
   - [DONE]
   ✓ Received X chunks

4. Testing error handling...
   Testing 401 - Invalid API key...
   - Status: 401
   - Error structure: ['error']
   - Error message: Invalid API key

   Testing 400 - Invalid model...
   - Status: 400
   - Error structure: ['error']
   - Error message: Model not found

✓ xAI API compatibility test completed successfully!
```

### Common Issues

#### Authentication Errors
```
Error: XAI_API_KEY environment variable not set
```
**Solution:** Export your API key properly

#### Network Issues
```
Error: requests.exceptions.ConnectionError
```
**Solution:** Check network connectivity and firewall settings

#### Invalid Model
```
Status: 400
Error message: Model not found
```
**Solution:** Use a valid xAI model name (check model listing)

## Interpreting Results

### Compatibility Assessment

**✅ Fully Compatible**
- All tests pass with 200 status codes
- Streaming works correctly
- Error handling returns structured JSON
- Test token appears in response

**⚠️ Partially Compatible**
- Chat completion works but streaming fails
- Some error responses don't match OpenAI format
- Minor response structure differences

**❌ Incompatible**
- Authentication failures with valid key
- Malformed JSON responses
- Missing required response fields

### Response Structure Validation

The tests check for OpenAI-compatible response structure:

```json
{
  "choices": [
    {
      "message": {
        "role": "assistant",
        "content": "response text"
      },
      "finish_reason": "stop"
    }
  ],
  "usage": {
    "prompt_tokens": 10,
    "completion_tokens": 15,
    "total_tokens": 25
  },
  "model": "grok-code-fast-1"
}
```

### Streaming Format Validation

Streaming responses should follow Server-Sent Events format:

```
data: {"choices":[{"delta":{"content":"Hello"}}]}

data: {"choices":[{"delta":{"content":" world"}}]}

data: [DONE]
```

## Manual Testing with Codex

After API tests pass, test with actual Codex:

1. Configure xAI provider in `~/.codex/config.toml`
2. Run a simple Codex command:
   ```bash
   codex --provider api_x "Write a simple Hello World in Python"
   ```
3. Verify the response and streaming behavior

## Debugging Failed Tests

### Enable Verbose Output

For the Python script, you can modify it to show raw responses:

```python
# Add this to see raw responses
print(f"Raw response: {response.text}")
```

### Check API Documentation

If tests fail, verify against the latest xAI API documentation:
- [xAI API Docs](https://docs.x.ai/docs)
- [OpenAI API Compatibility](https://docs.x.ai/docs/openai-compatibility)

### Rate Limiting

If you see 429 errors, you may be hitting rate limits:
- Add delays between requests
- Check your account usage limits
- Consider using different API keys for testing

## Reporting Issues

When reporting compatibility issues, include:

1. Full test output
2. xAI API key format (first 10 characters only)
3. Model names you're testing
4. Network configuration details
5. Codex version information

## Advanced Testing

### Custom Model Testing

To test specific models, modify the test scripts:

```python
# Change model name in test scripts
"model": "your-custom-model-name"
```

### Load Testing

For production evaluation, consider:
- Multiple concurrent requests
- Extended streaming sessions
- Large context windows
- Function calling (if supported)

### Error Recovery Testing

Test how well the API handles:
- Network interruptions during streaming
- Invalid parameter combinations
- Extremely long prompts
- Special characters in messages