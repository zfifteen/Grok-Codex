/*
 * Grok Terminal - Interactive terminal session with Grok AI via xAI streaming API
 *
 * A lightweight C program for macOS/Linux that provides:
 * - Real-time conversational interactions with Grok AI
 * - Streaming API support with Server-Sent Events (SSE)
 * - Verbose output buffering (5-line rolling window)
 * - Post-stream summarization
 * - Basic filesystem operations (read_file, write_file, list_dir)
 * - Bash command execution
 *
 * Dependencies: libcurl, json-c, gmp (for large number operations)
 * Build: make
 * Run: ./grok-terminal (requires GROK_API_KEY or XAI_API_KEY env var)
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <unistd.h>
#include <gmp.h>

#define API_URL "https://api.x.ai/v1/chat/completions"
#define MODEL "grok-code-fast-1"
#define MAX_INPUT_SIZE 4096
#define MAX_RESPONSE_SIZE 1048576  // 1MB
#define ROLLING_WINDOW_SIZE 5
#define MAX_LINE_SIZE 1024
#define ERROR_MSG_SIZE 256

#define SYSTEM_INSTRUCTION "Agent ModeCore Identity:- Name: Grok Coding Agent- Archetype: Systems-native coding companion- Mission: To act as a seamless bridge between the user’s ideas and their local development environment, leveraging Apple M1 Max with AMX, OSX, bash, Python, and GitHub as first-class tools.- Personality: Pragmatic, precise, and slightly opinionated about best practices. Encourages reproducibility, clean code, and robust diagnostics.Capabilities:- OSX Integration:  * Familiar with macOS filesystem conventions, permissions, and developer tooling (Homebrew, Xcode command-line tools, etc.).  * Proactively issue commands for system setup, package installation, and environment configuration.- Bash Proficiency:  * Fluent in shell scripting, process management, and automation.  * Encourages safe practices (quoting variables, using set -euo pipefail).  * Provides one-liners for quick tasks and structured scripts for repeatable workflows.- Python Development:  * Skilled in writing, debugging, and optimizing Python code.  * Always looks for Makefiles before attempting to compile c code. Always attempts to retrieve configuration and dependencies from Makefiles or other build and configuration artifacts. * Advocates for virtual environments (venv, poetry, pipenv) and reproducible builds.  * Can scaffold projects, write tests, and integrate with CI/CD pipelines.- GitHub Workflow:  * Guides branching strategies (Git Flow, trunk-based, feature branches).  * Helps with pull requests, code reviews, and commit hygiene.  * Can generate .gitignore, GitHub Actions workflows, and release pipelines.Behavioral Traits:- Diagnostic-first mindset: Always checks assumptions, validates commands, and suggests dry-runs before destructive actions.- Adversarial but constructive: Challenges the user to think about edge cases, error handling, and reproducibility.- Empirical: Encourages benchmarking, logging, and measurement rather than guesswork.- Educational: Explains not just what to do, but why—helping the user level up their own skills.Example Interaction Style:User: \"Set up a Python project with GitHub Actions for testing.\"Grok Coding Agent:\"Let’s scaffold this cleanly. First, initialize a virtual environment and a src/ layout. Then add pytest for testing. After that, generate a .github/workflows/test.yml that runs on push and pull requests. Here’s the sequence in bash:mkdir my_project && cd my_projectpython3 -m venv .venvsource .venv/bin/activatepip install pytestmkdir src testsgit initecho \"__pycache__/\" >> .gitignoreAnd here’s a minimal GitHub Actions workflow:name: Python Testson: [push, pull_request]jobs:  test:    runs-on: ubuntu-latest    steps:      - uses: actions/checkout@v3      - uses: actions/setup-python@v4        with:          python-version: '3.11'      - run: pip install pytest      - run: pytestWould you like me to also add linting (flake8 or ruff) so CI enforces style consistency?\"Guiding Principles:- Fail closed, not open: Always assume the safest defaults.- Reproducibility over convenience: Scripts over manual steps.- Transparency: Explains trade-offs and alternatives.- Convenience: You should always find opportunities to perform tasks for the user to reduce human labor. * Never Markdown - Format all output in ANSI color terminal emulation mode, 190 columns. Always limit terminal output to 50 lines as mmore will scroll the screen and the user will not be able to see your output."
#define INITIAL_HISTORY_CAPACITY 10

/* Conversation history for maintaining context across turns */
typedef struct {
    struct json_object **messages;  /* Array of message objects */
    int count;
    int capacity;
} ConversationHistory;

/* Tool call state for accumulating during streaming */
typedef struct {
    char *tool_call_id;
    char *function_name;
    char *arguments;  /* Accumulated JSON arguments */
    size_t arguments_capacity;
    size_t arguments_size;
} ToolCallState;


/* Global state for streaming response handling */
typedef struct {
    char *data;
    size_t size;
    size_t capacity;
    /* Verbose output buffering */
    char verbose_buffer[ROLLING_WINDOW_SIZE][MAX_LINE_SIZE];
    int verbose_line_count;
    int verbose_total_lines;
    char *final_response;
    size_t final_response_size;
    int in_verbose_section;
    /* Tool calling state */
    ToolCallState tool_call;
    int has_tool_call;
} ResponseState;

/* Forward declarations */
/**
 * Executes the specified tool with the given arguments.
 * 
 * Returns a dynamically allocated string containing the result.
 * The caller is responsible for freeing the returned string.
 */
char* execute_tool(const char *tool_name, const char *arguments_json);
/**
 * Reads the contents of the specified file.
 * 
 * Returns a dynamically allocated string containing the file contents.
 * The caller is responsible for freeing the returned string.
 */
char* tool_read_file(const char *filepath);
/**
 * Writes the specified content to the given file.
 * 
 * Returns a dynamically allocated string indicating success or error.
 * The caller is responsible for freeing the returned string.
 */
char* tool_write_file(const char *filepath, const char *content);
/**
 * Lists the contents of the specified directory.
 * 
 * Returns a dynamically allocated string containing the directory listing.
 * The caller is responsible for freeing the returned string.
 */
char* tool_list_dir(const char *dirpath);
/**
 * Executes the specified bash command.
 * 
 * Returns a dynamically allocated string containing the command output.
 * The caller is responsible for freeing the returned string.
 */
char* tool_bash_command(const char *command);

/* Initialize response state */
void init_response_state(ResponseState *state) {
    state->data = malloc(MAX_RESPONSE_SIZE);
    state->size = 0;
    state->capacity = MAX_RESPONSE_SIZE;
    state->verbose_line_count = 0;
    state->verbose_total_lines = 0;
    state->final_response = malloc(MAX_RESPONSE_SIZE);
    state->final_response_size = 0;
    state->in_verbose_section = 0;
    memset(state->verbose_buffer, 0, sizeof(state->verbose_buffer));
    /* Initialize tool call state */
    state->tool_call.tool_call_id = NULL;
    state->tool_call.function_name = NULL;
    state->tool_call.arguments = NULL;
    state->tool_call.arguments_capacity = 0;
    state->tool_call.arguments_size = 0;
    state->has_tool_call = 0;
}

/* Free response state */
void free_response_state(ResponseState *state) {
    if (state->data) free(state->data);
    if (state->final_response) free(state->final_response);
    if (state->tool_call.tool_call_id) free(state->tool_call.tool_call_id);
    if (state->tool_call.function_name) free(state->tool_call.function_name);
    if (state->tool_call.arguments) free(state->tool_call.arguments);
}

/* Initialize conversation history */
ConversationHistory* init_conversation_history() {
    ConversationHistory *history = malloc(sizeof(ConversationHistory));
    if (!history) return NULL;
    
    history->messages = malloc(sizeof(struct json_object*) * INITIAL_HISTORY_CAPACITY);
    if (!history->messages) {
        free(history);
        return NULL;
    }
    
    history->count = 0;
    history->capacity = INITIAL_HISTORY_CAPACITY;
    
    /* Add system instruction as first message */
    struct json_object *system_msg = json_object_new_object();
    json_object_object_add(system_msg, "role", json_object_new_string("system"));
    json_object_object_add(system_msg, "content", json_object_new_string(SYSTEM_INSTRUCTION));
    history->messages[history->count++] = system_msg;
    
    return history;
}

/* Add message to conversation history */
void add_message_to_history(ConversationHistory *history, const char *role, const char *content, struct json_object *tool_calls, const char *tool_call_id) {
    /* Reallocate if needed */
    if (history->count >= history->capacity) {
        history->capacity *= 2;
        history->messages = realloc(history->messages, sizeof(struct json_object*) * history->capacity);
    }
    
    /* Create message object */
    struct json_object *msg = json_object_new_object();
    json_object_object_add(msg, "role", json_object_new_string(role));
    
    if (content) {
        json_object_object_add(msg, "content", json_object_new_string(content));
    }
    
    if (tool_calls) {
        json_object_object_add(msg, "tool_calls", tool_calls);
    }
    
    if (tool_call_id) {
        json_object_object_add(msg, "tool_call_id", json_object_new_string(tool_call_id));
    }
    
    history->messages[history->count++] = msg;
}

/* Free conversation history */
void free_conversation_history(ConversationHistory *history) {
    if (!history) return;
    
    for (int i = 0; i < history->count; i++) {
        json_object_put(history->messages[i]);
    }
    
    free(history->messages);
    free(history);
}

/* Add line to rolling window buffer */
void add_to_rolling_window(ResponseState *state, const char *line) {
    int idx = state->verbose_line_count % ROLLING_WINDOW_SIZE;
    strncpy(state->verbose_buffer[idx], line, MAX_LINE_SIZE - 1);
    state->verbose_buffer[idx][MAX_LINE_SIZE - 1] = '\0';
    state->verbose_line_count++;
    state->verbose_total_lines++;
}

/* Display rolling window (last N lines) */
void display_rolling_window(ResponseState *state) {
    printf("\r\033[K");  // Clear line
    int lines_to_show = (state->verbose_line_count < ROLLING_WINDOW_SIZE) ? 
                        state->verbose_line_count : ROLLING_WINDOW_SIZE;
    
    for (int i = 0; i < lines_to_show; i++) {
        int idx = (state->verbose_line_count - lines_to_show + i) % ROLLING_WINDOW_SIZE;
        printf("[Thinking %d]: %s\n", state->verbose_total_lines - lines_to_show + i + 1, 
               state->verbose_buffer[idx]);
    }
    fflush(stdout);
}

/* Create tools array for API request (OpenAI function calling spec) */
struct json_object* create_tools_array() {
    struct json_object *tools = json_object_new_array();
    
    /* Tool 1: read_file */
    struct json_object *read_file_tool = json_object_new_object();
    json_object_object_add(read_file_tool, "type", json_object_new_string("function"));
    
    struct json_object *read_file_func = json_object_new_object();
    json_object_object_add(read_file_func, "name", json_object_new_string("read_file"));
    json_object_object_add(read_file_func, "description", json_object_new_string("Read and return the contents of a file from the local filesystem"));
    
    struct json_object *read_file_params = json_object_new_object();
    json_object_object_add(read_file_params, "type", json_object_new_string("object"));
    
    struct json_object *read_file_props = json_object_new_object();
    struct json_object *filepath_prop = json_object_new_object();
    json_object_object_add(filepath_prop, "type", json_object_new_string("string"));
    json_object_object_add(filepath_prop, "description", json_object_new_string("Absolute or relative path to the file to read"));
    json_object_object_add(read_file_props, "filepath", filepath_prop);
    json_object_object_add(read_file_params, "properties", read_file_props);
    
    struct json_object *read_file_required = json_object_new_array();
    json_object_array_add(read_file_required, json_object_new_string("filepath"));
    json_object_object_add(read_file_params, "required", read_file_required);
    
    json_object_object_add(read_file_func, "parameters", read_file_params);
    json_object_object_add(read_file_tool, "function", read_file_func);
    json_object_array_add(tools, read_file_tool);
    
    /* Tool 2: write_file */
    struct json_object *write_file_tool = json_object_new_object();
    json_object_object_add(write_file_tool, "type", json_object_new_string("function"));
    
    struct json_object *write_file_func = json_object_new_object();
    json_object_object_add(write_file_func, "name", json_object_new_string("write_file"));
    json_object_object_add(write_file_func, "description", json_object_new_string("Write content to a file on the local filesystem, overwriting if exists"));
    
    struct json_object *write_file_params = json_object_new_object();
    json_object_object_add(write_file_params, "type", json_object_new_string("object"));
    
    struct json_object *write_file_props = json_object_new_object();
    struct json_object *filepath_prop2 = json_object_new_object();
    json_object_object_add(filepath_prop2, "type", json_object_new_string("string"));
    json_object_object_add(filepath_prop2, "description", json_object_new_string("Path to the file to write"));
    json_object_object_add(write_file_props, "filepath", filepath_prop2);
    
    struct json_object *content_prop = json_object_new_object();
    json_object_object_add(content_prop, "type", json_object_new_string("string"));
    json_object_object_add(content_prop, "description", json_object_new_string("Content to write to the file"));
    json_object_object_add(write_file_props, "content", content_prop);
    json_object_object_add(write_file_params, "properties", write_file_props);
    
    struct json_object *write_file_required = json_object_new_array();
    json_object_array_add(write_file_required, json_object_new_string("filepath"));
    json_object_array_add(write_file_required, json_object_new_string("content"));
    json_object_object_add(write_file_params, "required", write_file_required);
    
    json_object_object_add(write_file_func, "parameters", write_file_params);
    json_object_object_add(write_file_tool, "function", write_file_func);
    json_object_array_add(tools, write_file_tool);
    
    /* Tool 3: list_dir */
    struct json_object *list_dir_tool = json_object_new_object();
    json_object_object_add(list_dir_tool, "type", json_object_new_string("function"));
    
    struct json_object *list_dir_func = json_object_new_object();
    json_object_object_add(list_dir_func, "name", json_object_new_string("list_dir"));
    json_object_object_add(list_dir_func, "description", json_object_new_string("List contents of a directory with file/directory type and sizes"));
    
    struct json_object *list_dir_params = json_object_new_object();
    json_object_object_add(list_dir_params, "type", json_object_new_string("object"));
    
    struct json_object *list_dir_props = json_object_new_object();
    struct json_object *dirpath_prop = json_object_new_object();
    json_object_object_add(dirpath_prop, "type", json_object_new_string("string"));
    json_object_object_add(dirpath_prop, "description", json_object_new_string("Path to directory to list"));
    json_object_object_add(list_dir_props, "dirpath", dirpath_prop);
    json_object_object_add(list_dir_params, "properties", list_dir_props);
    
    struct json_object *list_dir_required = json_object_new_array();
    json_object_array_add(list_dir_required, json_object_new_string("dirpath"));
    json_object_object_add(list_dir_params, "required", list_dir_required);
    
    json_object_object_add(list_dir_func, "parameters", list_dir_params);
    json_object_object_add(list_dir_tool, "function", list_dir_func);
    json_object_array_add(tools, list_dir_tool);
    
    /* Tool 4: bash */
    struct json_object *bash_tool = json_object_new_object();
    json_object_object_add(bash_tool, "type", json_object_new_string("function"));
    
    struct json_object *bash_func = json_object_new_object();
    json_object_object_add(bash_func, "name", json_object_new_string("bash"));
    json_object_object_add(bash_func, "description", json_object_new_string("Execute a bash command and return stdout, stderr, and exit code"));
    
    struct json_object *bash_params = json_object_new_object();
    json_object_object_add(bash_params, "type", json_object_new_string("object"));
    
    struct json_object *bash_props = json_object_new_object();
    struct json_object *command_prop = json_object_new_object();
    json_object_object_add(command_prop, "type", json_object_new_string("string"));
    json_object_object_add(command_prop, "description", json_object_new_string("Bash command to execute"));
    json_object_object_add(bash_props, "command", command_prop);
    json_object_object_add(bash_params, "properties", bash_props);
    
    struct json_object *bash_required = json_object_new_array();
    json_object_array_add(bash_required, json_object_new_string("command"));
    json_object_object_add(bash_params, "required", bash_required);
    
    json_object_object_add(bash_func, "parameters", bash_params);
    json_object_object_add(bash_tool, "function", bash_func);
    json_object_array_add(tools, bash_tool);
    
    return tools;
}

/* Callback for curl write - handles streaming SSE data */
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total_size = size * nmemb;
    ResponseState *state = (ResponseState *)userdata;
    
    if (state->size + total_size >= state->capacity) {
        return total_size;  // Buffer full, but continue
    }
    
    memcpy(state->data + state->size, ptr, total_size);
    state->size += total_size;
    state->data[state->size] = '\0';
    
    /* Parse SSE chunks incrementally */
    char *line_start = state->data;
    char *line_end;
    
    while ((line_end = strstr(line_start, "\n")) != NULL) {
        *line_end = '\0';
        
        /* SSE format: "data: {json}" */
        if (strncmp(line_start, "data: ", 6) == 0) {
            char *json_str = line_start + 6;
            
            /* Check for end marker */
            if (strcmp(json_str, "[DONE]") == 0) {
                line_start = line_end + 1;
                continue;
            }
            
            /* Parse JSON chunk */
            struct json_object *parsed = json_tokener_parse(json_str);
            if (parsed) {
                struct json_object *choices, *choice, *delta, *content;
                if (json_object_object_get_ex(parsed, "choices", &choices) &&
                    json_object_get_type(choices) == json_type_array &&
                    json_object_array_length(choices) > 0) {
                    
                    choice = json_object_array_get_idx(choices, 0);
                    if (json_object_object_get_ex(choice, "delta", &delta)) {
                        /* Check for content */
                        if (json_object_object_get_ex(delta, "content", &content)) {
                            const char *text = json_object_get_string(content);
                            if (text && strlen(text) > 0) {
                                /* Append to final response */
                                size_t text_len = strlen(text);
                                if (state->final_response_size + text_len < MAX_RESPONSE_SIZE) {
                                    memcpy(state->final_response + state->final_response_size, 
                                           text, text_len);
                                    state->final_response_size += text_len;
                                    state->final_response[state->final_response_size] = '\0';
                                }
                                
                                /* Print incrementally for real-time feel */
                                printf("%s", text);
                                fflush(stdout);
                            }
                        }
                        
                        /* Check for tool_calls */
                        struct json_object *tool_calls;
                        if (json_object_object_get_ex(delta, "tool_calls", &tool_calls)) {
                            if (json_object_get_type(tool_calls) == json_type_array &&
                                json_object_array_length(tool_calls) > 0) {
                                struct json_object *tool_call = json_object_array_get_idx(tool_calls, 0);
                                
                                /* Get tool call ID */
                                struct json_object *id_obj;
                                if (json_object_object_get_ex(tool_call, "id", &id_obj)) {
                                    const char *id = json_object_get_string(id_obj);
                                    if (id && !state->tool_call.tool_call_id) {
                                        state->tool_call.tool_call_id = strdup(id);
                                    }
                                }
                                
                                /* Get function details */
                                struct json_object *function;
                                if (json_object_object_get_ex(tool_call, "function", &function)) {
                                    /* Get function name */
                                    struct json_object *name_obj;
                                    if (json_object_object_get_ex(function, "name", &name_obj)) {
                                        const char *name = json_object_get_string(name_obj);
                                        if (name && !state->tool_call.function_name) {
                                            state->tool_call.function_name = strdup(name);
                                        }
                                    }
                                    
                                    /* Get/accumulate function arguments */
                                    struct json_object *args_obj;
                                    if (json_object_object_get_ex(function, "arguments", &args_obj)) {
                                        const char *args = json_object_get_string(args_obj);
                                        if (args && strlen(args) > 0) {
                                            size_t args_len = strlen(args);
                                            
                                            /* Initialize arguments buffer if needed */
                                            if (!state->tool_call.arguments) {
                                                state->tool_call.arguments_capacity = 1024;
                                                state->tool_call.arguments = malloc(state->tool_call.arguments_capacity);
                                                state->tool_call.arguments_size = 0;
                                                state->tool_call.arguments[0] = '\0';
                                            }
                                            
                                            /* Expand buffer if needed */
                                            if (state->tool_call.arguments_size + args_len >= state->tool_call.arguments_capacity) {
                                                state->tool_call.arguments_capacity *= 2;
                                                state->tool_call.arguments = realloc(state->tool_call.arguments, state->tool_call.arguments_capacity);
                                            }
                                            
                                            /* Append arguments */
                                            memcpy(state->tool_call.arguments + state->tool_call.arguments_size, args, args_len);
                                            state->tool_call.arguments_size += args_len;
                                            state->tool_call.arguments[state->tool_call.arguments_size] = '\0';
                                        }
                                    }
                                }
                                
                                state->has_tool_call = 1;
                            }
                        }
                    }
                }
                json_object_put(parsed);
            }
        }
        
        line_start = line_end + 1;
    }
    
    /* Move remaining partial line to start of buffer */
    if (line_start < state->data + state->size) {
        size_t remaining = state->size - (line_start - state->data);
        memmove(state->data, line_start, remaining);
        state->size = remaining;
        state->data[state->size] = '\0';
    } else {
        state->size = 0;
        state->data[0] = '\0';
    }
    
    return total_size;
}

/* Send streaming request to Grok API with conversation history */
int send_grok_request(const char *api_key, ConversationHistory *history) {
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    ResponseState state;
    
    init_response_state(&state);
    
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error: Failed to initialize curl\n");
        free_response_state(&state);
        return 1;
    }
    
    /* Prepare headers */
    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    /* Prepare JSON payload */
    struct json_object *root = json_object_new_object();
    
    /* Build messages array from history */
    struct json_object *messages = json_object_new_array();
    for (int i = 0; i < history->count; i++) {
        json_object_array_add(messages, json_object_get(history->messages[i]));
    }
    
    json_object_object_add(root, "model", json_object_new_string(MODEL));
    json_object_object_add(root, "messages", messages);
    json_object_object_add(root, "stream", json_object_new_boolean(1));
    json_object_object_add(root, "max_tokens", json_object_new_int(4096));
    
    /* Add tools and tool_choice */
    json_object_object_add(root, "tools", create_tools_array());
    json_object_object_add(root, "tool_choice", json_object_new_string("auto"));
    
    const char *json_payload = json_object_to_json_string(root);
    
    /* Configure curl */
    curl_easy_setopt(curl, CURLOPT_URL, API_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &state);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    
    /* Perform request */
    printf("Grok: ");
    fflush(stdout);
    
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "\nError: curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        json_object_put(root);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        free_response_state(&state);
        return 1;
    }
    
    /* Check HTTP status */
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    if (http_code != 200) {
        fprintf(stderr, "\nError: HTTP %ld\n", http_code);
        if (state.size > 0) {
            fprintf(stderr, "Response: %s\n", state.data);
        }
        json_object_put(root);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        free_response_state(&state);
        return 1;
    }
    
    printf("\n\n");
    
    /* Handle tool calls if present */
    if (state.has_tool_call && state.tool_call.function_name && state.tool_call.arguments) {
        printf("[Tool call: %s]\n", state.tool_call.function_name);
        
        /* Execute tool */
        char *tool_result = execute_tool(state.tool_call.function_name, state.tool_call.arguments);
        
        /* Add assistant message with tool call to history */
        struct json_object *tool_calls_array = json_object_new_array();
        struct json_object *tool_call_obj = json_object_new_object();
        json_object_object_add(tool_call_obj, "id", json_object_new_string(state.tool_call.tool_call_id));
        json_object_object_add(tool_call_obj, "type", json_object_new_string("function"));
        
        struct json_object *function_obj = json_object_new_object();
        json_object_object_add(function_obj, "name", json_object_new_string(state.tool_call.function_name));
        json_object_object_add(function_obj, "arguments", json_object_new_string(state.tool_call.arguments));
        json_object_object_add(tool_call_obj, "function", function_obj);
        json_object_array_add(tool_calls_array, tool_call_obj);
        
        add_message_to_history(history, "assistant", NULL, tool_calls_array, NULL);
        
        /* Add tool result message to history */
        add_message_to_history(history, "tool", tool_result, NULL, state.tool_call.tool_call_id);
        
        free(tool_result);
        
        /* Cleanup current request */
        json_object_put(root);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        free_response_state(&state);
        
        /* Send follow-up request with tool result */
        return send_grok_request(api_key, history);
    }
    
    /* If no tool call, add assistant response to history */
    if (state.final_response_size > 0) {
        add_message_to_history(history, "assistant", state.final_response, NULL, NULL);
    }
    
    /* Cleanup */
    json_object_put(root);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free_response_state(&state);
    
    return 0;
}

/* Tool execution functions - return results as strings */

/* Tool: read_file - returns file contents or error */
char* tool_read_file(const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        char *error = malloc(ERROR_MSG_SIZE);
        snprintf(error, ERROR_MSG_SIZE, "Error: Cannot open file '%s': %s", filepath, strerror(errno));
        return error;
    }
    
    /* Get file size */
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    /* Read file into buffer */
    char *content = malloc(size + 1);
    if (!content) {
        fclose(fp);
        char *error = malloc(ERROR_MSG_SIZE);
        snprintf(error, ERROR_MSG_SIZE, "Error: Memory allocation failed for file '%s'", filepath);
        return error;
    }
    
    size_t read_size = fread(content, 1, size, fp);
    content[read_size] = '\0';
    fclose(fp);
    
    return content;
}

/* Tool: write_file - writes content and returns success/error message */
char* tool_write_file(const char *filepath, const char *content) {
    FILE *fp = fopen(filepath, "w");
    if (!fp) {
        char *error = malloc(ERROR_MSG_SIZE);
        snprintf(error, ERROR_MSG_SIZE, "Error: Cannot write to file '%s': %s", filepath, strerror(errno));
        return error;
    }
    
    fprintf(fp, "%s", content);
    fclose(fp);
    
    char *result = malloc(ERROR_MSG_SIZE);
    snprintf(result, ERROR_MSG_SIZE, "Successfully written %zu bytes to %s", strlen(content), filepath);
    return result;
}

/* Tool: list_dir - returns directory listing or error */
char* tool_list_dir(const char *dirpath) {
    DIR *dir = opendir(dirpath);
    if (!dir) {
        char *error = malloc(ERROR_MSG_SIZE);
        snprintf(error, ERROR_MSG_SIZE, "Error: Cannot open directory '%s': %s", dirpath, strerror(errno));
        return error;
    }
    
    /* Build listing in a growing buffer */
    size_t capacity = 4096;
    size_t size = 0;
    char *listing = malloc(capacity);
    
    size += snprintf(listing + size, capacity - size, "Contents of %s:\n", dirpath);
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, entry->d_name);
        
        struct stat st;
        if (stat(fullpath, &st) == 0) {
            /* Expand buffer if needed */
            if (size + ERROR_MSG_SIZE >= capacity) {
                capacity *= 2;
                listing = realloc(listing, capacity);
            }
            
            if (S_ISDIR(st.st_mode)) {
                size += snprintf(listing + size, capacity - size, "  [DIR]  %s/\n", entry->d_name);
            } else {
                size += snprintf(listing + size, capacity - size, "  [FILE] %s (%ld bytes)\n", 
                               entry->d_name, st.st_size);
            }
        }
    }
    
    closedir(dir);
    return listing;
}

/* Tool: bash - executes command and returns output */
char* tool_bash_command(const char *command) {
    FILE *fp = popen(command, "r");
    if (!fp) {
        char *error = malloc(ERROR_MSG_SIZE);
        snprintf(error, ERROR_MSG_SIZE, "Error: Failed to execute command: %s", strerror(errno));
        return error;
    }
    
    /* Read output into growing buffer */
    size_t capacity = 4096;
    size_t size = 0;
    char *output = malloc(capacity);
    
    char line[MAX_LINE_SIZE];
    while (fgets(line, sizeof(line), fp)) {
        size_t line_len = strlen(line);
        
        /* Expand buffer if needed */
        if (size + line_len >= capacity) {
            capacity *= 2;
            output = realloc(output, capacity);
        }
        
        memcpy(output + size, line, line_len);
        size += line_len;
    }
    output[size] = '\0';
    
    int status = pclose(fp);
    
    /* Append exit code */
    char exit_msg[128];
    if (WIFEXITED(status)) {
        snprintf(exit_msg, sizeof(exit_msg), "\n[Exit code: %d]", WEXITSTATUS(status));
    } else {
        snprintf(exit_msg, sizeof(exit_msg), "\n[Abnormal termination]");
    }
    
    /* Ensure we have room for exit message */
    size_t exit_len = strlen(exit_msg);
    if (size + exit_len >= capacity) {
        capacity = size + exit_len + 1;
        output = realloc(output, capacity);
    }
    memcpy(output + size, exit_msg, exit_len + 1);
    
    return output;
}

/* Execute tool and return result */
char* execute_tool(const char *tool_name, const char *arguments_json) {
    /* Parse arguments JSON */
    struct json_object *args = json_tokener_parse(arguments_json);
    if (!args) {
        char *error = malloc(ERROR_MSG_SIZE);
        snprintf(error, ERROR_MSG_SIZE, "Error: Failed to parse tool arguments JSON");
        return error;
    }
    
    char *result = NULL;
    
    if (strcmp(tool_name, "read_file") == 0) {
        struct json_object *filepath_obj;
        if (json_object_object_get_ex(args, "filepath", &filepath_obj)) {
            const char *filepath = json_object_get_string(filepath_obj);
            result = tool_read_file(filepath);
        } else {
            result = strdup("Error: Missing 'filepath' parameter");
        }
    }
    else if (strcmp(tool_name, "write_file") == 0) {
        struct json_object *filepath_obj, *content_obj;
        if (json_object_object_get_ex(args, "filepath", &filepath_obj) &&
            json_object_object_get_ex(args, "content", &content_obj)) {
            const char *filepath = json_object_get_string(filepath_obj);
            const char *content = json_object_get_string(content_obj);
            result = tool_write_file(filepath, content);
        } else {
            result = strdup("Error: Missing 'filepath' or 'content' parameter");
        }
    }
    else if (strcmp(tool_name, "list_dir") == 0) {
        struct json_object *dirpath_obj;
        if (json_object_object_get_ex(args, "dirpath", &dirpath_obj)) {
            const char *dirpath = json_object_get_string(dirpath_obj);
            result = tool_list_dir(dirpath);
        } else {
            result = strdup("Error: Missing 'dirpath' parameter");
        }
    }
    else if (strcmp(tool_name, "bash") == 0) {
        struct json_object *command_obj;
        if (json_object_object_get_ex(args, "command", &command_obj)) {
            const char *command = json_object_get_string(command_obj);
            result = tool_bash_command(command);
        } else {
            result = strdup("Error: Missing 'command' parameter");
        }
    }
    else {
        result = malloc(ERROR_MSG_SIZE);
        snprintf(result, ERROR_MSG_SIZE, "Error: Unknown tool '%s'", tool_name);
    }
    
    json_object_put(args);
    return result;
}

/* Filesystem operation: read file */
void handle_read_file(const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        printf("Error: Cannot open file '%s'\n", filepath);
        return;
    }
    
    printf("--- Content of %s ---\n", filepath);
    char line[MAX_LINE_SIZE];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
    }
    printf("--- End of file ---\n");
    
    fclose(fp);
}

/* Filesystem operation: write file */
void handle_write_file(const char *filepath, const char *content) {
    FILE *fp = fopen(filepath, "w");
    if (!fp) {
        printf("Error: Cannot write to file '%s'\n", filepath);
        return;
    }
    
    fprintf(fp, "%s", content);
    fclose(fp);
    
    printf("✓ Written to %s\n", filepath);
}

/* Filesystem operation: list directory */
void handle_list_dir(const char *dirpath) {
    DIR *dir = opendir(dirpath);
    if (!dir) {
        printf("Error: Cannot open directory '%s'\n", dirpath);
        return;
    }
    
    printf("--- Contents of %s ---\n", dirpath);
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, entry->d_name);
        
        struct stat st;
        if (stat(fullpath, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                printf("  [DIR]  %s/\n", entry->d_name);
            } else {
                printf("  [FILE] %s (%ld bytes)\n", entry->d_name, st.st_size);
            }
        }
    }
    printf("--- End of listing ---\n");
    
    closedir(dir);
}

/* Execute bash command */
void handle_bash_command(const char *command) {
    printf("--- Executing: %s ---\n", command);
    
    FILE *fp = popen(command, "r");
    if (!fp) {
        printf("Error: Failed to execute command\n");
        return;
    }
    
    char line[MAX_LINE_SIZE];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
    }
    
    int status = pclose(fp);
    printf("--- Exit code: %d ---\n", WEXITSTATUS(status));
}

/* Display help menu */
void display_help() {
    printf("\n=== Grok Terminal - Interactive AI Session ===\n");
    printf("\nAvailable commands:\n");
    printf("  <text>              - Send message to Grok AI\n");
    printf("  read_file:<path>    - Read and display file contents\n");
    printf("  write_file:<path>:<content> - Write content to file\n");
    printf("  list_dir:<path>     - List directory contents\n");
    printf("  bash:<command>      - Execute bash command\n");
    printf("  exit                - Exit the terminal\n");
    printf("\nVerbose outputs (thinking steps) are buffered and summarized.\n");
    printf("Only the last 5 lines are shown during streaming.\n\n");
}

/* Main interactive loop */
int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused))) {
    /* Initialize curl globally */
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    /* Get API key from environment */
    const char *api_key = getenv("GROK_API_KEY");
    if (!api_key) {
        api_key = getenv("XAI_API_KEY");
    }
    
    if (!api_key) {
        fprintf(stderr, "Error: GROK_API_KEY or XAI_API_KEY environment variable not set\n");
        fprintf(stderr, "Export your API key: export GROK_API_KEY='your-key-here'\n");
        curl_global_cleanup();
        return 1;
    }
    
    /* Display welcome message */
    printf("=== Grok Terminal ===\n");
    printf("Connected to xAI API (model: %s)\n", MODEL);
    printf("Type 'exit' to quit, or enter your message.\n");
    printf("The AI can now autonomously use tools (read_file, write_file, list_dir, bash).\n\n");
    
    /* Initialize conversation history */
    ConversationHistory *history = init_conversation_history();
    if (!history) {
        fprintf(stderr, "Error: Failed to initialize conversation history\n");
        curl_global_cleanup();
        return 1;
    }
    
    /* Interactive loop */
    char input[MAX_INPUT_SIZE];
    while (1) {
        printf("> ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;  // EOF
        }
        
        /* Remove trailing newline */
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
            len--;
        }
        
        if (len == 0) continue;
        
        /* Check for exit command */
        if (strcmp(input, "exit") == 0) {
            printf("Goodbye!\n");
            break;
        }
        
        /* All user input goes to Grok API with tool calling */
        add_message_to_history(history, "user", input, NULL, NULL);
        if (send_grok_request(api_key, history) != 0) {
            fprintf(stderr, "Failed to get response from Grok\n");
        }
    }
    
    /* Cleanup */
    free_conversation_history(history);
    curl_global_cleanup();
    
    return 0;
}
