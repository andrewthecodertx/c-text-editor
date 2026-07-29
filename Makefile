CC ?= cc
PKG_CONFIG ?= pkg-config

TARGET := erwintext
SRCS := main.c editor.c file.c syntax.c ui.c error_handler.c editor_lines_array.c \
	editor_actions.c
HEADERS := $(wildcard *.h)

BUILD_DIR ?= build
DEBUG_DIR := $(BUILD_DIR)/debug
RELEASE_DIR := $(BUILD_DIR)/release
TSAN_DIR := $(BUILD_DIR)/tsan

DEBUG_OBJS := $(SRCS:%.c=$(DEBUG_DIR)/%.o)
RELEASE_OBJS := $(SRCS:%.c=$(RELEASE_DIR)/%.o)
TSAN_OBJS := $(SRCS:%.c=$(TSAN_DIR)/%.o)

DEBUG_BIN := $(DEBUG_DIR)/$(TARGET)
RELEASE_BIN := $(RELEASE_DIR)/$(TARGET)
TSAN_BIN := $(TSAN_DIR)/$(TARGET)

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
DESTDIR ?=

CPPFLAGS += -D_POSIX_C_SOURCE=200809L
WARNFLAGS := -Wall -Wextra -Wpedantic
COMMON_CFLAGS := -std=c99 $(WARNFLAGS) -MMD -MP
DEBUG_CFLAGS := -O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer
RELEASE_CFLAGS := -O2 -DNDEBUG
TSAN_CFLAGS := -O1 -g3 -fsanitize=thread -fno-omit-frame-pointer

NCURSES_CFLAGS := $(shell $(PKG_CONFIG) --cflags ncurses 2>/dev/null)
NCURSES_LIBS := $(shell $(PKG_CONFIG) --libs ncurses 2>/dev/null)
ifeq ($(strip $(NCURSES_LIBS)),)
NCURSES_LIBS := -lncurses
endif

CPPFLAGS += $(NCURSES_CFLAGS)
LDLIBS += $(NCURSES_LIBS)

.PHONY: all debug release tsan clean distclean format format-check check install uninstall

all: debug

debug: $(DEBUG_BIN)
	cp $(DEBUG_BIN) $(TARGET)

release: $(RELEASE_BIN)
	cp $(RELEASE_BIN) $(TARGET)

tsan: $(TSAN_BIN)
	cp $(TSAN_BIN) $(TARGET)

$(DEBUG_BIN): $(DEBUG_OBJS)
	$(CC) $(LDFLAGS) $(DEBUG_CFLAGS) $^ $(LDLIBS) -o $@

$(RELEASE_BIN): $(RELEASE_OBJS)
	$(CC) $(LDFLAGS) $(RELEASE_CFLAGS) $^ $(LDLIBS) -o $@

$(TSAN_BIN): $(TSAN_OBJS)
	$(CC) $(LDFLAGS) $(TSAN_CFLAGS) $^ $(LDLIBS) -o $@

$(DEBUG_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(COMMON_CFLAGS) $(DEBUG_CFLAGS) -c $< -o $@

$(RELEASE_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(COMMON_CFLAGS) $(RELEASE_CFLAGS) -c $< -o $@

$(TSAN_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(COMMON_CFLAGS) $(TSAN_CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

distclean: clean
	rm -f *.gcda *.gcno *.gcov

format:
	clang-format -i $(SRCS) $(HEADERS)

format-check:
	clang-format --dry-run --Werror $(SRCS) $(HEADERS)

check:
	cppcheck --enable=warning,performance,portability --std=c99 \
		--error-exitcode=1 --suppress=missingIncludeSystem $(SRCS)

install: release
	install -d "$(DESTDIR)$(BINDIR)"
	install -m 755 $(TARGET) "$(DESTDIR)$(BINDIR)/$(TARGET)"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/$(TARGET)"

-include $(DEBUG_OBJS:.o=.d)
-include $(RELEASE_OBJS:.o=.d)
-include $(TSAN_OBJS:.o=.d)
