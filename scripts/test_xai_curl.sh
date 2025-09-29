#!/bin/bash
# Test script for xAI API compatibility using curl
# Based on the issue requirements for testing api.x.ai endpoints

if [ -z "$XAI_API_KEY" ]; then
    echo "Error: XAI_API_KEY environment variable not set"
    echo "Export your xAI API key: export XAI_API_KEY='your-api-key-here'"
    exit 1
fi

BASE_URL="https://api.x.ai/v1"

echo "Testing xAI API compatibility with curl..."
echo "Base URL: $BASE_URL"
echo "API Key: ${XAI_API_KEY:0:10}$(printf '%*s' $((${#XAI_API_KEY} - 10)) | tr ' ' '*')"
echo

echo "1. Testing model listing..."
curl -s -X GET "$BASE_URL/models" \
  -H "Authorization: Bearer $XAI_API_KEY" | jq .

echo
echo "2. Testing chat completion with test token..."
curl -s -X POST "$BASE_URL/chat/completions" \
  -H "Authorization: Bearer $XAI_API_KEY" \
  -H "Content-Type: application/json" \
  -d '{
    "model": "grok-code-fast-1",
    "messages": [
      {"role": "user", "content": "Hello! Please respond with the literal token: COMPAT_TEST_12345"}
    ],
    "max_tokens": 50,
    "stream": false
  }' | jq .

echo
echo "3. Testing streaming response..."
curl -s -X POST "$BASE_URL/chat/completions" \
  -H "Authorization: Bearer $XAI_API_KEY" \
  -H "Content-Type: application/json" \
  -d '{
    "model": "grok-code-fast-1",
    "messages": [
      {"role": "user", "content": "Count from 1 to 3, each number on a new line."}
    ],
    "max_tokens": 50,
    "stream": true
  }' | head -20

echo
echo "4. Testing error handling - invalid model..."
curl -s -X POST "$BASE_URL/chat/completions" \
  -H "Authorization: Bearer $XAI_API_KEY" \
  -H "Content-Type: application/json" \
  -d '{
    "model": "nonexistent-model",
    "messages": [
      {"role": "user", "content": "test"}
    ]
  }' | jq .

echo
echo "5. Testing error handling - invalid API key..."
curl -s -X POST "$BASE_URL/chat/completions" \
  -H "Authorization: Bearer invalid-key" \
  -H "Content-Type: application/json" \
  -d '{
    "model": "grok-code-fast-1",
    "messages": [
      {"role": "user", "content": "test"}
    ]
  }' | jq .

echo
echo "✓ xAI API compatibility test completed!"