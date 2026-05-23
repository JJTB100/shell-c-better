#include "line_editor.h"
#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

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

// Safely add a match to the dynamic array, preventing duplicates
static void add_match(char ***matches, int *count, int *capacity, const char *new_match) {
    for (int i = 0; i < *count; i++) {
        if (strcmp((*matches)[i], new_match) == 0) return;
    }
    if (*count >= *capacity) {
        *capacity *= 2;
        *matches = realloc(*matches, *capacity * sizeof(char *));
    }
    (*matches)[(*count)++] = strdup(new_match);
}

// Scans Built-ins, PATH, and Filesystem based on cursor context
static char **get_all_matches(const char *buffer, int *match_count, int *prefix_pos) {
    int capacity = 10;
    int count = 0;
    char **matches = malloc(capacity * sizeof(char *));
    
    // Isolate the current word being typed
    char *last_space = strrchr(buffer, ' ');
    *prefix_pos = last_space ? (last_space - buffer) + 1 : 0;
    const char *word = buffer + *prefix_pos;
    int word_len = strlen(word);

    // 1. Search Built-ins & PATH (Only if we are completing the first word/command)
    if (*prefix_pos == 0) {
        for (int i = 0; builtins[i].name != NULL; i++) {
            if (strncmp(word, builtins[i].name, word_len) == 0) {
                add_match(&matches, &count, &capacity, builtins[i].name);
            }
        }

        char *path_env = getenv("PATH");
        if (path_env) {
            char *path_copy = strdup(path_env);
            char *saveptr;
            char *dir = strtok_r(path_copy, ":", &saveptr);
            
            while (dir) {
                DIR *d = opendir(dir);
                if (d) {
                    struct dirent *entry;
                    while ((entry = readdir(d)) != NULL) {
                        if (strncmp(word, entry->d_name, word_len) == 0 &&
                            strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                            add_match(&matches, &count, &capacity, entry->d_name);
                        }
                    }
                    closedir(d);
                }
                dir = strtok_r(NULL, ":", &saveptr);
            }
            free(path_copy);
        }
    }

    // 2. Search Local Filesystem (Always searched, works for relative paths like src/b)
    char dir_path[1024] = ".";
    char base_prefix[1024];
    strcpy(base_prefix, word);
    
    char *last_slash = strrchr(base_prefix, '/');
    if (last_slash) {
        *last_slash = '\0'; // Split the directory path from the file prefix
        if (last_slash == base_prefix) {
            strcpy(dir_path, "/"); // Absolute root
        } else {
            strcpy(dir_path, base_prefix);
        }
        
        // Safely extract the file prefix without memory overlap
        char temp[1024];
        strcpy(temp, last_slash + 1);
        strcpy(base_prefix, temp);
    }

    DIR *d = opendir(dir_path);
    if (d) {
        struct dirent *entry;
        int base_len = strlen(base_prefix);
        
        while ((entry = readdir(d)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            
            if (strncmp(entry->d_name, base_prefix, base_len) == 0) {
                char full_match[2048];
                char stat_path[2048];
                
                // Reconstruct the full path strings
                if (last_slash) {
                    if (strcmp(dir_path, "/") == 0) {
                        snprintf(full_match, sizeof(full_match), "/%s", entry->d_name);
                        snprintf(stat_path, sizeof(stat_path), "/%s", entry->d_name);
                    } else {
                        snprintf(full_match, sizeof(full_match), "%s/%s", dir_path, entry->d_name);
                        snprintf(stat_path, sizeof(stat_path), "%s/%s", dir_path, entry->d_name);
                    }
                } else {
                    strcpy(full_match, entry->d_name);
                    snprintf(stat_path, sizeof(stat_path), "%s/%s", dir_path, entry->d_name);
                }
                
                // Determine if it's a directory
                struct stat st;
                if (stat(stat_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                    strcat(full_match, "/");
                }
                
                add_match(&matches, &count, &capacity, full_match);
            }
        }
        closedir(d);
    }

    qsort(matches, count, sizeof(char *), compare_strings);
    matches[count] = NULL;
    *match_count = count;
    return matches;
}


// --- Main Reader ---

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
            char **matches = get_all_matches(buffer, &match_count, &prefix_pos);
            int word_len = pos - prefix_pos; // Length of the word already typed

            if (match_count == 0) {
                printf("\a"); // Bell
            } 
            else if (match_count == 1) {
                // Add the remainder of the exact match
                const char *to_add = &matches[0][word_len];
                if (strlen(to_add) > 0) {
                    strcat(buffer, to_add);
                    printf("%s", to_add);
                    pos += strlen(to_add);
                }
                
                // Only add a space if the completed match is a file, not a directory
                if (matches[0][strlen(matches[0]) - 1] != '/') {
                    strcat(buffer, " ");
                    printf(" ");
                    pos++;
                }
                tab_count = 0;
            } 
            else {
                char *prefix = longest_common_prefix(matches, match_count);
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

            for (int i = 0; i < match_count; i++) free(matches[i]);
            free(matches);
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