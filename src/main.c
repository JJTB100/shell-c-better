#include <stdio.h>
#include <stdlib.h>
#include "shell_context.h"
#include "line_editor.h"

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

        // Future stages: Tokenise, Parse, Execute happen here.

        free(line);
    }

    int exit_code = ctx.last_exit_code;
    shell_context_destroy(&ctx);
    return exit_code;
}