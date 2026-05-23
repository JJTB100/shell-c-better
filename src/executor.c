#include "executor.h"
#include "builtins.h"
#include <stdio.h>

void execute_command(ShellContext *ctx, TokenList *tokens) {
    if (tokens->count == 0) return;

    if (handle_builtin(ctx, tokens)) {
        return; // The built-in ran successfully
    }

    char *command = tokens->tokens[0];

    // TODO: Builtins and PATH resolution will be checked here later.
    // For now, every command is invalid.
    printf("%s: command not found\n", command);
    
    // 127 is the standard POSIX exit code for "command not found"
    ctx->last_exit_code = 127; 
}