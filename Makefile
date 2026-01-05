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

test: scc $(QBE) test.c
	./scc
	$(QBE) -o test.s test.qbe
	$(CC) test.s -o test.out
	@echo ./test.out
	@echo Test program exited with code: $$(./test.out; echo $$?)

$(QBE):
	make -C qbe
