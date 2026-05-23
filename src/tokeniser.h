// Takes the raw char* and returns an array or linked list of Token structs 
// (e.g., TOKEN_WORD, TOKEN_PIPE, TOKEN_REDIRECT_OUT). 
// Do not evaluate syntax or execute anything here; only group characters.

#ifndef TOKENISER_H
#define TOKENISER_H

typedef struct {
    char **tokens;
    int count;
    int capacity;
} TokenList;

TokenList tokenize_line(char *line);
void free_token_list(TokenList *list);

#endif