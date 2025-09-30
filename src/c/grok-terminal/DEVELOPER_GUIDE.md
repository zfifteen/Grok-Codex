# Developer Guide: Extending the Grok Terminal Model Menu System

## Overview

The Grok Terminal includes a modular, easily extendable model menu system that allows users to switch between different XAI models during their session. This guide explains how to add, modify, or customize the model menu.

## Architecture

### Model Preset Structure

Models are defined using the `ModelPreset` structure in `grok_terminal.c`:

```c
typedef struct {
    const char *name;           // Model identifier for API calls
    const char *label;          // User-friendly display name  
    const char *description;    // Description of when to use this model
} ModelPreset;
```

### Model Presets Array

All available models are stored in a static array:

```c
static const ModelPreset model_presets[] = {
    {
        "grok-code-fast-1",
        "Grok Code Fast",
        "Optimized for fast coding tasks with balanced performance"
    },
    // ... more models ...
};
```

### Current Model State

The currently selected model is stored in a global variable:

```c
static char current_model[MAX_MODEL_NAME_SIZE] = DEFAULT_MODEL;
```

This variable is used in API calls to determine which model to use.

## Adding a New Model

### Step 1: Locate the Model Presets Array

Open `grok_terminal.c` and find the `model_presets` array (approximately line 54).

### Step 2: Add Your Model Entry

Add a new entry to the array with the following format:

```c
{
    "api-model-identifier",     // Exact model name as expected by xAI API
    "User-Friendly Name",       // Name shown in the menu
    "Model description text"    // Brief description of use cases
}
```

**Important Notes**:
- The model name must match exactly what the xAI API expects
- Add a comma after the previous entry
- Keep descriptions concise (under 100 characters recommended)
- Models appear in the menu in the order they're listed

### Step 3: Save and Rebuild

```bash
make clean
make
```

### Example: Adding Grok 3 Preview

```c
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
    },
    {
        "grok-3-preview",           // NEW MODEL
        "Grok 3 Preview",           // NEW MODEL
        "Early preview with advanced multimodal capabilities"  // NEW MODEL
    }
};
```

## Modifying Existing Models

### Changing Model Descriptions

Simply edit the description field in the `model_presets` array:

```c
{
    "grok-beta",
    "Grok Beta",
    "Updated description with more detail"  // Modified
}
```

### Changing Display Names

Edit the label field while keeping the API name unchanged:

```c
{
    "grok-beta",
    "Grok Beta (Experimental)",  // Updated display name
    "Beta version with experimental features and capabilities"
}
```

### Reordering Models

Change the order of entries in the array. The menu displays models in array order:

```c
static const ModelPreset model_presets[] = {
    // Most commonly used model first
    {
        "grok-2-latest",
        "Grok 2 Latest",
        "Latest Grok 2 model with enhanced reasoning capabilities"
    },
    // Less common models below
    {
        "grok-code-fast-1",
        "Grok Code Fast",
        "Optimized for fast coding tasks with balanced performance"
    },
    // ...
};
```

## Removing Models

To remove a model from the menu:

1. Delete or comment out its entry in the `model_presets` array
2. Rebuild the program

**Warning**: If you remove the default model (`grok-code-fast-1`), update the `DEFAULT_MODEL` constant at the top of the file.

## Changing the Default Model

The default model is set with the `DEFAULT_MODEL` constant:

```c
#define DEFAULT_MODEL "grok-code-fast-1"
```

To change it:

1. Update the constant to match one of the model names in your presets array
2. Rebuild

Example:

```c
#define DEFAULT_MODEL "grok-2-latest"
```

## Advanced Customization

### Adding Model Categories

You can organize models by adding visual separators in descriptions:

```c
static const ModelPreset model_presets[] = {
    // --- Fast Models ---
    {
        "grok-code-fast-1",
        "Grok Code Fast",
        "Fast coding - Optimized for quick responses"
    },
    {
        "grok-fast-v1",
        "Grok Fast v1",
        "Fast general - Quick responses for simple queries"
    },
    // --- Advanced Models ---
    {
        "grok-2-latest",
        "Grok 2 Latest",
        "Advanced - Enhanced reasoning capabilities"
    },
};
```

### Customizing the Menu Display

The menu display is handled by the `handle_model_selection()` function. You can customize:

- **Menu title**: Edit the `printf` statement at line ~399
- **Format**: Modify the loop that displays models (line ~403)
- **Current model indicator**: Change the "✓ Currently selected" text (line ~409)

### Adding Model-Specific Parameters

To add parameters like temperature or max tokens per model, extend the `ModelPreset` structure:

```c
typedef struct {
    const char *name;
    const char *label;
    const char *description;
    int max_tokens;             // NEW
    float temperature;          // NEW
} ModelPreset;
```

Then update the array and the `send_grok_request()` function to use these parameters.

## Testing Your Changes

### 1. Compile Check

```bash
make clean
make
```

Ensure there are no compilation errors.

### 2. Run the Program

```bash
./grok-terminal
```

### 3. Test Model Selection

```
> /model
```

Verify:
- All models appear in the menu
- Descriptions are displayed correctly
- Model numbers are sequential (1, 2, 3, ...)
- Current model is marked with ✓

### 4. Test Model Switching

Select each model and verify:
- Selection is confirmed with a success message
- The selected model is used in subsequent API calls
- No errors occur during model switching

### 5. Test Edge Cases

- Enter invalid numbers (0, negative, too large)
- Test with empty input
- Try pressing Ctrl+D during selection

## Best Practices

### Model Names
- Use lowercase with hyphens (kebab-case)
- Match exactly what the xAI API expects
- Keep names concise but descriptive

### Display Labels
- Use Title Case for readability
- Keep under 30 characters
- Include version numbers when relevant

### Descriptions
- Start with the primary use case
- Keep under 100 characters
- Avoid technical jargon
- Be specific about benefits

### Code Style
- Follow existing indentation (4 spaces)
- Add comments for non-obvious changes
- Keep array entries aligned vertically
- Use consistent naming conventions

## Troubleshooting

### "Model not found" API Error

**Problem**: Selected model doesn't exist in xAI API.

**Solution**: Verify the model name in the `model_presets` array matches the API exactly.

### Menu Doesn't Show New Model

**Problem**: Added model but it doesn't appear.

**Solutions**:
- Ensure you recompiled after changes (`make clean && make`)
- Check for syntax errors (missing comma, quotes)
- Verify the model was added before the closing `};`

### Compilation Errors

**Problem**: Code doesn't compile after changes.

**Common Issues**:
- Missing comma between array elements
- Unmatched braces or quotes
- Incorrect constant names

### Model Selection Shows Wrong Numbers

**Problem**: Menu numbering is off.

**Solution**: The `NUM_MODEL_PRESETS` macro automatically calculates the count. No action needed unless you modified the macro.

## Examples

### Minimal Model Entry

```c
{
    "model-id",
    "Short Name",
    "Brief description"
}
```

### Detailed Model Entry

```c
{
    "grok-vision-beta-2024",
    "Grok Vision Beta (2024)",
    "Multimodal model with image understanding and enhanced visual reasoning capabilities for complex analysis"
}
```

### Specialized Coding Model

```c
{
    "grok-code-specialist-v2",
    "Grok Code Specialist v2",
    "Specialized for code review, debugging, and architecture analysis with extended context window"
}
```

## Reference

### Key Functions

- `handle_model_selection()` - Displays menu and handles selection
- `send_grok_request()` - Uses `current_model` for API calls
- `display_help()` - Shows `/model` command in help text

### Key Constants

- `DEFAULT_MODEL` - Initial model on startup
- `MAX_MODEL_NAME_SIZE` - Maximum length for model names (64 chars)
- `NUM_MODEL_PRESETS` - Calculated size of presets array

### Key Variables

- `current_model` - Currently selected model name
- `model_presets[]` - Array of available models

## Contributing

When contributing model additions:

1. Test with the actual xAI API
2. Verify model names are current
3. Write clear, accurate descriptions
4. Follow the existing format
5. Update this documentation if needed

## Support

For questions about:
- xAI model availability: Check xAI documentation
- API endpoints: Refer to xAI API reference
- Code structure: See ARCHITECTURE.md
- General usage: See README.md
