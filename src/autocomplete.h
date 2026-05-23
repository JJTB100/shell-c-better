#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H

typedef enum {
    COMP_WORDLIST, // Registered via -W
    COMP_COMMAND   // Registered via -C
} CompType;

char **get_autocomplete_matches(const char *buffer, int *match_count, int *prefix_pos);
char *get_longest_common_prefix(char **matches, int match_count);
void free_autocomplete_matches(char **matches, int match_count);

void register_completion(const char *command, CompType type, const char *arg);

void print_completions(const char *target_command);

#endif