def run_stdio_command(request_json):
    """Run the MCP stdio server and send a JSON-RPC request."""
    classpath = (
        "/Users/velocityworks/Applications/IntelliJ IDEA Ultimate.app/Contents/plugins/mcpserver/lib/io.github.smiley4.schema.kenerator.core.jar:"
        "/Users/velocityworks/Applications/IntelliJ IDEA Ultimate.app/Contents/plugins/mcpserver/lib/io.github.smiley4.schema.kenerator.jsonschema.jar:"
        "/Users/velocityworks/Applications/IntelliJ IDEA Ultimate.app/Contents/plugins/mcpserver/lib/io.github.smiley4.schema.kenerator.serialization.jar:"
        "/Users/velocityworks/Applications/IntelliJ IDEA Ultimate.app/Contents/plugins/mcpserver/lib/io.ktor.utils.jar:"
        "/Users/velocityworks/Applications/IntelliJ IDEA Ultimate.app/Contents/plugins/mcpserver/lib/ktor-server-cio.jar:"
        "/Users/velocityworks/Applications/IntelliJ IDEA Ultimate.app/Contents/plugins/mcpserver/lib/mcpserver-frontend.jar:"
        "/Users/velocityworks/Applications/IntelliJ IDEA Ultimate.app/Contents/plugins/mcpserver/lib/mcpserver.jar:"
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