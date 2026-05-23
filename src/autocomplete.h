#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H

#include <stdbool.h>

typedef enum {
    COMP_WORDLIST, // -W
    COMP_COMMAND,  // -C
    COMP_FILE,     // -f
    COMP_DIR       // -d
} CompType;

char **get_autocomplete_matches(const char *buffer, int *match_count, int *prefix_pos);
char *get_longest_common_prefix(char **matches, int match_count);
void free_autocomplete_matches(char **matches, int match_count);

void register_completion(const char *command, CompType type, const char *arg);
bool print_completions(const char *target_command);

// New removal function
bool remove_completion(const char *command);

#endif