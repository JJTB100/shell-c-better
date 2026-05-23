#include "autocomplete.h"
#include "builtins.h"
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>

typedef struct {
    char *command;
    CompType type;
    char *arg; // Holds either the wordlist string or the generator command string
} CustomCompletion;

static CustomCompletion *custom_completions = NULL;
static int custom_count = 0;
static int custom_capacity = 0;

static int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

void register_completion(const char *command, CompType type, const char *arg) {
    // Update existing rule
    for (int i = 0; i < custom_count; i++) {
        if (strcmp(custom_completions[i].command, command) == 0) {
            free(custom_completions[i].arg);
            custom_completions[i].type = type;
            custom_completions[i].arg = strdup(arg);
            return;
        }
    }
    // Add new rule
    if (custom_count >= custom_capacity) {
        custom_capacity = custom_capacity == 0 ? 4 : custom_capacity * 2;
        custom_completions = realloc(custom_completions, custom_capacity * sizeof(CustomCompletion));
    }
    custom_completions[custom_count].command = strdup(command);
    custom_completions[custom_count].type = type;
    custom_completions[custom_count].arg = strdup(arg);
    custom_count++;
}

void print_completions(const char *target_command) {
    for (int i = 0; i < custom_count; i++) {
        if (!target_command || strcmp(custom_completions[i].command, target_command) == 0) {
            const char *flag = (custom_completions[i].type == COMP_WORDLIST) ? "-W" : "-C";
            // Mimic bash 'complete -p' output
            printf("complete %s '%s' %s\n", flag, custom_completions[i].arg, custom_completions[i].command);
        }
    }
}

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

char **get_autocomplete_matches(const char *buffer, int *match_count, int *prefix_pos) {
    int capacity = 10;
    int count = 0;
    char **matches = malloc(capacity * sizeof(char *));
    
    char *last_space = strrchr(buffer, ' ');
    *prefix_pos = last_space ? (last_space - buffer) + 1 : 0;
    const char *word = buffer + *prefix_pos;
    int word_len = strlen(word);

    // 1. Command Completion (Built-ins & PATH)
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

    // 2. Programmable Custom Completions
    if (*prefix_pos > 0) {
        char base_cmd[256] = {0};
        sscanf(buffer, "%s", base_cmd); // Extract the first word (the command)

        for (int i = 0; i < custom_count; i++) {
            if (strcmp(custom_completions[i].command, base_cmd) == 0) {
                
                if (custom_completions[i].type == COMP_WORDLIST) {
                    char *list_copy = strdup(custom_completions[i].arg);
                    char *saveptr;
                    char *token = strtok_r(list_copy, " ", &saveptr);
                    
                    while (token) {
                        if (strncmp(word, token, word_len) == 0) {
                            add_match(&matches, &count, &capacity, token);
                        }
                        token = strtok_r(NULL, " ", &saveptr);
                    }
                    free(list_copy);
                } 
                else if (custom_completions[i].type == COMP_COMMAND) {
                    char sys_cmd[1024];
                    // Bash conventionally passes the base command and the current word to the generator
                    snprintf(sys_cmd, sizeof(sys_cmd), "%s \"%s\" \"%s\"", custom_completions[i].arg, base_cmd, word);
                    
                    FILE *fp = popen(sys_cmd, "r");
                    if (fp) {
                        char line[1024];
                        while (fgets(line, sizeof(line), fp)) {
                            line[strcspn(line, "\r\n")] = '\0'; // Strip trailing newline/carriage return
                            
                            // The shell is responsible for filtering the script's output against the current prefix
                            if (strlen(line) > 0 && strncmp(word, line, word_len) == 0) {
                                add_match(&matches, &count, &capacity, line);
                            }
                        }
                        pclose(fp);
                    }
                }
            }
        }
    }
    
    // 3. File Completion (Fallback)
    char dir_path[1024] = ".";
    char base_prefix[1024];
    strcpy(base_prefix, word);
    
    char *last_slash = strrchr(base_prefix, '/');
    if (last_slash) {
        *last_slash = '\0';
        if (last_slash == base_prefix) {
            strcpy(dir_path, "/");
        } else {
            strcpy(dir_path, base_prefix);
        }
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

char *get_longest_common_prefix(char **matches, int match_count) {
    if (match_count == 0) return strdup("");
    if (match_count == 1) return strdup(matches[0]);

    char *first = matches[0];
    char *last = matches[match_count - 1];
    int i = 0;

    while (first[i] && last[i] && first[i] == last[i]) {
        i++;
    }

    char *prefix = malloc(i + 1);
    strncpy(prefix, first, i);
    prefix[i] = '\0';
    return prefix;
}

void free_autocomplete_matches(char **matches, int match_count) {
    for (int i = 0; i < match_count; i++) free(matches[i]);
    free(matches);
}