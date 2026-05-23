#include "line_editor.h"
#include "autocomplete.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *read_line(ShellContext *ctx) {
    if (!ctx->is_interactive) {
        char *line = NULL;
        size_t len = 0;
        if (getline(&line, &len, stdin) == -1) {
            free(line);
            return NULL;
        }
        if (len > 0 && line[strlen(line) - 1] == '\n') line[strlen(line) - 1] = '\0';
        return line;
    }

    char buffer[1024] = {0};
    int pos = 0;
    char c;
    int tab_count = 0;

    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == '\n' || c == '\r') {
            printf("\r\n");
            return strdup(buffer);
        } 
        else if (c == '\t') {
            tab_count++;
            int match_count;
            int prefix_pos;
            char **matches = get_autocomplete_matches(buffer, &match_count, &prefix_pos);
            int word_len = pos - prefix_pos;

            if (match_count == 0) {
                printf("\a"); // Bell
            } 
            else if (match_count == 1) {
                const char *to_add = &matches[0][word_len];
                if (strlen(to_add) > 0) {
                    strcat(buffer, to_add);
                    printf("%s", to_add);
                    pos += strlen(to_add);
                }
                
                if (matches[0][strlen(matches[0]) - 1] != '/') {
                    strcat(buffer, " ");
                    printf(" ");
                    pos++;
                }
                tab_count = 0;
            } 
            else {
                char *prefix = get_longest_common_prefix(matches, match_count);
                if (strlen(prefix) > (size_t)word_len) {
                    char *suffix = prefix + word_len;
                    strcat(buffer, suffix);
                    printf("%s", suffix);
                    pos += strlen(suffix);
                } else if (tab_count == 1) {
                    printf("\a");
                }
                
                if (tab_count == 2) {
                    printf("\r\n");
                    for(int i = 0; i < match_count; i++){
                        printf("%s  ", matches[i]);
                    }
                    printf("\r\n$ %s", buffer);
                    tab_count = 0;
                }
                free(prefix);
            }

            free_autocomplete_matches(matches, match_count);
        } 
        else if (c == 127 || c == '\b') {
            tab_count = 0;
            if (pos > 0) {
                pos--;
                buffer[pos] = '\0';
                printf("\b \b");
            }
        } 
        else if (c == 4) { // Ctrl+D
            if (pos == 0) return NULL;
        } 
        else if (c >= 32 && c <= 126) {
            tab_count = 0;
            if (pos < 1023) {
                buffer[pos++] = c;
                buffer[pos] = '\0';
                printf("%c", c);
            }
        }
    }
    
    return NULL;
}