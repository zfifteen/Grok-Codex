# Grok Terminal - A command-line interface for xAI's Grok model
# Provides local tool execution capabilities for file operations, bash commands, and development workflows

import json
import os
import subprocess
import sys
import stat  # Currently unused - consider removing if not needed elsewhere
import datetime  # Currently unused - consider removing if not needed elsewhere
from typing import List, Dict, Any

from openai import OpenAI  # Using OpenAI client for xAI API compatibility

# xAI API configuration
API_BASE_URL = "https://api.x.ai/v1"
MODEL = "grok-code-fast-1"  # Fast coding-focused Grok model

SYSTEM_INSTRUCTION = """Agent ModeCore Identity:- Name: Grok Coding Agent- Archetype: Systems-native coding companion- Mission: To act as a seamless bridge between the user’s ideas and their local development environment, leveraging Apple M1 Max with AMX, OSX, bash, Python, and GitHub as first-class tools.- Personality: Pragmatic, precise, and slightly opinionated about best practices. Encourages reproducibility, clean code, and robust diagnostics.Capabilities:- OSX Integration:  * Familiar with macOS filesystem conventions, permissions, and developer tooling (Homebrew, Xcode command-line tools, etc.).  * Proactively issue commands for system setup, package installation, and environment configuration.- Bash Proficiency:  * Fluent in shell scripting, process management, and automation.  * Encourages safe practices (quoting variables, using set -euo pipefail).  * Provides one-liners for quick tasks and structured scripts for repeatable workflows.- Python Development:  * Skilled in writing, debugging, and optimizing Python code.  * Always looks for Makefiles before attempting to compile c code. Always attempts to retrieve configuration and dependencies from Makefiles or other build and configuration artifacts. * Advocates for virtual environments (venv, poetry, pipenv) and reproducible builds.  * Can scaffold projects, write tests, and integrate with CI/CD pipelines.- GitHub Workflow:  * Guides branching strategies (Git Flow, trunk-based, feature branches).  * Helps with pull requests, code reviews, and commit hygiene.  * Can generate .gitignore, GitHub Actions workflows, and release pipelines.Behavioral Traits:- Diagnostic-first mindset: Always checks assumptions, validates commands, and suggests dry-runs before destructive actions.- Adversarial but constructive: Challenges the user to think about edge cases, error handling, and reproducibility.- Empirical: Encourages benchmarking, logging, and measurement rather than guesswork.- Educational: Explains not just what to do, but why—helping the user level up their own skills.Example Interaction Style:User: \"Set up a Python project with GitHub Actions for testing.\"Grok Coding Agent:\"Let’s scaffold this cleanly. First, initialize a virtual environment and a src/ layout. Then add pytest for testing. After that, generate a .github/workflows/test.yml that runs on push and pull requests. Here’s the sequence in bash:mkdir my_project && cd my_projectpython3 -m venv .venvsource .venv/bin/activatepip install pytestmkdir src testsgit initecho \"__pycache__/\" >> .gitignoreAnd here’s a minimal GitHub Actions workflow:name: Python Testson: [push, pull_request]jobs:  test:    runs-on: ubuntu-latest    steps:      - uses: actions/checkout@v3      - uses: actions/setup-python@v4        with:          python-version: '3.11'      - run: pip install pytest      - run: pytestWould you like me to also add linting (flake8 or ruff) so CI enforces style consistency?\"Guiding Principles:- Fail closed, not open: Always assume the safest defaults.- Reproducibility over convenience: Scripts over manual steps.- Transparency: Explains trade-offs and alternatives.- Convenience: You should always find opportunities to perform tasks for the user to reduce human labor. * Never Markdown - Format all output in ANSI color terminal emulation mode, 190 columns. Always limit terminal output to 50 lines as mmore will scroll the screen and the user will not be able to see your output."""

# System instruction defines the AI agent's behavior, capabilities, and interaction style
# Configured for macOS development environment with emphasis on reproducible workflows

# ANSI color codes for terminal output formatting
ANSI_RESET = "\033[0m"    # Reset all formatting
ANSI_GREEN = "\033[32m"   # Success messages
ANSI_RED = "\033[31m"     # Error messages
ANSI_BLUE = "\033[34m"    # System messages
ANSI_YELLOW = "\033[33m"  # Warning messages
ANSI_BOLD = "\033[1m"     # Bold text
ANSI_DIM = "\033[2m"      # Dimmed text

def terminal_supports_colors() -> bool:
    """Check if terminal supports ANSI colors based on environment variables and TTY status.

    Returns:
        bool: True if colors should be displayed, False otherwise
    """
    # Respect NO_COLOR environment variable (https://no-color.org/)
    no_color = os.environ.get("NO_COLOR")
    if no_color and no_color != "":
        return False

    # Only use colors if output is going to a terminal (not redirected to file)
    if not sys.stdout.isatty():
        return False

    # Check TERM environment variable for color support indicators
    term = os.environ.get("TERM")
    if not term:
        return False

    # Common terminal types that support ANSI colors
    if "color" in term or "xterm" in term or "screen" in term or "tmux" in term or "rxvt" in term or "vt100" in term or term == "linux":
        return True
    return False

# Global flag for color support - determined once at startup
COLORS_ENABLED = terminal_supports_colors()

def get_color_code(color_type: str) -> str:
    """Get ANSI color code for specified color type.

    Args:
        color_type: Type of color (success, error, system, warning, reset, bold, dim)

    Returns:
        str: ANSI escape code or empty string if colors disabled
    """
    if not COLORS_ENABLED:
        return ""

    # Mapping of semantic color names to ANSI codes
    colors = {
        "success": ANSI_GREEN,
        "error": ANSI_RED,
        "system": ANSI_BLUE,
        "warning": ANSI_YELLOW,
        "reset": ANSI_RESET,
        "bold": ANSI_BOLD,
        "dim": ANSI_DIM,
    }
    return colors.get(color_type, "")

def print_with_color(text: str, color_type: str = "", end: str = "") -> None:
    """Print text with ANSI color codes if terminal supports colors."""
    print(f"{get_color_code(color_type)}{text}{get_color_code('reset')}", end=end)

def wrap_text(text: str, max_width: int = 190) -> str:
    """Wrap text to specified maximum width, preserving words when possible.

    Args:
        text: Text to wrap
        max_width: Maximum character width per line (default: 190 for wide terminals)

    Returns:
        str: Text wrapped to fit within specified width
    """
    lines = []
    for line in text.split("\n"):
        while len(line) > max_width:
            # Find last space within max_width to avoid breaking words
            break_pos = line[:max_width].rfind(" ")
            if break_pos == -1:
                # No space found - force break at max_width
                break_pos = max_width
            lines.append(line[:break_pos])
            line = line[break_pos:].lstrip()  # Remove leading whitespace from remainder
        lines.append(line)
    return "\n".join(lines)

def execute_bash_command(command: str) -> str:
    """Execute bash command and return combined output with exit code.

    Args:
        command: Shell command to execute

    Returns:
        str: Combined stdout/stderr output plus exit code
    """
    try:
        # Execute command with shell=True for full bash compatibility
        process = subprocess.Popen(
            command,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        stdout, stderr = process.communicate()
        exit_code = process.returncode

        # Combine stdout and stderr, including stderr only if present
        output = stdout.strip() + "\n" + stderr.strip() if stderr else stdout.strip()
        return f"{output}\n[Exit code: {exit_code}]"
    except Exception as e:
        return f"Error executing command: {str(e)}"

def tool_read_file(arguments: Dict[str, Any]) -> str:
    """Read file contents from the local filesystem.

    Args:
        arguments: Dict containing 'filepath' parameter

    Returns:
        str: File contents or error message
    """
    filepath = arguments.get("filepath")
    if not filepath:
        return "Error: Missing 'filepath' parameter"
    try:
        with open(filepath, "r") as f:
            return f.read()
    except Exception as e:
        return f"Error reading file '{filepath}': {str(e)}"

def tool_write_file(arguments: Dict[str, Any]) -> str:
    """Write content to a file, overwriting existing content.

    Args:
        arguments: Dict containing 'filepath' and 'content' parameters

    Returns:
        str: Success message or error description
    """
    filepath = arguments.get("filepath")
    content = arguments.get("content")
    if not filepath or content is None:
        return "Error: Missing 'filepath' or 'content' parameter"
    try:
        with open(filepath, "w") as f:
            f.write(content)
        return f"Successfully written to {filepath}"
    except Exception as e:
        return f"Error writing to file '{filepath}': {str(e)}"

def tool_list_dir(arguments: Dict[str, Any]) -> str:
    """List directory contents with file types and sizes.

    Args:
        arguments: Dict containing optional 'dirpath' parameter (defaults to current directory)

    Returns:
        str: Formatted directory listing or error message
    """
    dirpath = arguments.get("dirpath", ".")
    try:
        listing = f"Contents of {dirpath}:\n"
        for entry in os.scandir(dirpath):
            st = entry.stat()
            if entry.is_dir():
                listing += f"  [DIR]  {entry.name}/\n"
            else:
                listing += f"  [FILE] {entry.name} ({st.st_size} bytes)\n"
        return listing
    except Exception as e:
        return f"Error listing directory '{dirpath}': {str(e)}"

def tool_bash(arguments: Dict[str, Any]) -> str:
    """Execute arbitrary bash commands.

    Args:
        arguments: Dict containing 'command' parameter

    Returns:
        str: Command output and exit code
    """
    command = arguments.get("command")
    if not command:
        return "Error: Missing 'command' parameter"
    return execute_bash_command(command)

def tool_git(arguments: Dict[str, Any]) -> str:
    """Execute git version control commands.

    Args:
        arguments: Dict containing 'args' parameter with git command arguments

    Returns:
        str: Git command output and exit code
    """
    args = arguments.get("args")
    if not args:
        return "Error: Missing 'args' parameter"
    return execute_bash_command(f"git {args}")

def tool_brew(arguments: Dict[str, Any]) -> str:
    """Execute Homebrew package manager commands (macOS).

    Args:
        arguments: Dict containing 'args' parameter with brew command arguments

    Returns:
        str: Brew command output and exit code
    """
    args = arguments.get("args")
    if not args:
        return "Error: Missing 'args' parameter"
    return execute_bash_command(f"brew {args}")

def tool_python(arguments: Dict[str, Any]) -> str:
    """Execute Python scripts and modules using python3.

    Args:
        arguments: Dict containing 'args' parameter with python command arguments

    Returns:
        str: Python execution output and exit code
    """
    args = arguments.get("args")
    if not args:
        return "Error: Missing 'args' parameter"
    return execute_bash_command(f"python3 {args}")

def tool_pip(arguments: Dict[str, Any]) -> str:
    """Execute pip package management commands using pip3.

    Args:
        arguments: Dict containing 'args' parameter with pip command arguments

    Returns:
        str: Pip command output and exit code
    """
    args = arguments.get("args")
    if not args:
        return "Error: Missing 'args' parameter"
    return execute_bash_command(f"pip3 {args}")

# OpenAI-compatible tool definitions for the AI model
# These define the available functions the AI can call during conversations
TOOLS = [
    {
        "type": "function",
        "function": {
            "name": "read_file",
            "description": "Read and return the contents of a file from the local filesystem",
            "parameters": {
                "type": "object",
                "properties": {
                    "filepath": {"type": "string", "description": "Absolute or relative path to the file to read"},
                },
                "required": ["filepath"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "write_file",
            "description": "Write content to a file on the local filesystem, overwriting if exists",
            "parameters": {
                "type": "object",
                "properties": {
                    "filepath": {"type": "string", "description": "Path to the file to write"},
                    "content": {"type": "string", "description": "Content to write to the file"},
                },
                "required": ["filepath", "content"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "list_dir",
            "description": "List contents of a directory with file/directory type and sizes",
            "parameters": {
                "type": "object",
                "properties": {
                    "dirpath": {"type": "string", "description": "Path to directory to list"},
                },
                "required": ["dirpath"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "bash",
            "description": "Execute a bash command and return stdout, stderr, and exit code",
            "parameters": {
                "type": "object",
                "properties": {
                    "command": {"type": "string", "description": "Bash command to execute"},
                },
                "required": ["command"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "git",
            "description": "Execute git commands for version control operations",
            "parameters": {
                "type": "object",
                "properties": {
                    "args": {"type": "string", "description": "Git command arguments"},
                },
                "required": ["args"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "brew",
            "description": "Execute Homebrew commands for macOS package management",
            "parameters": {
                "type": "object",
                "properties": {
                    "args": {"type": "string", "description": "Brew command arguments"},
                },
                "required": ["args"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "python",
            "description": "Execute Python scripts or modules",
            "parameters": {
                "type": "object",
                "properties": {
                    "args": {"type": "string", "description": "Python command arguments"},
                },
                "required": ["args"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "pip",
            "description": "Execute pip commands for Python package management",
            "parameters": {
                "type": "object",
                "properties": {
                    "args": {"type": "string", "description": "Pip command arguments"},
                },
                "required": ["args"],
            },
        },
    },
]

# Mapping of tool names to their executor functions
# Used to dispatch tool calls to the appropriate Python functions
TOOL_EXECUTORS = {
    "read_file": tool_read_file,
    "write_file": tool_write_file,
    "list_dir": tool_list_dir,
    "bash": tool_bash,
    "git": tool_git,
    "brew": tool_brew,
    "python": tool_python,
    "pip": tool_pip,
}

def execute_tool(tool_call: Dict[str, Any]) -> str:
    """Execute a tool call by dispatching to the appropriate function.

    Args:
        tool_call: Tool call object from AI model containing function name and arguments

    Returns:
        str: Result of tool execution or error message
    """
    function_name = tool_call["function"]["name"]
    try:
        # Parse JSON arguments from the AI model
        arguments = json.loads(tool_call["function"]["arguments"])
    except json.JSONDecodeError:
        return "Error: Invalid arguments JSON"

    # Find and execute the appropriate tool function
    executor = TOOL_EXECUTORS.get(function_name)
    if not executor:
        return f"Error: Unknown tool '{function_name}'"
    return executor(arguments)

def main():
    """Main application entry point - initializes API client and runs interactive terminal."""
    # Check for API key in environment variables (supports both GROK_API_KEY and XAI_API_KEY)
    api_key = os.environ.get("GROK_API_KEY") or os.environ.get("XAI_API_KEY")
    if not api_key:
        print_with_color("Error: GROK_API_KEY or XAI_API_KEY environment variable not set", "error")
        print("Export your API key: export GROK_API_KEY='your-key-here'")
        sys.exit(1)

    # Initialize OpenAI client configured for xAI's API endpoint
    client = OpenAI(base_url=API_BASE_URL, api_key=api_key)

    # Display startup information
    print_with_color("=== Grok Terminal ===", "bold")
    print_with_color(f"Connected to xAI API (model: {MODEL})", "system")
    print("Type 'exit' to quit, or enter your message.")
    print("The AI can use tools: read_file, write_file, list_dir, bash, git, brew, python, pip.")
    print_with_color(f"Colors: {'Enabled' if COLORS_ENABLED else 'Disabled (set TERM or unset NO_COLOR)'}", "dim")
    print("")

    # Initialize conversation with system instruction
    messages: List[Dict[str, Any]] = [{"role": "system", "content": SYSTEM_INSTRUCTION}]

    # Main interaction loop
    while True:
        # Get user input
        sys.stdout.write("> ")
        sys.stdout.flush()
        user_input = sys.stdin.readline().strip()
        if not user_input:
            continue
        if user_input.lower() == "exit":
            print("Goodbye!")
            break

        # Add user message to conversation history
        messages.append({"role": "user", "content": user_input})

        # Handle AI response and potential tool calls (may require multiple rounds)
        while True:
            # Request completion from AI model with streaming enabled
            response = client.chat.completions.create(
                model=MODEL,
                messages=messages,
                tools=TOOLS,
                tool_choice="auto",  # Let AI decide when to use tools
                stream=True,
                max_tokens=4096,
            )

            # Display AI response header
            print_with_color("Grok: ", "system")
            sys.stdout.flush()

            # Initialize collectors for streaming response content and tool calls
            collected_content = ""
            tool_calls = []

            # Process streaming response chunks
            for chunk in response:
                # Handle content streaming (text response)
                if chunk.choices[0].delta.content:
                    content = chunk.choices[0].delta.content
                    wrapped = wrap_text(content)
                    print(wrapped, end="")
                    sys.stdout.flush()
                    collected_content += content
                if chunk.choices[0].delta.tool_calls:
                    # Accumulate tool calls (streaming may send deltas)
                    for tc_delta in chunk.choices[0].delta.tool_calls:
                        # Ensure tool_calls list is large enough for this index
                        if len(tool_calls) <= tc_delta.index:
                            tool_calls.extend([{"id": "", "type": "function", "function": {"name": "", "arguments": ""}} for _ in range(tc_delta.index - len(tool_calls) + 1)])

                        # Safely update tool call attributes if they exist
                        if tc_delta.id:
                            tool_calls[tc_delta.index]["id"] = tc_delta.id
                        if tc_delta.function and tc_delta.function.name:
                            tool_calls[tc_delta.index]["function"]["name"] += tc_delta.function.name
                        if tc_delta.function and tc_delta.function.arguments:
                            tool_calls[tc_delta.index]["function"]["arguments"] += tc_delta.function.arguments

            print("\n")  # End the AI response output

            # Add assistant message to conversation history
            assistant_message = {"role": "assistant"}
            if collected_content:
                assistant_message["content"] = collected_content
            if tool_calls:
                assistant_message["tool_calls"] = tool_calls
            messages.append(assistant_message)

            # If no tools were called, this conversation turn is complete
            if not tool_calls:
                break

            # Execute all requested tools and add results to conversation
            for tool_call in tool_calls:
                print_with_color(f"[Tool call: {tool_call['function']['name']}]", "system")
                tool_result = execute_tool(tool_call)
                # Add tool result as a message that AI can see in next iteration
                messages.append({
                    "role": "tool",
                    "content": tool_result,
                    "tool_call_id": tool_call["id"],
                })

# Entry point - run main() when script is executed directly
if __name__ == "__main__":
    main()