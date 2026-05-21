CC = gcc
CFLAGS = -Wall -Wextra -g
SRCS = main.c LineEditor.c Tokeniser.c parser.c Executor.c ShellContext.c
headers = $(SRCS:.c=.h)
OBJS = $(SRCS:.c=.o)

myshell: $(OBJS)
	$(CC) $(CFLAGS) -o myshell $(OBJS)

%.o: %.c $(headers)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f myshell $(OBJS)