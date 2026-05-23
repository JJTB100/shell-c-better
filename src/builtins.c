#include "builtins.h"
#include <string.h>
#include <stdlib.h>

// --- Forward Declarations ---
static int builtin_exit(ShellContext *ctx, TokenList *tokens);
static int builtin_echo(ShellContext *ctx, TokenList *tokens);
static int builtin_cd(ShellContext *ctx, TokenList *tokens);
static int builtin_history(ShellContext *ctx, TokenList *tokens);
static int builtin_type(ShellContext *ctx, TokenList *tokens);
static int builtin_pwd(ShellContext *ctx, TokenList *tokens);

// --- Dispatch Table ---
// Add new built-ins here. The NULL sentinel marks the end of the array.
static const BuiltinCommand builtins[] = {
    {"exit", builtin_exit},
    {"echo", builtin_echo},
    {"cd", builtin_cd},
    {"history", builtin_history},
    {"type", builtin_type},
    {"pwd", builtin_pwd},
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

static int builtin_echo(ShellContext *ctx, TokenList *tokens) {
    (void)ctx; // Suppress unused parameter warning
    
    for (int i = 1; i < tokens->count; i++) {
        printf("%s", tokens->tokens[i]);
        if (i < tokens->count - 1) {
            printf(" ");
        }
    }
    printf("\n");
    return 0;
}

static int builtin_pwd(ShellContext *ctx, TokenList *tokens) {
    (void)ctx;
    (void)tokens;
    
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0;
    } else {
        perror("pwd error");
        return 1;
    }
}

static int builtin_type(ShellContext *ctx, TokenList *tokens) {
    (void)ctx;
    if (tokens->count < 2) return 1;
    
    char *target = tokens->tokens[1];

    // 1. Check Builtins
    for (int i = 0; builtins[i].name != NULL; i++) {
        if (strcmp(target, builtins[i].name) == 0) {
            printf("%s is a shell builtin\n", target);
            return 0;
        }
    }

    // 2. Check PATH
    char *path_env = getenv("PATH");
    if (path_env == NULL) {
        printf("%s: not found\n", target);
        return 1;
    }

    char *path_copy = strdup(path_env);
    char *saveptr;
    char *dir = strtok_r(path_copy, ":", &saveptr); // Thread-safe tokenization
    char full_path[1024];
    int found = 0;

    while (dir != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, target);
        if (access(full_path, X_OK) == 0) {
            printf("%s is %s\n", target, full_path);
            found = 1;
            break;
        }
        dir = strtok_r(NULL, ":", &saveptr);
    }
    
    if (!found) {
        printf("%s: not found\n", target);
    }
    
    free(path_copy);
    return found ? 0 : 1;
}