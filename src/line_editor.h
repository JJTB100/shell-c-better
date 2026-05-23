//Handles termios raw mode, history navigation, and autocomplete. 
//It accepts a ShellContext and returns a clean, raw char* buffer.c
#ifndef LINE_EDITOR_H
#define LINE_EDITOR_H

#include "shell_context.h"

char *read_line(ShellContext *ctx);

#endif