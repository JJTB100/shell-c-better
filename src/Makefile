CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -g
SRCS = main.c line_editor.c tokeniser.c parser.c executor.c shell_context.c builtins.c
headers = $(SRCS:.c=.h)
OBJS = $(SRCS:.c=.o)

myshell: $(OBJS)
	$(CC) $(CFLAGS) -o myshell $(OBJS)

%.o: %.c $(headers)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f myshell $(OBJS)