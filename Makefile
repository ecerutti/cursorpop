# cursorpop — Makefile
PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin
MANDIR  ?= $(PREFIX)/share/man/man1
APPDIR  ?= $(PREFIX)/share/applications
CC      ?= cc
VERSION := 0.1.0

PKGS    := x11 xfixes xi xext
CFLAGS  ?= -O2
CFLAGS  += -std=c11 -Wall -Wextra -D_GNU_SOURCE \
           -DCURSORPOP_VERSION='"$(VERSION)"' $(shell pkg-config --cflags $(PKGS))
LDLIBS  := $(shell pkg-config --libs $(PKGS)) -lm

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
BIN := cursorpop

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

install: $(BIN)
	install -Dm755 $(BIN) $(DESTDIR)$(BINDIR)/$(BIN)
	install -Dm755 gui/cursorpop-settings.py $(DESTDIR)$(BINDIR)/cursorpop-settings
	install -Dm644 data/cursorpop-settings.desktop $(DESTDIR)$(APPDIR)/cursorpop-settings.desktop
	@if [ -f man/cursorpop.1 ]; then \
		install -Dm644 man/cursorpop.1 $(DESTDIR)$(MANDIR)/cursorpop.1; \
	fi
	@echo
	@echo "Para que arranque con la sesión:"
	@echo "  mkdir -p ~/.config/autostart && cp data/cursorpop.desktop ~/.config/autostart/"

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(BIN) \
	      $(DESTDIR)$(BINDIR)/cursorpop-settings \
	      $(DESTDIR)$(APPDIR)/cursorpop-settings.desktop \
	      $(DESTDIR)$(MANDIR)/cursorpop.1

.PHONY: all clean install uninstall
