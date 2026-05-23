#include "parser.h"
#include <stdlib.h>
#include <string.h>

Command *parse_tokens(TokenList *tokens) {
    Command *cmd = malloc(sizeof(Command));
    // Allocate max possible size for argv
    cmd->argv = malloc(sizeof(char*) * (tokens->count + 1));
    cmd->argc = 0;
    
    cmd->output_file = NULL;
    cmd->append_out = false;
    cmd->error_file = NULL;
    cmd->append_err = false;

    for (int i = 0; i < tokens->count; i++) {
        char *t = tokens->tokens[i];

        if (strcmp(t, ">") == 0 || strcmp(t, "1>") == 0) {
            if (i + 1 < tokens->count) {
                cmd->output_file = tokens->tokens[++i];
                cmd->append_out = false;
            }
        } else if (strcmp(t, ">>") == 0 || strcmp(t, "1>>") == 0) {
            if (i + 1 < tokens->count) {
                cmd->output_file = tokens->tokens[++i];
                cmd->append_out = true;
            }
        } else if (strcmp(t, "2>") == 0) {
            if (i + 1 < tokens->count) {
                cmd->error_file = tokens->tokens[++i];
                cmd->append_err = false;
            }
        } else if (strcmp(t, "2>>") == 0) {
            if (i + 1 < tokens->count) {
                cmd->error_file = tokens->tokens[++i];
                cmd->append_err = true;
            }
        } else {
            // Borrow the pointer from TokenList. 
            // We do not duplicate the string, saving memory.
            cmd->argv[cmd->argc++] = t;
        }
    }
    
    cmd->argv[cmd->argc] = NULL; // Required by execvp
    return cmd;
}

void free_command(Command *cmd) {
    if (!cmd) return;
    // We only free the argv array itself, not the strings inside it,
    // because the strings are owned and freed by TokenList.
    free(cmd->argv);
    free(cmd);
}