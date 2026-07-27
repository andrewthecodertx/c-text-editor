CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -g -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lncurses

SRCS = main.c editor.c file.c syntax.c ui.c error_handler.c editor_lines_array.c editor_actions.c
OBJS = $(SRCS:.c=.o)
TARGET = erwintext

.PHONY: all clean format

RM = rm -f

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	$(RM) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

format:
	clang-format -i $(shell find . -name '*.c' -o -name '*.h')

install: all
	cp $(TARGET) /usr/local/bin

uninstall:
	rm -f /usr/local/bin/$(TARGET)
