#ifndef BUILTINS_H
#define BUILTINS_H

#include <stdbool.h>
#include "shell_context.h"
#include "parser.h"

typedef int (*builtin_fn)(ShellContext *ctx, Command *cmd);

typedef struct {
    const char *name;
    builtin_fn handler;
} BuiltinCommand;

bool is_builtin(const char *name);
bool handle_builtin(ShellContext *ctx, Command *cmd);

#endif