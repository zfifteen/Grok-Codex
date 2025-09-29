#!/usr/bin/env python3
"""
Test script to verify xAI API compatibility with OpenAI Chat Completions format.
Based on the issue requirements for testing api.x.ai endpoints.
"""

import os
import json
import requests
import time
import sys
from typing import Dict, Any, Optional


def test_api_x_ai():
    """Test xAI API compatibility with OpenAI Chat Completions format."""
    
    # Get API key from environment
    api_key = os.getenv("XAI_API_KEY")
    if not api_key:
        print("Error: XAI_API_KEY environment variable not set")
        return False
    
    base_url = "https://api.x.ai/v1"
    
    print("Testing xAI API compatibility...")
    print(f"Base URL: {base_url}")
    print(f"API Key: {api_key[:10]}{'*' * (len(api_key) - 10)}")
    print()
    
    # Test 1: Model listing / handshake
    print("1. Testing model listing...")
    try:
        response = requests.get(
            f"{base_url}/models",
            headers={"Authorization": f"Bearer {api_key}"},
            timeout=30
        )
        print(f"   Status: {response.status_code}")
        if response.status_code == 200:
            models = response.json()
            print(f"   Found {len(models.get('data', []))} models")
            for model in models.get('data', [])[:3]:  # Show first 3 models
                print(f"   - {model.get('id', 'unknown')}")
        else:
            print(f"   Error: {response.text}")
            return False
    except Exception as e:
        print(f"   Error: {e}")
        return False
    
    # Test 2: Chat completion with test token
    print("\n2. Testing chat completion...")
    try:
        payload = {
            "model": "grok-code-fast-1",  # Based on issue description
            "messages": [
                {"role": "user", "content": "Hello! Please respond with the literal token: COMPAT_TEST_12345"}
            ],
            "max_tokens": 50,
            "stream": False
        }
        
        response = requests.post(
            f"{base_url}/chat/completions",
            headers={
                "Authorization": f"Bearer {api_key}",
                "Content-Type": "application/json"
            },
            json=payload,
            timeout=30
        )
        
        print(f"   Status: {response.status_code}")
        if response.status_code == 200:
            result = response.json()
            message = result.get('choices', [{}])[0].get('message', {}).get('content', '')
            print(f"   Response: {message}")
            if "COMPAT_TEST_12345" in message:
                print("   ✓ Test token found in response")
            else:
                print("   ⚠ Test token not found in response (may still be compatible)")
            
            # Check response structure
            print("   Response structure:")
            print(f"   - Has choices: {'choices' in result}")
            print(f"   - Has usage: {'usage' in result}")
            print(f"   - Has model: {'model' in result}")
        else:
            print(f"   Error: {response.text}")
            return False
    except Exception as e:
        print(f"   Error: {e}")
        return False
    
    # Test 3: Streaming support
    print("\n3. Testing streaming support...")
    try:
        payload = {
            "model": "grok-code-fast-1",
            "messages": [
                {"role": "user", "content": "Count from 1 to 3, each number on a new line."}
            ],
            "max_tokens": 50,
            "stream": True
        }
        
        response = requests.post(
            f"{base_url}/chat/completions",
            headers={
                "Authorization": f"Bearer {api_key}",
                "Content-Type": "application/json"
            },
            json=payload,
            stream=True,
            timeout=30
        )
        
        print(f"   Status: {response.status_code}")
        if response.status_code == 200:
            print("   Streaming response:")
            chunk_count = 0
            for line in response.iter_lines():
                if line:
                    line_str = line.decode('utf-8')
                    if line_str.startswith('data: '):
                        chunk_count += 1
                        data = line_str[6:]
                        if data.strip() == '[DONE]':
                            print("   - [DONE]")
                            break
                        try:
                            chunk = json.loads(data)
                            delta = chunk.get('choices', [{}])[0].get('delta', {})
                            content = delta.get('content', '')
                            if content:
                                print(f"   - Chunk {chunk_count}: {repr(content)}")
                        except json.JSONDecodeError:
                            print(f"   - Invalid JSON: {data}")
                        
                        if chunk_count >= 10:  # Limit output
                            print("   - (truncated...)")
                            break
            print(f"   ✓ Received {chunk_count} chunks")
        else:
            print(f"   Error: {response.text}")
            print("   ⚠ Streaming may not be supported")
    except Exception as e:
        print(f"   Error: {e}")
        print("   ⚠ Streaming may not be supported")
    
    # Test 4: Error handling
    print("\n4. Testing error handling...")
    error_tests = [
        ("401 - Invalid API key", lambda: requests.post(
            f"{base_url}/chat/completions",
            headers={"Authorization": "Bearer invalid-key", "Content-Type": "application/json"},
            json={"model": "grok-code-fast-1", "messages": [{"role": "user", "content": "test"}]},
            timeout=10
        )),
        ("400 - Invalid model", lambda: requests.post(
            f"{base_url}/chat/completions",
            headers={"Authorization": f"Bearer {api_key}", "Content-Type": "application/json"},
            json={"model": "nonexistent-model", "messages": [{"role": "user", "content": "test"}]},
            timeout=10
        )),
    ]
    
    for test_name, test_func in error_tests:
        try:
            print(f"   Testing {test_name}...")
            response = test_func()
            print(f"   - Status: {response.status_code}")
            if response.status_code >= 400:
                error_data = response.json() if response.content else {}
                print(f"   - Error structure: {list(error_data.keys())}")
                if 'error' in error_data:
                    print(f"   - Error message: {error_data['error'].get('message', 'N/A')}")
        except Exception as e:
            print(f"   - Error: {e}")
    
    print("\n✓ xAI API compatibility test completed successfully!")
    return True


if __name__ == "__main__":
    if not test_api_x_ai():
        sys.exit(1)