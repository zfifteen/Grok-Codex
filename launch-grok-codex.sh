#!/bin/bash

# Grok-Codex Launcher Script
# This script provides a seamless way to launch Grok-Codex after setting up your API key

set -e

# Colors for better user experience
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Script directory and paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CODEX_RS_DIR="$SCRIPT_DIR/codex-rs"
CODEX_BINARY="$CODEX_RS_DIR/target/debug/codex"
RELEASE_BINARY="$CODEX_RS_DIR/target/release/codex"

# Display help information
show_help() {
    echo -e "${CYAN}🚀 Grok-Codex Launcher${NC}"
    echo
    echo "A seamless launcher for Grok-Codex with automatic setup and API key management."
    echo
    echo -e "${YELLOW}Usage:${NC}"
    echo "  ./launch-grok-codex.sh [options] [codex arguments]"
    echo
    echo -e "${YELLOW}Options:${NC}"
    echo "  -h, --help          Show this help message"
    echo "  -b, --build         Force rebuild of the codex binary"
    echo "  -r, --release       Build and use release binary (optimized)"
    echo "  -s, --setup         Interactive API key setup"
    echo "  -c, --check         Check API key configuration"
    echo "  -l, --list-keys     List all supported API keys"
    echo
    echo -e "${YELLOW}Examples:${NC}"
    echo "  ./launch-grok-codex.sh                    # Launch with interactive setup"
    echo "  ./launch-grok-codex.sh --help             # Show codex help"
    echo "  ./launch-grok-codex.sh \"explain this code\"  # Start with a prompt"
    echo "  ./launch-grok-codex.sh --setup            # Configure API keys"
    echo "  ./launch-grok-codex.sh --full-auto        # Launch in full-auto mode"
    echo
    echo -e "${YELLOW}Supported AI Providers:${NC}"
    echo "  • OpenAI (OPENAI_API_KEY)"
    echo "  • xAI/Grok (XAI_API_KEY)"
    echo "  • Azure OpenAI (AZURE_OPENAI_API_KEY)"
    echo "  • OpenRouter (OPENROUTER_API_KEY)"
    echo "  • Gemini (GEMINI_API_KEY)"
    echo "  • Ollama (OLLAMA_API_KEY)"
    echo "  • Mistral (MISTRAL_API_KEY)"
    echo "  • DeepSeek (DEEPSEEK_API_KEY)"
    echo "  • Groq (GROQ_API_KEY)"
    echo "  • ArceeAI (ARCEEAI_API_KEY)"
    echo
}

# List all supported API keys
list_api_keys() {
    echo -e "${CYAN}📋 Supported API Keys${NC}"
    echo
    echo -e "${YELLOW}Environment Variables:${NC}"
    
    local keys=(
        "OPENAI_API_KEY:OpenAI:https://platform.openai.com/api-keys"
        "XAI_API_KEY:xAI (Grok):https://console.x.ai/api-keys"
        "AZURE_OPENAI_API_KEY:Azure OpenAI:https://portal.azure.com/"
        "OPENROUTER_API_KEY:OpenRouter:https://openrouter.ai/keys"
        "GEMINI_API_KEY:Google Gemini:https://makersuite.google.com/app/apikey"
        "OLLAMA_API_KEY:Ollama (local):http://localhost:11434"
        "MISTRAL_API_KEY:Mistral:https://console.mistral.ai/"
        "DEEPSEEK_API_KEY:DeepSeek:https://platform.deepseek.com/"
        "GROQ_API_KEY:Groq:https://console.groq.com/"
        "ARCEEAI_API_KEY:ArceeAI:https://www.arcee.ai/"
    )
    
    for key_info in "${keys[@]}"; do
        IFS=':' read -r env_var provider url <<< "$key_info"
        if [[ -n "${!env_var}" ]]; then
            echo -e "  ✅ ${GREEN}$env_var${NC} - $provider (${GREEN}SET${NC})"
        else
            echo -e "  ❌ ${RED}$env_var${NC} - $provider (${RED}NOT SET${NC})"
            echo -e "     💡 Get your key: $url"
        fi
    done
    echo
}

# Check if required tools are available
check_requirements() {
    local missing_tools=()
    
    # Check for required tools
    if ! command -v cargo &> /dev/null; then
        missing_tools+=("cargo (Rust toolchain)")
    fi
    
    if ! command -v just &> /dev/null; then
        missing_tools+=("just")
    fi
    
    if [[ ${#missing_tools[@]} -ne 0 ]]; then
        echo -e "${RED}❌ Missing required tools:${NC}"
        for tool in "${missing_tools[@]}"; do
            echo "   - $tool"
        done
        echo
        echo -e "${YELLOW}📦 Installation instructions:${NC}"
        echo "1. Install Rust: curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh"
        echo "2. Install just: cargo install just"
        echo
        return 1
    fi
    
    return 0
}

# Check if any API key is configured
check_api_keys() {
    local api_keys=(
        "OPENAI_API_KEY"
        "XAI_API_KEY"
        "AZURE_OPENAI_API_KEY"
        "OPENROUTER_API_KEY"
        "GEMINI_API_KEY"
        "OLLAMA_API_KEY"
        "MISTRAL_API_KEY"
        "DEEPSEEK_API_KEY"
        "GROQ_API_KEY"
        "ARCEEAI_API_KEY"
    )
    
    local found_keys=()
    for key in "${api_keys[@]}"; do
        if [[ -n "${!key}" ]]; then
            found_keys+=("$key")
        fi
    done
    
    if [[ ${#found_keys[@]} -eq 0 ]]; then
        return 1
    fi
    
    echo -e "${GREEN}✅ Found configured API keys:${NC}"
    for key in "${found_keys[@]}"; do
        # Show only first 8 characters for security
        local masked_key="${!key:0:8}..."
        echo "   - $key: $masked_key"
    done
    echo
    
    return 0
}

# Interactive API key setup
setup_api_keys() {
    echo -e "${CYAN}🔧 Interactive API Key Setup${NC}"
    echo
    echo "This will help you configure API keys for Grok-Codex."
    echo "You only need to set up ONE provider to get started."
    echo
    
    # Ask user which provider they want to use
    echo -e "${YELLOW}Which AI provider would you like to use?${NC}"
    echo "1) OpenAI (GPT models)"
    echo "2) xAI (Grok models) - Recommended for this project"
    echo "3) Other providers"
    echo "4) Skip setup (use existing environment)"
    echo
    
    read -p "Enter your choice (1-4): " provider_choice
    
    case $provider_choice in
        1)
            echo
            echo -e "${BLUE}📝 OpenAI Setup${NC}"
            echo "1. Go to https://platform.openai.com/api-keys"
            echo "2. Create a new API key"
            echo "3. Copy the key and paste it below"
            echo
            read -s -p "Enter your OpenAI API key: " api_key
            echo
            if [[ -n "$api_key" ]]; then
                export OPENAI_API_KEY="$api_key"
                echo -e "${GREEN}✅ OPENAI_API_KEY configured for this session${NC}"
            fi
            ;;
        2)
            echo
            echo -e "${BLUE}📝 xAI (Grok) Setup${NC}"
            echo "1. Go to https://console.x.ai/api-keys"
            echo "2. Create a new API key"
            echo "3. Copy the key and paste it below"
            echo
            read -s -p "Enter your xAI API key: " api_key
            echo
            if [[ -n "$api_key" ]]; then
                export XAI_API_KEY="$api_key"
                echo -e "${GREEN}✅ XAI_API_KEY configured for this session${NC}"
            fi
            ;;
        3)
            echo
            echo -e "${YELLOW}💡 For other providers, please set the appropriate environment variable:${NC}"
            list_api_keys
            echo "Example: export XAI_API_KEY=\"your-key-here\""
            echo
            ;;
        4)
            echo -e "${YELLOW}⚠️  Skipping setup. Make sure you have an API key configured.${NC}"
            ;;
        *)
            echo -e "${RED}❌ Invalid choice. Please run --setup again.${NC}"
            exit 1
            ;;
    esac
    
    echo
    echo -e "${CYAN}💡 To make your API key permanent, add it to your shell profile:${NC}"
    echo "   echo 'export XAI_API_KEY=\"your-key-here\"' >> ~/.bashrc"
    echo "   echo 'export XAI_API_KEY=\"your-key-here\"' >> ~/.zshrc"
    echo
}

# Build the codex binary
build_codex() {
    local build_type="$1"
    
    echo -e "${BLUE}🔨 Building Grok-Codex...${NC}"
    
    if [[ ! -d "$CODEX_RS_DIR" ]]; then
        echo -e "${RED}❌ codex-rs directory not found at: $CODEX_RS_DIR${NC}"
        exit 1
    fi
    
    cd "$CODEX_RS_DIR"
    
    if [[ "$build_type" == "release" ]]; then
        echo "Building release binary (this may take a while)..."
        cargo build --release --bin codex
    else
        echo "Building debug binary..."
        cargo build --bin codex
    fi
    
    echo -e "${GREEN}✅ Build completed successfully${NC}"
    cd "$SCRIPT_DIR"
}

# Check if codex binary exists and is up to date
check_binary() {
    local use_release="$1"
    local binary_path
    
    if [[ "$use_release" == "true" ]]; then
        binary_path="$RELEASE_BINARY"
    else
        binary_path="$CODEX_BINARY"
    fi
    
    if [[ ! -f "$binary_path" ]]; then
        echo -e "${YELLOW}⚠️  Codex binary not found. Building it for you...${NC}"
        if [[ "$use_release" == "true" ]]; then
            build_codex "release"
        else
            build_codex "debug"
        fi
    fi
    
    echo "$binary_path"
}

# Main execution function
main() {
    local force_build=false
    local use_release=false
    local show_setup=false
    local check_keys=false
    local list_keys=false
    local codex_args=()
    
    # Parse arguments - simple approach
    # If --help is the ONLY argument, show launcher help
    # Otherwise, if --help is mixed with other args, pass to codex
    if [[ $# -eq 1 && ("$1" == "--help" || "$1" == "-h") ]]; then
        show_help
        exit 0
    fi
    
    # Parse launcher-specific flags first, then pass rest to codex
    while [[ $# -gt 0 ]]; do
        case $1 in
            -b|--build)
                force_build=true
                shift
                ;;
            -r|--release)
                use_release=true
                shift
                ;;
            -s|--setup)
                show_setup=true
                shift
                ;;
            -c|--check)
                check_keys=true
                shift
                ;;
            -l|--list-keys)
                list_keys=true
                shift
                ;;
            *)
                codex_args+=("$1")
                shift
                ;;
        esac
    done
    
    # Handle special flags
    if [[ "$list_keys" == "true" ]]; then
        list_api_keys
        exit 0
    fi
    
    if [[ "$check_keys" == "true" ]]; then
        if check_api_keys; then
            echo -e "${GREEN}✅ API key configuration looks good!${NC}"
        else
            echo -e "${RED}❌ No API keys found. Run --setup to configure.${NC}"
            exit 1
        fi
        exit 0
    fi
    
    if [[ "$show_setup" == "true" ]]; then
        setup_api_keys
        exit 0
    fi
    
    # Show banner
    echo -e "${MAGENTA}"
    echo "  ╔═══════════════════════════════════════╗"
    echo "  ║           🚀 Grok-Codex              ║"
    echo "  ║     Seamless AI Coding Assistant     ║"
    echo "  ╚═══════════════════════════════════════╝"
    echo -e "${NC}"
    
    # Check requirements
    if ! check_requirements; then
        exit 1
    fi
    
    # Check for API keys
    if ! check_api_keys; then
        echo -e "${YELLOW}⚠️  No API keys found in environment variables.${NC}"
        echo
        echo "To get started, you need to set up an AI provider API key."
        echo
        echo -e "${CYAN}Quick setup options:${NC}"
        echo "1. Run: ./launch-grok-codex.sh --setup"
        echo "2. Or set manually: export XAI_API_KEY=\"your-key-here\""
        echo
        
        read -p "Would you like to run the interactive setup now? (y/n): " setup_now
        if [[ "$setup_now" =~ ^[Yy] ]]; then
            setup_api_keys
            echo
            if ! check_api_keys; then
                echo -e "${RED}❌ Setup incomplete. Please configure an API key to continue.${NC}"
                exit 1
            fi
        else
            echo -e "${RED}❌ Cannot proceed without an API key. Run --setup when ready.${NC}"
            exit 1
        fi
    fi
    
    # Build if forced or check binary
    if [[ "$force_build" == "true" ]]; then
        if [[ "$use_release" == "true" ]]; then
            build_codex "release"
        else
            build_codex "debug"
        fi
    fi
    
    # Get the binary path
    binary_path=$(check_binary "$use_release")
    
    # Launch codex
    echo -e "${GREEN}🎯 Launching Grok-Codex...${NC}"
    echo
    
    # Execute codex with all arguments
    exec "$binary_path" "${codex_args[@]}"
}

# Run main function with all arguments
main "$@"