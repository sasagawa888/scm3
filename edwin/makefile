.POSIX:
.DELETE_ON_ERROR:

CC := gcc
LIBS := -lm -ldl -lpthread -lncurses
INCS := 
CURSES_CFLAGS := $(shell ncursesw6-config --cflags)
CURSES_LIBS := $(shell ncursesw6-config --libs)

CFLAGS += $(INCS) -Wall $(CURSES_CFLAGS) -O3 -DNDEBUG=1 -Wno-stringop-truncation

SRC_CII := cii/src/except.c \
           cii/src/fmt.c \
           cii/src/str.c \
           cii/src/text.c \
           cii/src/mem.c


# ====== target ======

TARGET := edwin

all: $(TARGET)

$(TARGET): edlis.o syn_highlight.o 
	$(CC) $(CFLAGS) $^ -o $@ $(CURSES_LIBS)

edlis.o: edlis.c edlis.h term.h
	$(CC) $(CFLAGS) -c $< -o $@

syn_highlight.o: syn_highlight.c 
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ====== install / uninstall ======

PREFIX := /usr/local
DESTDIR :=
INSTALL := install
INSTALL_PROGRAM := $(INSTALL) -m755
MKDIR_PROGRAM := mkdir -p -m 755
bindir := $(PREFIX)/bin

.PHONY: install uninstall clean

install: $(TARGET)
	$(MKDIR_PROGRAM) $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) $(TARGET) $(DESTDIR)$(bindir)/$(TARGET)

uninstall:
	$(RM) $(DESTDIR)$(bindir)/$(TARGET)

clean:
	$(RM) *.o $(OBJ_CII) $(TARGET)