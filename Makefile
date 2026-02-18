CC = gcc
CFLAGS += -Wall -Wextra -Werror -g -MMD -MP

QBE = ./qbe/qbe

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c, bin/%.o, $(SRCS))

all: scc

scc: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

test: scc
	./test_all.py

bin/%.o: src/%.c
	@mkdir -p bin
	$(CC) $(CFLAGS) -c $< -o $@

-include $(OBJ:.o=.d)

$(QBE):
	make -C qbe
