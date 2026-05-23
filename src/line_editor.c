#include "line_editor.h"
#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

// --- Autocomplete Helpers ---

static int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

static char* longest_common_prefix(char** names, int names_size) {
    if (names_size == 0) return strdup("");
    if (names_size == 1) return strdup(names[0]);

    char* first = names[0];
    char* last = names[names_size - 1];
    int i = 0;

    while (first[i] && last[i] && first[i] == last[i]) {
        i++;
    }

    char* prefix = malloc(i + 1);
    strncpy(prefix, first, i);
    prefix[i] = '\0';
    return prefix;
}

static char **get_all_matches(const char *prefix, int *match_count) {
    int capacity = 10;
    int count = 0;
    char **matches = malloc(capacity * sizeof(char *));
    int prefix_len = strlen(prefix);

    // 1. Search Built-ins
    for (int i = 0; builtins[i].name != NULL; i++) {
        if (strncmp(prefix, builtins[i].name, prefix_len) == 0) {
            if (count >= capacity) {
                capacity *= 2;
                matches = realloc(matches, capacity * sizeof(char *));
            }
            matches[count++] = strdup(builtins[i].name);
        }
    }

    // 2. Search PATH
    char *path_env = getenv("PATH");
    if (path_env != NULL) {
        char *path_copy = strdup(path_env);
        char *saveptr;
        char *dir = strtok_r(path_copy, ":", &saveptr);

        while (dir != NULL) {
            DIR *d = opendir(dir);
            if (d != NULL) {
                struct dirent *entry;
                while ((entry = readdir(d)) != NULL) {
                    if (strncmp(prefix, entry->d_name, prefix_len) == 0 &&
                        strcmp(entry->d_name, ".") != 0 &&
                        strcmp(entry->d_name, "..") != 0) {
                        
                        // Prevent duplicates (e.g. built-in and PATH match)
                        int is_dup = 0;
                        for(int k=0; k<count; k++) {
                            if(strcmp(matches[k], entry->d_name) == 0) {
                                is_dup = 1; break;
                            }
                        }
                        if(is_dup) continue;

                        if (count >= capacity) {
                            capacity *= 2;
                            matches = realloc(matches, capacity * sizeof(char *));
                        }
                        matches[count++] = strdup(entry->d_name);
                    }
                }
                closedir(d);
            }
            dir = strtok_r(NULL, ":", &saveptr);
        }
        free(path_copy);
    }

    qsort(matches, count, sizeof(char *), compare_strings);
    matches[count] = NULL;
    *match_count = count;
    return matches;
}


// --- Main Reader ---

char *read_line(ShellContext *ctx) {
    // If running in a test script or piped input, use standard getline
    if (!ctx->is_interactive) {
        char *line = NULL;
        size_t len = 0;
        if (getline(&line, &len, stdin) == -1) {
            free(line);
            return NULL;
        }
        if (len > 0 && line[strlen(line) - 1] == '\n') {
            line[strlen(line) - 1] = '\0';
        }
        return line;
    }

    // --- Interactive Raw Mode Reader ---
    char buffer[1024] = {0};
    int pos = 0;
    char c;
    int tab_count = 0;

    // Read byte-by-byte directly from standard input
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == '\n' || c == '\r') {
            printf("\r\n");
            return strdup(buffer);
        } 
        else if (c == '\t') { // Tab Completion
            tab_count++;
            int match_count;
            char **matches = get_all_matches(buffer, &match_count);

            if (match_count == 0) {
                printf("\a"); // Bell (No matches)
            } 
            else if (match_count == 1) {
                // Single Match
                const char *to_add = &matches[0][pos];
                if (strlen(to_add) > 0) {
                    strcat(buffer, to_add);
                    printf("%s", to_add);
                    pos += strlen(to_add);
                }
                strcat(buffer, " ");
                printf(" ");
                pos++;
                tab_count = 0;
            } 
            else {
                // Multiple Matches
                char *prefix = longest_common_prefix(matches, match_count);
                if (strlen(prefix) > (size_t)pos) {
                    char *suffix = prefix + pos;
                    strcat(buffer, suffix);
                    printf("%s", suffix);
                    pos += strlen(suffix);
                } else if (tab_count == 1) {
                    printf("\a"); // Bell on first attempt if prefix can't extend
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

            // Cleanup dynamically allocated match array
            for (int i = 0; i < match_count; i++) free(matches[i]);
            free(matches);
            
        } 
        else if (c == 127 || c == '\b') { // Backspace
            tab_count = 0;
            if (pos > 0) {
                pos--;
                buffer[pos] = '\0';
                printf("\b \b"); // Erase character visually
            }
        } 
        else if (c == 4) { // Ctrl+D (EOF)
            if (pos == 0) return NULL;
        } 
        else if (c >= 32 && c <= 126) { // Standard Printable Characters
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