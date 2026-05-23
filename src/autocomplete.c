#include "autocomplete.h"
#include "builtins.h"
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct {
    char *command;
    CompType type;
    char *arg; 
} CustomCompletion;

static CustomCompletion *custom_completions = NULL;
static int custom_count = 0;
static int custom_capacity = 0;

static int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

void register_completion(const char *command, CompType type, const char *arg) {
    for (int i = 0; i < custom_count; i++) {
        if (strcmp(custom_completions[i].command, command) == 0) {
            if (custom_completions[i].arg) free(custom_completions[i].arg);
            custom_completions[i].type = type;
            custom_completions[i].arg = arg ? strdup(arg) : NULL;
            return;
        }
    }
    if (custom_count >= custom_capacity) {
        custom_capacity = custom_capacity == 0 ? 4 : custom_capacity * 2;
        custom_completions = realloc(custom_completions, custom_capacity * sizeof(CustomCompletion));
    }
    custom_completions[custom_count].command = strdup(command);
    custom_completions[custom_count].type = type;
    custom_completions[custom_count].arg = arg ? strdup(arg) : NULL;
    custom_count++;
}

bool print_completions(const char *target_command) {
    bool found = false;
    for (int i = 0; i < custom_count; i++) {
        if (!target_command || strcmp(custom_completions[i].command, target_command) == 0) {
            found = true;
            if (custom_completions[i].type == COMP_WORDLIST) {
                printf("complete -W '%s' %s\n", custom_completions[i].arg, custom_completions[i].command);
            } else if (custom_completions[i].type == COMP_COMMAND) {
                printf("complete -C '%s' %s\n", custom_completions[i].arg, custom_completions[i].command);
            } else if (custom_completions[i].type == COMP_FILE) {
                printf("complete -f %s\n", custom_completions[i].command);
            } else if (custom_completions[i].type == COMP_DIR) {
                printf("complete -d %s\n", custom_completions[i].command);
            }
        }
    }
    
    if (target_command && !found) {
        printf("complete: %s: no completion specification\n", target_command);
    }
    
    return found;
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

static void generate_fs_matches(char ***matches, int *count, int *capacity, const char *word, bool dirs_only) {
    char dir_path[1024] = ".";
    char base_prefix[1024];
    strcpy(base_prefix, word);
    
    char *last_slash = strrchr(base_prefix, '/');
    if (last_slash) {
        *last_slash = '\0';
        if (last_slash == base_prefix) strcpy(dir_path, "/");
        else strcpy(dir_path, base_prefix);
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
                bool is_dir = (stat(stat_path, &st) == 0 && S_ISDIR(st.st_mode));
                
                if (dirs_only && !is_dir) continue;
                if (is_dir) strcat(full_match, "/");
                
                add_match(matches, count, capacity, full_match);
            }
        }
        closedir(d);
    }
}

char **get_autocomplete_matches(const char *buffer, int *match_count, int *prefix_pos) {
    int capacity = 10;
    int count = 0;
    char **matches = malloc(capacity * sizeof(char *));
    
    char *last_space = strrchr(buffer, ' ');
    *prefix_pos = last_space ? (last_space - buffer) + 1 : 0;
    const char *word = buffer + *prefix_pos;
    int word_len = strlen(word);

    char prev_word[256] = {0};
    if (*prefix_pos > 0) {
        int p = *prefix_pos - 1;
        while (p > 0 && buffer[p - 1] == ' ') p--; 
        if (p > 0) {
            int start = p - 1;
            while (start >= 0 && buffer[start] != ' ') start--; 
            start++;
            int len = p - start;
            if (len > 0 && len < 255) {
                strncpy(prev_word, &buffer[start], len);
                prev_word[len] = '\0';
            }
        }
    }

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

    bool custom_matched = false;
    if (*prefix_pos > 0) {
        char base_cmd[256] = {0};
        sscanf(buffer, "%s", base_cmd); 

        for (int i = 0; i < custom_count; i++) {
            if (strcmp(custom_completions[i].command, base_cmd) == 0) {
                custom_matched = true;
                
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
                    snprintf(sys_cmd, sizeof(sys_cmd), "%s \"%s\" \"%s\" \"%s\"", 
                             custom_completions[i].arg, base_cmd, word, prev_word);
                    
                    // --- NEW: Set completion environment variables ---
                    char point_str[32];
                    snprintf(point_str, sizeof(point_str), "%zu", strlen(buffer));
                    setenv("COMP_LINE", buffer, 1);
                    setenv("COMP_POINT", point_str, 1);
                    // -------------------------------------------------

                    FILE *fp = popen(sys_cmd, "r");
                    if (fp) {
                        char line[1024];
                        while (fgets(line, sizeof(line), fp)) {
                            line[strcspn(line, "\r\n")] = '\0'; 
                            if (strlen(line) > 0 && strncmp(word, line, word_len) == 0) {
                                add_match(&matches, &count, &capacity, line);
                            }
                        }
                        pclose(fp);
                    }

                    // --- NEW: Clean up the environment variables ---
                    unsetenv("COMP_LINE");
                    unsetenv("COMP_POINT");
                    // -----------------------------------------------
                }
                else if (custom_completions[i].type == COMP_FILE) {
                    generate_fs_matches(&matches, &count, &capacity, word, false);
                }
                else if (custom_completions[i].type == COMP_DIR) {
                    generate_fs_matches(&matches, &count, &capacity, word, true);
                }
            }
        }
    }
    
    if (*prefix_pos > 0 && !custom_matched) {
        generate_fs_matches(&matches, &count, &capacity, word, false);
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

    while (first[i] && last[i] && first[i] == last[i]) i++;

    char *prefix = malloc(i + 1);
    strncpy(prefix, first, i);
    prefix[i] = '\0';
    return prefix;
}

void free_autocomplete_matches(char **matches, int match_count) {
    for (int i = 0; i < match_count; i++) free(matches[i]);
    free(matches);
}