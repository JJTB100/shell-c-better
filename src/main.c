#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell_context.h"
#include "line_editor.h"
#include "tokeniser.h"
#include "executor.h"

int main(void) {
    setbuf(stdout, NULL);

    ShellContext ctx;
    shell_context_init(&ctx);

    while (ctx.is_running) {
        if (ctx.is_interactive) {
            printf("$ ");
            fflush(stdout);
        }

        char *line = read_line(&ctx);
        if (!line) {
            ctx.is_running = false;
            break;
        }

        if (strlen(line) > 0) {
            TokenList tokens = tokenize_line(line);
            
            if (tokens.count > 0) {
                // Skipping parser.c for now; AST is overkill for a single word
                execute_command(&ctx, &tokens);
            }

            free_token_list(&tokens);
        }

        free(line);
    }

    int exit_code = ctx.last_exit_code;
    shell_context_destroy(&ctx);
    return exit_code;
}