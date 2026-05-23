#include "executor.h"
#include "builtins.h"
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>

void execute_command(ShellContext *ctx, TokenList *tokens) {
    if (tokens->count == 0) return;

    if (handle_builtin(ctx, tokens)) {
        return; // The built-in ran successfully
    }

    char *command = tokens->tokens[0];

    pid_t pid = fork();

    if (pid == -1) {
        // Fork failed
        perror("fork");
        ctx->last_exit_code = 1;
    } else if (pid == 0) {
        // --- CHILD PROCESS ---
        
        execvp(tokens->tokens[0], tokens->tokens);
        
        printf("%s: command not found\n", tokens->tokens[0]);
        
        exit(127); 
        // 127 is the standard POSIX exit code for "command not found"
        
    } else {
        // --- PARENT PROCESS ---
        
        int status;
        waitpid(pid, &status, 0);
        
        // Extract the actual exit code from the child process
        if (WIFEXITED(status)) {
            ctx->last_exit_code = WEXITSTATUS(status);
        } else {
            ctx->last_exit_code = 1; // Default error fallback if terminated by signal
        }
    }
}