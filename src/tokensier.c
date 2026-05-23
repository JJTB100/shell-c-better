#include "tokeniser.h"
#include <stdlib.h>
#include <string.h>

TokenList tokenize_line(char *line) {
    TokenList list;
    list.count = 0;
    list.capacity = 16;
    list.tokens = malloc(sizeof(char*) * list.capacity);

    char *saveptr;
    // Delimiters: space, tab, carriage return, newline
    char *token = strtok_r(line, " \t\r\n", &saveptr);
    
    while (token != NULL) {
        if (list.count >= list.capacity - 1) {
            list.capacity *= 2;
            list.tokens = realloc(list.tokens, sizeof(char*) * list.capacity);
        }
        
        // strdup transfers ownership of the string to the TokenList
        list.tokens[list.count++] = strdup(token);
        token = strtok_r(NULL, " \t\r\n", &saveptr);
    }
    
    list.tokens[list.count] = NULL; // POSIX execvp requires a NULL-terminated array
    return list;
}

void free_token_list(TokenList *list) {
    for (int i = 0; i < list->count; i++) {
        free(list->tokens[i]);
    }
    free(list->tokens);
}