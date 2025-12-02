CC = gcc
CFLAGS = -Wall -Wextra -Werror -g

LIBS = -lunwind -lunwind-x86_64

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c, bin/%.o, $(SRCS))

scc: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

bin/%.o: src/%.c
	@mkdir -p bin
	$(CC) $(CFLAGS) -c $< -o $@
