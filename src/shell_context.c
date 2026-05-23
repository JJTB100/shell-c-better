#include "shell_context.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

void shell_context_init(ShellContext *ctx) {
    ctx->is_running = true;
    ctx->last_exit_code = 0;
    ctx->is_interactive = isatty(STDIN_FILENO);
    
    ctx->history_depth = 0;
    ctx->next_line_to_save = 0;
    ctx->session_start_line = 0;

    // Resolve HISTFILE once during startup
    char *env_hist = getenv("HISTFILE");
    if (!env_hist || strcmp(env_hist, "/dev/null") == 0) {
        ctx->history_file = strdup("hist_file.txt");
    } else {
        ctx->history_file = strdup(env_hist);
    }

    if (ctx->is_interactive) {
        tcgetattr(STDIN_FILENO, &ctx->orig_termios);
    }
}

void shell_context_destroy(ShellContext *ctx) {
    if (ctx->history_file) {
        free(ctx->history_file);
        ctx->history_file = NULL;
    }
    // Ensure terminal is reset on exit
    if (ctx->is_interactive) {
        terminal_disable_raw_mode(ctx);
    }
}

void terminal_enable_raw_mode(ShellContext *ctx) {
    if (!ctx->is_interactive) return;
    struct termios raw = ctx->orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void terminal_disable_raw_mode(ShellContext *ctx) {
    if (!ctx->is_interactive) return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &ctx->orig_termios);
}