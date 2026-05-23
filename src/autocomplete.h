#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H

// Generates an array of matches based on the current input buffer.
// Sets match_count to the total matches found.
// Sets prefix_pos to the index in the buffer where the current word begins.
char **get_autocomplete_matches(const char *buffer, int *match_count, int *prefix_pos);

// Calculates the longest shared string prefix among all matches.
// Returns a dynamically allocated string that must be freed.
char *get_longest_common_prefix(char **matches, int match_count);

// Cleans up the matches array.
void free_autocomplete_matches(char **matches, int match_count);

void register_completion_words(const char *command, const char *wordlist);
#endif