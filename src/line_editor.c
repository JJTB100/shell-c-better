#include "line_editor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char *read_line(ShellContext *ctx) {
    (void)ctx; // Unused until raw mode/history is implemented
    
    char *line = NULL;
    size_t len = 0;
    ssize_t read = getline(&line, &len, stdin);

    if (read == -1) {
        free(line);
        return NULL;
    }

    if (read > 0 && line[read - 1] == '\n') {
        line[read - 1] = '\0';
    }

    return line;
}