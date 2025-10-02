#!/usr/bin/env python3
"""
IntelliJ IDEA MCP Client Tool

This script provides a command-line interface to interact with IntelliJ IDEA's MCP server
for dynamic IDE operations like file reading, code search, and refactoring.

Usage:
    python3 intellij_tool.py <action> <arguments_json>

Actions:
    list_tools: List available MCP tools
    call_tool: Call a specific MCP tool with arguments

Arguments:
    For list_tools: Empty JSON '{}'
    For call_tool: JSON with 'name' and 'arguments' keys
"""

import asyncio
import json
import os
import sys
from mcp import StdioServerParameters
from mcp.client.stdio import stdio_client


async def list_tools():
    """List available tools from the MCP server."""
    try:
        # MCP server configuration for IntelliJ IDEA
        server_params = StdioServerParameters(
            command="/Users/velocityworks/Applications/IntelliJ IDEA Ultimate.app/Contents/jbr/Contents/Home/bin/java",
            args=[
                "-classpath",
                "/Users/velocityworks/Applications/IntelliJ IDEA Ultimate.app/Contents/plugins/mcpserver/lib/mcpserver-frontend.jar:/Users/velocityworks/Applications/IntelliJ IDEA Ultimate.app/Contents/lib/util-8.jar",
                "com.intellij.mcpserver.stdio.McpStdioRunnerKt"
            ],
            env={
                "IJ_MCP_SERVER_PORT": "64342",
                **os.environ
            }
        )

        async with stdio_client(server_params) as (read, write):
            async with read as client:
                result = await client.list_tools()
                return {"tools": [tool.model_dump() for tool in result.tools]}
    except Exception as e:
        return {"error": f"Failed to list tools: {str(e)}"}


async def call_tool(name: str, arguments: dict):
    """Call a specific MCP tool with arguments."""
    try:
        # MCP server configuration for IntelliJ IDEA
        server_params = StdioServerParameters(
            command="/Users/velocityworks/Applications/IntelliJ IDEA Ultimate.app/Contents/jbr/Contents/Home/bin/java",
            args=[
                "-classpath",
                "/Users/velocityworks/Applications/IntelliJ IDEA Ultimate.app/Contents/plugins/mcpserver/lib/mcpserver-frontend.jar:/Users/velocityworks/Applications/IntelliJ IDEA Ultimate.app/Contents/lib/util-8.jar",
                "com.intellij.mcpserver.stdio.McpStdioRunnerKt"
            ],
            env={
                "IJ_MCP_SERVER_PORT": "64342",
                **os.environ
            }
        )

        async with stdio_client(server_params) as (read, write):
            async with read as client:
                result = await client.call_tool(name, arguments)
                return {"result": result.model_dump() if hasattr(result, 'model_dump') else str(result)}
    except Exception as e:
        return {"error": f"Failed to call tool '{name}': {str(e)}"}


async def main():
    """Main entry point for the script."""
    if len(sys.argv) < 3:
        print(json.dumps({"error": "Usage: python3 intellij_tool.py <action> <arguments_json>"}))
        sys.exit(1)

    action = sys.argv[1]
    args_json = sys.argv[2]

    try:
        args = json.loads(args_json)
    except json.JSONDecodeError as e:
        print(json.dumps({"error": f"Invalid JSON arguments: {str(e)}"}))
        sys.exit(1)

    if action == "list_tools":
        result = await list_tools()
    elif action == "call_tool":
        name = args.get("name")
        tool_args = args.get("arguments", {})
        if not name:
            result = {"error": "Missing 'name' for call_tool"}
        else:
            result = await call_tool(name, tool_args)
    else:
        result = {"error": f"Unknown action: {action}"}

    print(json.dumps(result))


if __name__ == "__main__":
    asyncio.run(main())