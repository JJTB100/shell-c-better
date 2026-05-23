// Consumes Tokens and builds an Abstract Syntax Tree (AST). 
// For a basic shell, this is a linked list of Command structs, 
// but it scales to handle nested logic (&&, ||, subshells) if structured as a tree.
#ifndef PARSER_H
#define PARSER_H

#include "tokeniser.h"
#include <stdbool.h>

typedef struct Command {
    char **argv;       // Cleaned argument list for execvp
    int argc;          // Number of arguments
    
    char *output_file; // ">", "1>", ">>", "1>>"
    bool append_out;
    
    char *error_file;  // "2>", "2>>"
    bool append_err;
} Command;

Command *parse_tokens(TokenList *tokens);
void free_command(Command *cmd);

#endif