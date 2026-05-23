#ifndef BUILTINS_H
#define BUILTINS_H

#include <stdbool.h>
#include "shell_context.h"
#include "tokeniser.h"

// Every built-in function must match this signature
typedef int (*builtin_fn)(ShellContext *ctx, TokenList *tokens);

typedef struct {
    const char *name;
    builtin_fn handler;
} BuiltinCommand;

// Evaluates if the command is a built-in, runs it if so, and returns true.
bool handle_builtin(ShellContext *ctx, TokenList *tokens);

#endif