# Quick Start with Grok-Codex Launcher

This repository includes a convenient bash launcher script that provides a seamless Grok-Codex experience after setting up your API key.

## 🚀 Getting Started

### 1. Quick Launch
```bash
# Clone the repository
git clone https://github.com/zfifteen/Grok-Codex.git
cd Grok-Codex

# Make sure the script is executable (should be by default)
chmod +x launch-grok-codex.sh

# Run the launcher - it will guide you through setup
./launch-grok-codex.sh
```

### 2. Set Up Your API Key
The launcher supports multiple AI providers. Choose the one you prefer:

**For xAI (Grok) - Recommended for this project:**
```bash
export XAI_API_KEY="your-xai-api-key-here"
./launch-grok-codex.sh
```

**For OpenAI:**
```bash
export OPENAI_API_KEY="your-openai-api-key-here"
./launch-grok-codex.sh
```

**Interactive Setup:**
```bash
./launch-grok-codex.sh --setup
```

### 3. Usage Examples

```bash
# Launch interactive mode
./launch-grok-codex.sh

# Launch with a specific prompt
./launch-grok-codex.sh "explain this codebase to me"

# Full auto mode (be careful!)
./launch-grok-codex.sh --full-auto "create a simple web app"

# Check your API key configuration
./launch-grok-codex.sh --check

# List all supported providers
./launch-grok-codex.sh --list-keys

# Force rebuild the binary
./launch-grok-codex.sh --build

# Build optimized release version
./launch-grok-codex.sh --release
```

## 🔑 Supported AI Providers

The launcher supports all major AI providers that are compatible with the OpenAI API:

- **OpenAI** (`OPENAI_API_KEY`)
- **xAI/Grok** (`XAI_API_KEY`) - Recommended
- **Azure OpenAI** (`AZURE_OPENAI_API_KEY`)
- **OpenRouter** (`OPENROUTER_API_KEY`)
- **Google Gemini** (`GEMINI_API_KEY`)
- **Ollama** (`OLLAMA_API_KEY`) - Local models
- **Mistral** (`MISTRAL_API_KEY`)
- **DeepSeek** (`DEEPSEEK_API_KEY`)
- **Groq** (`GROQ_API_KEY`)
- **ArceeAI** (`ARCEEAI_API_KEY`)

## 🛠️ Features

- **Automatic Building**: Builds the Rust binary automatically if it doesn't exist
- **API Key Management**: Detects and validates your API keys
- **Interactive Setup**: Guides you through API key configuration
- **Multiple Providers**: Supports all major AI providers
- **Error Handling**: Clear error messages and troubleshooting tips
- **Release Builds**: Option to build optimized release binaries

## 📋 Command Reference

| Command | Description |
|---------|-------------|
| `./launch-grok-codex.sh` | Launch with interactive setup if needed |
| `./launch-grok-codex.sh --help` | Show launcher help |
| `./launch-grok-codex.sh --setup` | Interactive API key setup |
| `./launch-grok-codex.sh --check` | Check API key configuration |
| `./launch-grok-codex.sh --list-keys` | List all supported providers |
| `./launch-grok-codex.sh --build` | Force rebuild the binary |
| `./launch-grok-codex.sh --release` | Build optimized version |

## 🔍 Troubleshooting

**Missing build tools:**
```bash
# Install Rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source ~/.cargo/env

# Install just
cargo install just
```

**No API key configured:**
- Run `./launch-grok-codex.sh --setup` for interactive setup
- Or manually: `export XAI_API_KEY="your-key-here"`

**Build fails:**
- Make sure you have Rust 1.90+ installed
- Run `./launch-grok-codex.sh --build` to force rebuild

## 💡 Pro Tips

1. **Permanent API Keys**: Add your API key to your shell profile or create a `.env` file:
   ```bash
   # Option 1: Shell profile
   echo 'export XAI_API_KEY="your-key-here"' >> ~/.bashrc
   source ~/.bashrc
   
   # Option 2: .env file in project root
   echo 'XAI_API_KEY=your-key-here' > .env
   ```

2. **Release Mode**: For better performance in production:
   ```bash
   ./launch-grok-codex.sh --release
   ```

3. **Check Status**: Verify everything is working:
   ```bash
   ./launch-grok-codex.sh --check
   ./launch-grok-codex.sh --version
   ```

## 🎯 What's Next?

After setup, you can use all the powerful features of Grok-Codex:
- Interactive coding sessions
- Automated file editing
- Code explanation and documentation
- Full-auto mode for complete autonomy

Start with simple prompts and gradually explore the full capabilities!