import json
import os
import subprocess
import sys

def run_stdio_command(request_json):
    """Run the MCP stdio server and send a JSON-RPC request."""
    classpath = (
        "/Users/velocityworks/Applications/IntelliJ IDEA Ultimate.app/Contents/plugins/mcpserver/lib/mcpserver-frontend.jar:"
        "/Users/velocityworks/Applications/IntelliJ IDEA Ultimate.app/Contents/lib/util-8.jar"
    )
    command = [
        "/Users/velocityworks/Applications/IntelliJ IDEA Ultimate.app/Contents/jbr/Contents/Home/bin/java",
        "-classpath",
        classpath,
        "com.intellij.mcpserver.stdio.McpStdioRunnerKt"
    ]
    env = os.environ.copy()
    env["IJ_MCP_SERVER_PORT"] = "64342"

    try:
        proc = subprocess.Popen(
            command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            env=env
        )
        # Send the request
        stdout, stderr = proc.communicate(input=request_json, timeout=10)

        if proc.returncode != 0:
            return {"error": f"Process failed with code {proc.returncode}: {stderr}"}

        try:
            return json.loads(stdout.strip())
        except json.JSONDecodeError:
            return {"error": f"Invalid JSON response: {stdout}"}

    except subprocess.TimeoutExpired:
        proc.kill()
        return {"error": "Process timed out"}
    except Exception as e:
        return {"error": f"Failed to run command: {str(e)}"}

def list_tools():
    """List all available MCP tools."""
    request = {
        "jsonrpc": "2.0",
        "id": 1,
        "method": "tools/list",
        "params": {}
    }
    return run_stdio_command(json.dumps(request))

def call_tool(name, arguments):
    """Call a specific MCP tool."""
    request = {
        "jsonrpc": "2.0",
        "id": 2,
        "method": "tools/call",
        "params": {
            "name": name,
            "arguments": arguments
        }
    }
    return run_stdio_command(json.dumps(request))

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Error: Missing action")
        sys.exit(1)

    action = sys.argv[1]

    if action == "list_tools":
        result = list_tools()
        print(json.dumps(result))
    elif action == "call_tool":
        if len(sys.argv) < 3:
            print("Error: Missing arguments for call_tool")
            sys.exit(1)
        args_json = sys.argv[2]
        try:
            args = json.loads(args_json)
            tool_name = args.get("name")
            tool_arguments = args.get("arguments", {})
            result = call_tool(tool_name, tool_arguments)
            print(json.dumps(result))
        except json.JSONDecodeError:
            print("Error: Invalid JSON arguments")
            sys.exit(1)
    else:
        print(f"Error: Unknown action '{action}'")
        sys.exit(1)