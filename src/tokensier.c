#include "tokeniser.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

TokenList tokenize_line(char *line) {
    TokenList list;
    list.count = 0;
    list.capacity = 16;
    list.tokens = malloc(sizeof(char*) * list.capacity);

    int len = strlen(line);
    
    // A single token can never be longer than the entire input line
    char *buffer = malloc(len + 1);
    int buf_idx = 0;

    bool in_single = false;
    bool in_double = false;
    bool in_token = false; // Tracks if we are currently building a token

    for (int i = 0; i < len; i++) {
        char c = line[i];

        if (c == '\\') {
            if (in_single) {
                // Inside single quotes, backslash is just a literal backslash
                buffer[buf_idx++] = c;
            } else if (in_double) {
                // Inside double quotes, backslash escapes specific characters
                if (i + 1 < len) {
                    char next = line[i + 1];
                    if (next == '"' || next == '\\' || next == '$' || next == '`' || next == '\n') {
                        buffer[buf_idx++] = next;
                        i++; // Skip the escaped character
                    } else {
                        // If it doesn't escape a special char, keep the backslash literal
                        buffer[buf_idx++] = c;
                    }
                } else {
                    buffer[buf_idx++] = c;
                }
            } else {
                // Outside quotes, backslash escapes ANY next literal character
                if (i + 1 < len) {
                    buffer[buf_idx++] = line[i + 1];
                    i++;
                    in_token = true;
                }
            }
        } else if (c == '\'') {
            if (!in_double) {
                in_single = !in_single; // Toggle state
                in_token = true;        // Even empty quotes '' form a valid token
            } else {
                buffer[buf_idx++] = c;  // Literal quote inside double quotes
            }
        } else if (c == '"') {
            if (!in_single) {
                in_double = !in_double; // Toggle state
                in_token = true;
            } else {
                buffer[buf_idx++] = c;  // Literal quote inside single quotes
            }
        } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            if (in_single || in_double) {
                // Whitespace is literal inside any quotes
                buffer[buf_idx++] = c;
            } else {
                // Unquoted whitespace terminates the current token
                if (in_token) {
                    buffer[buf_idx] = '\0';
                    
                    if (list.count >= list.capacity - 1) {
                        list.capacity *= 2;
                        list.tokens = realloc(list.tokens, sizeof(char*) * list.capacity);
                    }
                    
                    list.tokens[list.count++] = strdup(buffer);
                    buf_idx = 0;
                    in_token = false;
                }
            }
        } else {
            // Standard character
            buffer[buf_idx++] = c;
            in_token = true;
        }
    }

    // Capture the final token if the line didn't end with whitespace
    if (in_token) {
        buffer[buf_idx] = '\0';
        if (list.count >= list.capacity - 1) {
            list.capacity *= 2;
            list.tokens = realloc(list.tokens, sizeof(char*) * list.capacity);
        }
        list.tokens[list.count++] = strdup(buffer);
    }

    list.tokens[list.count] = NULL; // Required by execvp
    free(buffer);
    
    return list;
}

void free_token_list(TokenList *list) {
    for (int i = 0; i < list->count; i++) {
        free(list->tokens[i]);
    }
    free(list->tokens);
}