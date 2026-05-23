#include "builtins.h"
#include <string.h>
#include <stdlib.h>

// --- Forward Declarations ---
static int builtin_exit(ShellContext *ctx, TokenList *tokens);

// --- Dispatch Table ---
// Add new built-ins here. The NULL sentinel marks the end of the array.
static const BuiltinCommand builtins[] = {
    {"exit", builtin_exit},
    {NULL, NULL}
};

// --- Entry Point ---
bool handle_builtin(ShellContext *ctx, TokenList *tokens) {
    if (tokens->count == 0) return false;

    const char *cmd = tokens->tokens[0];

    for (int i = 0; builtins[i].name != NULL; i++) {
        if (strcmp(cmd, builtins[i].name) == 0) {
            // Execute the built-in and store its return value as the exit code
            ctx->last_exit_code = builtins[i].handler(ctx, tokens);
            return true;
        }
    }
    
    return false;
}

// --- Built-in Implementations ---

static int builtin_exit(ShellContext *ctx, TokenList *tokens) {
    ctx->is_running = false;
    
    // Support optional exit code argument (e.g., 'exit 0' or 'exit 1')
    if (tokens->count > 1) {
        return atoi(tokens->tokens[1]);
    }
    return 0; // Default success exit code
}