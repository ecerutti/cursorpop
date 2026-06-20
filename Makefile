# cursorpop — Makefile
PREFIX   ?= /usr/local
BINDIR   ?= $(PREFIX)/bin
MANDIR   ?= $(PREFIX)/share/man/man1
APPDIR   ?= $(PREFIX)/share/applications
CC       ?= cc
VERSION  := 0.1.0
DEB_ARCH := $(shell dpkg --print-architecture 2>/dev/null || echo amd64)
DEB_FILE := cursorpop_$(VERSION)_$(DEB_ARCH).deb
STAGING  := /tmp/cursorpop-deb-staging

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
	install -Dm755 $(BIN)                          $(DESTDIR)$(BINDIR)/$(BIN)
	install -Dm755 gui/cursorpop-settings.py       $(DESTDIR)$(BINDIR)/cursorpop-settings
	install -Dm644 data/cursorpop-settings.desktop $(DESTDIR)$(APPDIR)/cursorpop-settings.desktop
	@if [ -f man/cursorpop.1 ]; then \
		install -Dm644 man/cursorpop.1 $(DESTDIR)$(MANDIR)/cursorpop.1; \
	fi
	@update-desktop-database $(DESTDIR)$(APPDIR) 2>/dev/null || true
	@echo
	@echo "Para que arranque con la sesión:"
	@echo "  make autostart   (instala para el usuario actual)"
	@echo "  — o manualmente:"
	@echo "  mkdir -p ~/.config/autostart && cp data/cursorpop.desktop ~/.config/autostart/"

autostart:
	mkdir -p ~/.config/autostart
	cp data/cursorpop.desktop ~/.config/autostart/cursorpop.desktop
	@echo "Autostart instalado en ~/.config/autostart/cursorpop.desktop"

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(BIN) \
	      $(DESTDIR)$(BINDIR)/cursorpop-settings \
	      $(DESTDIR)$(APPDIR)/cursorpop-settings.desktop \
	      $(DESTDIR)$(MANDIR)/cursorpop.1
	@update-desktop-database $(DESTDIR)$(APPDIR) 2>/dev/null || true

# ── Paquete Debian ────────────────────────────────────────────────────────────
PKGDIR := packaging/debian

deb: $(BIN)
	rm -rf $(STAGING)
	$(MAKE) install DESTDIR=$(STAGING) PREFIX=/usr
	mkdir -p $(STAGING)/DEBIAN
	sed -e 's/@VERSION@/$(VERSION)/g' -e 's/@ARCH@/$(DEB_ARCH)/g' \
	  $(PKGDIR)/control > $(STAGING)/DEBIAN/control
	install -m755 $(PKGDIR)/postinst $(STAGING)/DEBIAN/postinst
	install -m755 $(PKGDIR)/prerm    $(STAGING)/DEBIAN/prerm
	rm -f $(STAGING)/usr/share/applications/mimeinfo.cache
	dpkg-deb --root-owner-group --build $(STAGING) $(DEB_FILE)
	rm -rf $(STAGING)
	@echo
	@echo "Paquete listo: $(DEB_FILE)"
	@echo "Para instalar: sudo dpkg -i $(DEB_FILE)"

.PHONY: all clean install autostart uninstall deb
