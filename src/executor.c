#include "executor.h"
#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

static bool setup_redirections(Command *cmd, int *saved_stdout, int *saved_stderr) {
    if (cmd->output_file) {
        if (saved_stdout) *saved_stdout = dup(STDOUT_FILENO);
        int flags = O_WRONLY | O_CREAT | (cmd->append_out ? O_APPEND : O_TRUNC);
        int fd = open(cmd->output_file, flags, 0644);
        if (fd < 0) return false;
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    if (cmd->error_file) {
        if (saved_stderr) *saved_stderr = dup(STDERR_FILENO);
        int flags = O_WRONLY | O_CREAT | (cmd->append_err ? O_APPEND : O_TRUNC);
        int fd = open(cmd->error_file, flags, 0644);
        if (fd < 0) return false;
        dup2(fd, STDERR_FILENO);
        close(fd);
    }
    return true;
}

static void restore_redirections(int saved_stdout, int saved_stderr) {
    if (saved_stdout != -1) {
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
    }
    if (saved_stderr != -1) {
        dup2(saved_stderr, STDERR_FILENO);
        close(saved_stderr);
    }
}

void execute_command(ShellContext *ctx, Command *cmd) {
    if (cmd->argc == 0) return;

    if (is_builtin(cmd->argv[0])) {
        int saved_out = -1, saved_err = -1;
        
        // Temporarily intercept stdout/stderr for the parent process
        if (setup_redirections(cmd, &saved_out, &saved_err)) {
            handle_builtin(ctx, cmd);
            restore_redirections(saved_out, saved_err);
        } else {
            ctx->last_exit_code = 1;
        }
        return;
    }

    // External Command
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        ctx->last_exit_code = 1;
    } else if (pid == 0) {
        // Child Process: Redirections stay permanent for this process
        if (!setup_redirections(cmd, NULL, NULL)) {
            exit(1);
        }
        
        execvp(cmd->argv[0], cmd->argv);
        printf("%s: command not found\n", cmd->argv[0]);
        exit(127); 
    } else {
        // Parent Process: Wait for child
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            ctx->last_exit_code = WEXITSTATUS(status);
        } else {
            ctx->last_exit_code = 1; 
        }
    }
}