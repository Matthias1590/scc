CC = gcc
CFLAGS = -Wall -Wextra -Werror -g

QBE = ./qbe/qbe

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c, bin/%.o, $(SRCS))

scc: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

bin/%.o: src/%.c
	@mkdir -p bin
	$(CC) $(CFLAGS) -c $< -o $@

test: scc $(QBE)
	./scc

$(QBE):
	make -C qbe
