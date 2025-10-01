import subprocess
import json
import sys
import time
import os

# Start the MCP server
cmd = [
    'java',
    '-cp',
    '*:../../../lib/util-8.jar',
    'com.intellij.mcpserver.stdio.McpStdioRunnerKt'
]

env = {'IJ_MCP_SERVER_PORT': '64342'}
cwd = '/Users/velocityworks/Applications/IntelliJ IDEA Ultimate.app/Contents/plugins/mcpserver/lib'

proc = subprocess.Popen(
    cmd,
    cwd=cwd,
    env={**os.environ, **env},
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    text=True
)

def send_msg(msg_dict, id=None):
    if id is not None:
        msg_dict['id'] = id
        msg_dict['jsonrpc'] = '2.0'
    else:
        msg_dict['jsonrpc'] = '2.0'
    msg = json.dumps(msg_dict) + '\n'
    proc.stdin.write(msg)
    proc.stdin.flush()

def recv_msg():
    line = proc.stdout.readline().strip()
    if line:
        return json.loads(line)
    return None

# Send initialize
init_msg = {
    'method': 'initialize',
    'params': {
        'protocolVersion': '2024-11-05',
        'capabilities': {},
        'clientInfo': {'name': 'GrokDemo', 'version': '1.0'}
    }
}
send_msg(init_msg, 1)

# Receive initialize response
response = recv_msg()
print('Initialize response:', response)

# Send initialized
send_msg({'method': 'initialized', 'params': {}})

# Send tools/list
send_msg({'method': 'tools/list', 'params': {}}, 2)

# Receive tools/list response
response = recv_msg()
print('Tools list response:', response)

# Clean up
proc.terminate()