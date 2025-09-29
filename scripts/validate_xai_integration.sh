#!/bin/bash
# Comprehensive validation script for xAI integration with Codex
# Tests both API compatibility and Codex configuration

set -e

echo "🚀 xAI Integration Validation Script"
echo "======================================"

# Function to find cargo executable
find_cargo() {
    # Check if cargo is in PATH
    if command -v cargo &> /dev/null; then
        echo "cargo"
        return
    fi
    
    # Check common installation locations
    if [ -f "$HOME/.cargo/bin/cargo" ]; then
        echo "$HOME/.cargo/bin/cargo"
        return
    fi
    
    # Check if rustup is available and can provide cargo
    if command -v rustup &> /dev/null; then
        local cargo_path=$(rustup which cargo 2>/dev/null || true)
        if [ -n "$cargo_path" ] && [ -f "$cargo_path" ]; then
            echo "$cargo_path"
            return
        fi
    fi
    
    echo ""
}

# Check prerequisites
echo "📋 Checking prerequisites..."

if [ -z "$XAI_API_KEY" ]; then
    echo "❌ XAI_API_KEY environment variable not set"
    echo "   Please export your xAI API key: export XAI_API_KEY='your-key-here'"
    exit 1
fi

if ! command -v python3 &> /dev/null; then
    echo "❌ Python 3 not found"
    exit 1
fi

if ! command -v curl &> /dev/null; then
    echo "❌ curl not found"
    exit 1
fi

if ! command -v jq &> /dev/null; then
    echo "⚠️  jq not found - JSON output will not be formatted"
fi

# Check for cargo availability
CARGO_CMD=$(find_cargo)
if [ -z "$CARGO_CMD" ]; then
    echo "❌ cargo not found"
    echo "   Please install Rust and cargo: https://rustup.rs/"
    exit 1
fi

echo "✅ Prerequisites check passed"
echo

# Test API compatibility
echo "🧪 Testing xAI API Compatibility..."
echo "-----------------------------------"

if python3 test_xai_api.py; then
    echo "✅ Python API tests passed"
else
    echo "❌ Python API tests failed"
    exit 1
fi

echo

# Test TOML configuration parsing
echo "🔧 Testing Configuration Parsing..."
echo "-----------------------------------"

cd ../codex-rs

# Run the specific xAI configuration test
if "$CARGO_CMD" test -p codex-core test_deserialize_xai_model_provider_toml --quiet; then
    echo "✅ xAI TOML configuration test passed"
else
    echo "❌ xAI TOML configuration test failed"
    exit 1
fi

cd ..

# Create a temporary config file for testing
echo "📝 Creating temporary configuration..."
TEMP_CONFIG=$(mktemp)
cat > "$TEMP_CONFIG" << 'EOF'
model = "grok-code-fast-1"
model_provider = "api_x"

[model_providers.api_x]
name = "xAI"
base_url = "https://api.x.ai/v1"
env_key = "XAI_API_KEY"
wire_api = "chat"
request_max_retries = 4
stream_max_retries = 10
EOF

echo "✅ Temporary configuration created at $TEMP_CONFIG"

# Validate TOML syntax
echo "🔍 Validating TOML syntax..."
if python3 -c "import toml; toml.load('$TEMP_CONFIG')" 2>/dev/null; then
    echo "✅ TOML syntax validation passed"
else
    echo "❌ TOML syntax validation failed"
    rm "$TEMP_CONFIG"
    exit 1
fi

# Show the configuration that would be used
echo "📄 Configuration preview:"
echo "========================"
cat "$TEMP_CONFIG"
echo "========================"

# Clean up
rm "$TEMP_CONFIG"

# Summary
echo
echo "🎉 Integration Validation Complete!"
echo "==================================="
echo
echo "✅ API compatibility confirmed"
echo "✅ Configuration parsing works"
echo "✅ Test scripts operational"
echo "✅ Documentation provided"
echo
echo "📚 Next steps:"
echo "1. Copy docs/examples/xai_config.toml to ~/.codex/config.toml"
echo "2. Run: codex --provider api_x 'Your prompt here'"
echo "3. Verify streaming and response quality"
echo
echo "📖 Documentation available:"
echo "- docs/xai_configuration.md - Complete setup guide"
echo "- docs/xai_testing_guide.md - Testing instructions"
echo "- docs/examples/xai_config.toml - Example configuration"
echo
echo "🔧 Test scripts available:"
echo "- scripts/test_xai_api.py - Python API tests"
echo "- scripts/test_xai_curl.sh - Curl API tests"
echo
echo "✨ xAI provider is ready for use with Codex!"