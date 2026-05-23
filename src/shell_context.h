// This is passed around to avoid global variables. 
// It holds environment variables, the current working directory, 
// history state, and the exit status of the last command.

#ifndef SHELL_CONTEXT_H
#define SHELL_CONTEXT_H

#include <stdbool.h>
#include <termios.h>

// Encapsulates the entire runtime state of the shell
typedef struct {
    bool is_running;              // REPL loop control
    bool is_interactive;          // True if input is from a TTY
    int last_exit_code;           // Stores $?
    
    // Terminal State
    struct termios orig_termios;  // Saved state for clean exit
    
    // History State
    char *history_file;           // Resolved path to HISTFILE
    int history_depth;            // Current navigation depth in history
    int next_line_to_save;        // Tracking appending to history
    int session_start_line;       // Delta for current session history
} ShellContext;

// Lifecycle
void shell_context_init(ShellContext *ctx);
void shell_context_destroy(ShellContext *ctx);

// Terminal Control
void terminal_enable_raw_mode(ShellContext *ctx);
void terminal_disable_raw_mode(ShellContext *ctx);

#endif // SHELL_CONTEXT_H