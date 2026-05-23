// Takes the AST and ShellContext.
// It traverses the tree, handles fork()/execvp(), manages file descriptors for pipes
// and redirections, and delegates to built-in functions.

#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "shell_context.h"
#include "tokeniser.h"

void execute_command(ShellContext *ctx, TokenList *tokens);

#endif