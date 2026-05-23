#include "shell_context.h"
#include "input.h" 

int main() {
    ShellContext ctx;
    shell_context_init(&ctx);
    terminal_enable_raw_mode(&ctx);

    while (ctx.is_running) {
        if (ctx.is_interactive) {
            printf("$ ");
            fflush(stdout);
        }

        // Pass context so read_line can access history and termios
        char *line = read_line(&ctx); 
        
        if (!line) {
            ctx.is_running = false;
            break;
        }

        if (strlen(line) > 0) {
            // Future: Tokenize(line) -> Parse(tokens) -> Execute(ast, &ctx)
        }

        free(line);
    }

    int final_exit = ctx.last_exit_code;
    shell_context_destroy(&ctx);
    return final_exit;
}