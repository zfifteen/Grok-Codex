# xAI Configuration Guide

This document describes how to configure Grok CLI to work with xAI's API endpoints at `https://api.x.ai`.

## Overview

xAI provides OpenAI-compatible endpoints that can be used with Grok CLI by adding a custom provider configuration. The xAI API uses the same Chat Completions format as OpenAI, making integration straightforward.

## Prerequisites

1. An xAI API key from [xAI Console](https://console.x.ai)
2. Grok CLI installed (`npm install -g @zfifteen/grok`)

## Configuration

### Step 1: Export Your API Key

Set your xAI API key as an environment variable:

```bash
export XAI_API_KEY="your-xai-api-key-here"
```

Add this to your shell profile (`.bashrc`, `.zshrc`, etc.) to make it persistent.

### Step 2: Configure the Provider

Add the following configuration to your `~/.codex/config.toml` file:

```toml
# Set the model and provider
model = "grok-code-fast-1"
model_provider = "api_x"

[model_providers.api_x]
name = "xAI"
base_url = "https://api.x.ai/v1"
env_key = "XAI_API_KEY"
wire_api = "chat"
request_max_retries = 4
stream_max_retries = 10
```

### Step 3: Run Grok CLI

Now you can run Grok CLI with the xAI provider:

```bash
grok --provider api_x
```

Or specify it directly in your config file and run:

```bash
grok
```

**Note:** If you're developing or running from source, the binary is called `codex` directly from the cargo workspace.

## Available Models

Common xAI models include:
- `grok-code-fast-1` - Fast coding model (recommended for Grok CLI)
- `grok-2-1212` - General purpose model
- `grok-2-vision-1212` - Vision-capable model

Check the [xAI documentation](https://docs.x.ai/docs) for the latest available models.

## Configuration Options

The xAI provider supports all standard Codex provider options:

| Option | Value | Description |
|--------|--------|-------------|
| `name` | `"xAI"` | Display name for the provider |
| `base_url` | `"https://api.x.ai/v1"` | xAI API base URL |
| `env_key` | `"XAI_API_KEY"` | Environment variable containing your API key |
| `wire_api` | `"chat"` | Use OpenAI Chat Completions format |
| `request_max_retries` | `4` | Number of HTTP request retries |
| `stream_max_retries` | `10` | Number of streaming reconnection attempts |
| `query_params` | `{}` | Additional query parameters (optional) |
| `http_headers` | `{}` | Additional HTTP headers (optional) |

## Testing Configuration

You can test your xAI configuration using the provided test scripts:

### Python Test Script
```bash
python3 scripts/test_xai_api.py
```

### Curl Test Script
```bash
bash scripts/test_xai_curl.sh
```

These scripts will verify:
1. Model listing endpoint
2. Chat completion functionality  
3. Streaming support
4. Error handling

## Troubleshooting

### Common Issues

1. **Authentication Error (401)**
   - Verify your `XAI_API_KEY` is set correctly
   - Check that your API key is valid and active

2. **Model Not Found (400)**
   - Ensure you're using a valid xAI model name
   - Check the xAI documentation for available models

3. **Connection Issues**
   - Verify network connectivity to `api.x.ai`
   - Check if you're behind a firewall or proxy

### Configuration Validation

Verify your configuration is loaded correctly:

```bash
codex --help
```

The xAI provider should appear in the available providers list.

## Compatibility Notes

xAI's API is compatible with the OpenAI Chat Completions format, which means:

✅ **Supported Features:**
- Chat completions
- Streaming responses
- Function calling (if supported by the model)
- Token usage reporting
- Error handling

⚠️ **Limitations:**
- xAI may not support all OpenAI-specific features
- Model capabilities vary by xAI model
- Rate limits may differ from OpenAI

## Advanced Configuration

### Custom Headers

If you need to add custom headers:

```toml
[model_providers.api_x]
name = "xAI"
base_url = "https://api.x.ai/v1"
env_key = "XAI_API_KEY"
wire_api = "chat"
http_headers = { "X-Custom-Header" = "value" }
```

### Environment Variable Headers

To use environment variables for headers:

```toml
[model_providers.api_x]
name = "xAI"  
base_url = "https://api.x.ai/v1"
env_key = "XAI_API_KEY"
wire_api = "chat"
env_http_headers = { "X-Custom-Header" = "CUSTOM_HEADER_ENV_VAR" }
```

### Network Tuning

For unreliable networks:

```toml
[model_providers.api_x]
name = "xAI"
base_url = "https://api.x.ai/v1"
env_key = "XAI_API_KEY"
wire_api = "chat" 
request_max_retries = 6
stream_max_retries = 15
stream_idle_timeout_ms = 180000  # 3 minutes
```

## Integration with Profiles

You can create profiles for different xAI models:

```toml
[profiles.grok-fast]
model = "grok-code-fast-1"
model_provider = "api_x"

[profiles.grok-vision]
model = "grok-2-vision-1212"
model_provider = "api_x"
```

Then use them with:

```bash
grok --profile grok-fast
grok --profile grok-vision
```

## Support

For xAI-specific issues:
- Check [xAI Documentation](https://docs.x.ai/docs)
- Contact xAI Support

For Grok CLI configuration issues:
- Check the main [Grok CLI Documentation](../README.md)
- Review the [Configuration Guide](config.md)