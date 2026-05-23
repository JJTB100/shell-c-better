#include "builtins.h"
#include "shell_context.h"
#include "autocomplete.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

// --- Forward Declarations ---
static int builtin_exit(ShellContext *ctx, Command *cmd);
static int builtin_echo(ShellContext *ctx, Command *cmd);
static int builtin_cd(ShellContext *ctx, Command *cmd);
static int builtin_type(ShellContext *ctx, Command *cmd);
static int builtin_pwd(ShellContext *ctx, Command *cmd);
static int builtin_complete(ShellContext *ctx, Command *cmd);

const BuiltinCommand builtins[] = {
    {"exit", builtin_exit},
    {"echo", builtin_echo},
    {"cd", builtin_cd},
    {"type", builtin_type},
    {"pwd", builtin_pwd},
    {"complete", builtin_complete},
    {NULL, NULL}
};

bool is_builtin(const char *name) {
    for (int i = 0; builtins[i].name != NULL; i++) {
        if (strcmp(name, builtins[i].name) == 0) return true;
    }
    return false;
}

bool handle_builtin(ShellContext *ctx, Command *cmd) {
    if (cmd->argc == 0) return false;
    const char *name = cmd->argv[0];

    for (int i = 0; builtins[i].name != NULL; i++) {
        if (strcmp(name, builtins[i].name) == 0) {
            ctx->last_exit_code = builtins[i].handler(ctx, cmd);
            return true;
        }
    }
    return false;
}

// --- Built-in Implementations ---

static int builtin_exit(ShellContext *ctx, Command *cmd) {
    ctx->is_running = false;
    
    // Support optional exit code argument (e.g., 'exit 0' or 'exit 1')
    if (cmd->argc > 1) {
        return atoi(cmd->argv[1]);
    }
    return 0; // Default success exit code
}

static int builtin_echo(ShellContext *ctx, Command *cmd) {
    (void)ctx; // Suppress unused parameter warning
    
    for (int i = 1; i < cmd->argc; i++) {
        printf("%s", cmd->argv[i]);
        if (i < cmd->argc - 1) {
            printf(" ");
        }
    }
    printf("\n");
    return 0;
}

static int builtin_pwd(ShellContext *ctx, Command *cmd) {
    (void)ctx;
    (void)cmd;
    
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0;
    } else {
        perror("pwd error");
        return 1;
    }
}

static int builtin_type(ShellContext *ctx, Command *cmd) {
    (void)ctx;
    if (cmd->argc < 2) return 1;
    
    char *target = cmd->argv[1];

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

static int builtin_cd(ShellContext *ctx, Command *cmd) {
    (void)ctx; // Suppress unused parameter warning
    
    const char *path;
    
    // Fallback to HOME if no arguments are provided, or if the argument is "~"
    if (cmd->argc < 2 || strcmp(cmd->argv[1], "~") == 0) {
        path = getenv("HOME");
        if (path == NULL) {
            printf("cd: HOME not set\n");
            return 1;
        }
    } else {
        path = cmd->argv[1];
    }
    
    if (chdir(path) != 0) {
        printf("cd: %s: No such file or directory\n", path);
        return 1; // failed cd
    }
    
    return 0; // Success
}

static int builtin_complete(ShellContext *ctx, Command *cmd) {
    (void)ctx;
    
    if (cmd->argc < 2) {
        print_completions(NULL);
        return 0;
    }
    
    if (strcmp(cmd->argv[1], "-p") == 0) {
        if (cmd->argc > 2) {
            if (!print_completions(cmd->argv[2])) {
                return 1;
            }
        } else {
            print_completions(NULL);
        }
        return 0;
    }

    if (strcmp(cmd->argv[1], "-r") == 0) {
        if (cmd->argc < 3) {
            fprintf(stderr, "complete: -r: option requires an argument\n");
            return 1;
        }
        if (!remove_completion(cmd->argv[2])) {
            printf("complete: %s: no completion specification\n", cmd->argv[2]);
            return 1;
        }
        return 0;
    }
    
    if (strcmp(cmd->argv[1], "-f") == 0 || strcmp(cmd->argv[1], "-d") == 0) {
        if (cmd->argc < 3) goto usage;
        CompType type = (strcmp(cmd->argv[1], "-f") == 0) ? COMP_FILE : COMP_DIR;
        register_completion(cmd->argv[2], type, NULL);
        return 0;
    }

    if (strcmp(cmd->argv[1], "-W") == 0 || strcmp(cmd->argv[1], "-C") == 0) {
        if (cmd->argc < 4) goto usage;
        CompType type = (strcmp(cmd->argv[1], "-W") == 0) ? COMP_WORDLIST : COMP_COMMAND;
        register_completion(cmd->argv[3], type, cmd->argv[2]);
        return 0;
    }
    
usage:
    // Updated usage string to include -r
    fprintf(stderr, "Usage: complete [-p] | [-r command] | [-f command] | [-d command] | [-W \"wordlist\" command] | [-C \"command\" command]\n");
    return 1;
}