#!/bin/bash
# Demo script for Grok Terminal
# This script demonstrates the functionality of the grok-terminal program

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Grok Terminal Demo ==="
echo ""

# Check if API key is set
if [ -z "$GROK_API_KEY" ] && [ -z "$XAI_API_KEY" ]; then
    echo "Error: GROK_API_KEY or XAI_API_KEY environment variable not set"
    echo ""
    echo "To run this demo, export your xAI API key:"
    echo "  export GROK_API_KEY='your-xai-api-key-here'"
    echo ""
    echo "Or:"
    echo "  export XAI_API_KEY='your-xai-api-key-here'"
    echo ""
    exit 1
fi

# Check if executable exists
if [ ! -f "../bin/grok-terminal" ]; then
    echo "Building grok-terminal..."
    cd ..
    make
    cd scripts
    echo ""
fi

# Create a temporary directory for demo
DEMO_DIR=$(mktemp -d)
trap "rm -rf $DEMO_DIR" EXIT

echo "Demo directory: $DEMO_DIR"
echo ""

# Create some test files
echo "Hello from Grok Terminal demo!" > "$DEMO_DIR/test.txt"
echo "Line 1" > "$DEMO_DIR/sample.txt"
echo "Line 2" >> "$DEMO_DIR/sample.txt"
echo "Line 3" >> "$DEMO_DIR/sample.txt"

mkdir -p "$DEMO_DIR/subdir"
echo "File in subdirectory" > "$DEMO_DIR/subdir/nested.txt"

echo "=== Interactive Demo ==="
echo ""
echo "The grok-terminal program supports:"
echo "  1. Conversational AI with Grok (streaming responses)"
echo "  2. File operations (read_file, write_file, list_dir)"
echo "  3. Bash command execution"
echo ""
echo "Example commands you can try:"
echo "  - Hello, Grok! Can you tell me about yourself?"
echo "  - read_file:$DEMO_DIR/test.txt"
echo "  - list_dir:$DEMO_DIR"
echo "  - bash:echo 'Hello from bash!'"
echo "  - write_file:$DEMO_DIR/output.txt:This is new content"
echo "  - exit"
echo ""

# Run in non-interactive mode for demo
echo "=== Automated Demo (non-interactive) ==="
echo ""

echo "1. Testing file read operation:"
../bin/grok-terminal <<EOF
read_file:$DEMO_DIR/test.txt
exit
EOF

echo ""
echo "2. Testing directory listing:"
../bin/grok-terminal <<EOF
list_dir:$DEMO_DIR
exit
EOF

echo ""
echo "3. Testing bash command execution:"
../bin/grok-terminal <<EOF
bash:echo 'Current date:'; date
exit
EOF

echo ""
echo "4. Testing file write operation:"
../bin/grok-terminal <<EOF
write_file:$DEMO_DIR/new_file.txt:Content created by Grok Terminal
exit
EOF

echo ""
echo "5. Verifying written file:"
if [ -f "$DEMO_DIR/new_file.txt" ]; then
    echo "✓ File created successfully:"
    cat "$DEMO_DIR/new_file.txt"
else
    echo "✗ File was not created"
fi

echo ""
echo ""
echo "=== Demo Complete ==="
echo ""
echo "To use Grok Terminal interactively, run:"
echo "  cd $SCRIPT_DIR/.."
echo "  ./bin/grok-terminal"
echo ""
echo "Or with the Grok API (requires valid API key):"
echo "  export GROK_API_KEY='your-key-here'"
echo "  ./bin/grok-terminal"
echo ""
echo "Note: The Grok API streaming functionality requires a valid API key."
echo "      The demo above only tests the local filesystem and bash commands."
echo ""
