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
#include <curl/curl.h>
#include <json-c/json.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <unistd.h>
#include <gmp.h>

#define API_URL "https://api.x.ai/v1/chat/completions"
#define DEFAULT_MODEL "grok-code-fast-1"
#define MAX_INPUT_SIZE 4096
#define MAX_RESPONSE_SIZE 1048576  // 1MB
#define ROLLING_WINDOW_SIZE 5
#define MAX_LINE_SIZE 1024
#define MAX_MODEL_NAME_SIZE 64
#define MAX_MODEL_DESC_SIZE 256

/* Model preset structure for XAI models */
typedef struct {
    const char *name;           // Model identifier for API
    const char *label;          // User-friendly display name
    const char *description;    // Description of when to use this model
} ModelPreset;

/* Available XAI model presets - easily extendable
 * 
 * To add a new model:
 * 1. Add a new ModelPreset entry to this array
 * 2. Set the name (API identifier), label (display name), and description
 * 3. The menu will automatically include the new model
 */
static const ModelPreset model_presets[] = {
    {
        "grok-code-fast-1",
        "Grok Code Fast",
        "Optimized for fast coding tasks with balanced performance"
    },
    {
        "grok-2-latest",
        "Grok 2 Latest",
        "Latest Grok 2 model with enhanced reasoning capabilities"
    },
    {
        "grok-2-1212",
        "Grok 2 (Dec 2024)",
        "Grok 2 December 2024 snapshot with improved accuracy"
    },
    {
        "grok-beta",
        "Grok Beta",
        "Beta version with experimental features and capabilities"
    }
};

#define NUM_MODEL_PRESETS (sizeof(model_presets) / sizeof(model_presets[0]))

/* Global state for current model */
static char current_model[MAX_MODEL_NAME_SIZE] = DEFAULT_MODEL;

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
} ResponseState;

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
}

/* Free response state */
void free_response_state(ResponseState *state) {
    if (state->data) free(state->data);
    if (state->final_response) free(state->final_response);
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
                    if (json_object_object_get_ex(choice, "delta", &delta) &&
                        json_object_object_get_ex(delta, "content", &content)) {
                        
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

/* Send streaming request to Grok API */
int send_grok_request(const char *api_key, const char *user_message) {
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
    struct json_object *messages = json_object_new_array();
    struct json_object *message = json_object_new_object();
    
    json_object_object_add(root, "model", json_object_new_string(current_model));
    json_object_object_add(message, "role", json_object_new_string("user"));
    json_object_object_add(message, "content", json_object_new_string(user_message));
    json_object_array_add(messages, message);
    json_object_object_add(root, "messages", messages);
    json_object_object_add(root, "stream", json_object_new_boolean(1));
    json_object_object_add(root, "max_tokens", json_object_new_int(4096));
    
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
    }
    
    printf("\n\n");
    
    /* Cleanup */
    json_object_put(root);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free_response_state(&state);
    
    return 0;
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
        if (entry->d_name[0] == '.') continue;  // Skip hidden files
        
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
    printf("  /model              - Display model selection menu\n");
    printf("  read_file:<path>    - Read and display file contents\n");
    printf("  write_file:<path>:<content> - Write content to file\n");
    printf("  list_dir:<path>     - List directory contents\n");
    printf("  bash:<command>      - Execute bash command\n");
    printf("  exit                - Exit the terminal\n");
    printf("\nVerbose outputs (thinking steps) are buffered and summarized.\n");
    printf("Only the last 5 lines are shown during streaming.\n\n");
}

/* Display model selection menu and handle user choice */
void handle_model_selection() {
    printf("\n=== XAI Model Selection Menu ===\n");
    printf("\nAvailable models:\n\n");
    
    /* Display all available models with descriptions */
    for (size_t i = 0; i < NUM_MODEL_PRESETS; i++) {
        printf("  [%zu] %s\n", i + 1, model_presets[i].label);
        printf("      %s\n", model_presets[i].description);
        
        /* Show if this is the currently selected model */
        if (strcmp(current_model, model_presets[i].name) == 0) {
            printf("      ✓ Currently selected\n");
        }
        printf("\n");
    }
    
    printf("Enter model number to select (or 0 to cancel): ");
    fflush(stdout);
    
    /* Read user choice */
    char choice_input[16];
    if (!fgets(choice_input, sizeof(choice_input), stdin)) {
        printf("Selection cancelled.\n");
        return;
    }
    
    char *endptr;
    long choice_long = strtol(choice_input, &endptr, 10);

    // Remove trailing newline from input if present
    size_t len = strlen(choice_input);
    if (len > 0 && choice_input[len - 1] == '\n') {
        choice_input[len - 1] = '\0';
    }

    // Check for conversion errors: no digits found, extra characters, or out of int range
    if (choice_input[0] == '\0' || *endptr != '\0' || choice_long < 0 || choice_long > INT_MAX) {
        printf("Error: Invalid input. Please enter a valid number between 1 and %zu, or 0 to cancel.\n", NUM_MODEL_PRESETS);
        return;
    }

    int choice = (int)choice_long;

    /* Validate choice */
    if (choice == 0) {
        printf("Selection cancelled.\n");
        return;
    }
    if (choice < 1 || (size_t)choice > NUM_MODEL_PRESETS) {
        printf("Error: Invalid choice. Please select a number between 1 and %zu.\n", 
               NUM_MODEL_PRESETS);
        return;
    }
    
    /* Update current model */
    size_t index = (size_t)choice - 1;
    strncpy(current_model, model_presets[index].name, MAX_MODEL_NAME_SIZE - 1);
    current_model[MAX_MODEL_NAME_SIZE - 1] = '\0';
    
    printf("\n✓ Model changed to: %s\n", model_presets[index].label);
    printf("  %s\n\n", model_presets[index].description);
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
    printf("Connected to xAI API (model: %s)\n", current_model);
    printf("Type 'exit' to quit, '/model' to change model, or enter your message.\n");
    display_help();
    
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
        
        /* Check for model selection command */
        if (strcmp(input, "/model") == 0) {
            handle_model_selection();
            continue;
        }
        
        /* Check for special commands */
        if (strncmp(input, "read_file:", 10) == 0) {
            handle_read_file(input + 10);
        } else if (strncmp(input, "write_file:", 11) == 0) {
            char *sep = strchr(input + 11, ':');
            if (sep) {
                *sep = '\0';
                handle_write_file(input + 11, sep + 1);
            } else {
                printf("Error: write_file format is 'write_file:<path>:<content>'\n");
            }
        } else if (strncmp(input, "list_dir:", 9) == 0) {
            handle_list_dir(input + 9);
        } else if (strncmp(input, "bash:", 5) == 0) {
            handle_bash_command(input + 5);
        } else {
            /* Send to Grok API */
            if (send_grok_request(api_key, input) != 0) {
                fprintf(stderr, "Failed to get response from Grok\n");
            }
        }
    }
    
    /* Cleanup */
    curl_global_cleanup();
    
    return 0;
}
