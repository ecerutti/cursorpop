# cursorpop — Makefile
PREFIX    ?= /usr/local
BINDIR    ?= $(PREFIX)/bin
MANDIR    ?= $(PREFIX)/share/man/man1
APPDIR    ?= $(PREFIX)/share/applications
LOCALEDIR ?= $(PREFIX)/share/locale
ICONDIR   ?= $(PREFIX)/share/icons/hicolor/scalable/apps
CC        ?= cc
VERSION   := 0.3.0
DEB_ARCH  := $(shell dpkg --print-architecture 2>/dev/null || echo amd64)
DEB_FILE  := cursorpop_$(VERSION)_$(DEB_ARCH).deb
STAGING   := /tmp/cursorpop-deb-staging

PKGS    := x11 xfixes xi xext
CFLAGS  ?= -O2
CFLAGS  += -std=c11 -Wall -Wextra -D_GNU_SOURCE \
           -DCURSORPOP_VERSION='"$(VERSION)"' $(shell pkg-config --cflags $(PKGS))
LDLIBS  := $(shell pkg-config --libs $(PKGS)) -lm

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
BIN := cursorpop

# Translations (gettext). Add a language code here after creating po/<lang>.po.
LINGUAS := es

all: $(BIN) mo

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# ── Translations ──────────────────────────────────────────────────────────────
# Compile po/<lang>.po -> locale/<lang>/LC_MESSAGES/cursorpop.mo.
# Degrades gracefully when gettext (msgfmt) is not installed: the build still
# succeeds and the GUI falls back to English.
mo:
	@if command -v msgfmt >/dev/null 2>&1; then \
	  for l in $(LINGUAS); do \
	    mkdir -p locale/$$l/LC_MESSAGES; \
	    msgfmt po/$$l.po -o locale/$$l/LC_MESSAGES/cursorpop.mo && \
	    echo "  compiled locale/$$l/LC_MESSAGES/cursorpop.mo"; \
	  done; \
	else \
	  echo "msgfmt not found (install gettext); skipping translations"; \
	fi

# Regenerate the string template from the GUI source (for translators).
pot:
	xgettext --language=Python --keyword=_ --keyword=N_ \
	  --package-name=cursorpop --package-version=$(VERSION) \
	  --msgid-bugs-address=esteban.cerutti@gmail.com \
	  --from-code=UTF-8 -o po/cursorpop.pot gui/cursorpop-settings.py
	@echo "Updated po/cursorpop.pot — run 'msgmerge -U po/<lang>.po po/cursorpop.pot' to refresh catalogs"

clean:
	rm -f $(OBJ) $(BIN)
	rm -rf locale

install: $(BIN) mo
	install -Dm755 $(BIN)                          $(DESTDIR)$(BINDIR)/$(BIN)
	install -Dm755 gui/cursorpop-settings.py       $(DESTDIR)$(BINDIR)/cursorpop-settings
	install -Dm644 data/cursorpop-settings.desktop $(DESTDIR)$(APPDIR)/cursorpop-settings.desktop
	install -Dm644 data/cursorpop-tray-white.svg   $(DESTDIR)$(ICONDIR)/cursorpop-tray-white.svg
	install -Dm644 data/cursorpop-tray-black.svg   $(DESTDIR)$(ICONDIR)/cursorpop-tray-black.svg
	install -Dm644 data/cursorpop-tray-white.svg   $(DESTDIR)$(ICONDIR)/cursorpop.svg
	@if [ -f man/cursorpop.1 ]; then \
		install -Dm644 man/cursorpop.1 $(DESTDIR)$(MANDIR)/cursorpop.1; \
	fi
	@for l in $(LINGUAS); do \
		if [ -f locale/$$l/LC_MESSAGES/cursorpop.mo ]; then \
			install -Dm644 locale/$$l/LC_MESSAGES/cursorpop.mo \
				$(DESTDIR)$(LOCALEDIR)/$$l/LC_MESSAGES/cursorpop.mo; \
		fi; \
	done
	@update-desktop-database $(DESTDIR)$(APPDIR) 2>/dev/null || true
	@echo
	@echo "To start it with your session:"
	@echo "  make autostart   (installs for the current user)"
	@echo "  — or manually:"
	@echo "  mkdir -p ~/.config/autostart && cp data/cursorpop.desktop ~/.config/autostart/"

autostart:
	mkdir -p ~/.config/autostart
	cp data/cursorpop.desktop ~/.config/autostart/cursorpop.desktop
	@echo "Autostart installed at ~/.config/autostart/cursorpop.desktop"

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(BIN) \
	      $(DESTDIR)$(BINDIR)/cursorpop-settings \
	      $(DESTDIR)$(APPDIR)/cursorpop-settings.desktop \
	      $(DESTDIR)$(ICONDIR)/cursorpop-tray-white.svg \
	      $(DESTDIR)$(ICONDIR)/cursorpop-tray-black.svg \
	      $(DESTDIR)$(ICONDIR)/cursorpop.svg \
	      $(DESTDIR)$(MANDIR)/cursorpop.1
	@for l in $(LINGUAS); do \
		rm -f $(DESTDIR)$(LOCALEDIR)/$$l/LC_MESSAGES/cursorpop.mo; \
	done
	@update-desktop-database $(DESTDIR)$(APPDIR) 2>/dev/null || true
	@echo "Uninstalled. Per-user data is left untouched; remove it with:"
	@echo "  rm -rf ~/.config/cursorpop ~/.config/autostart/cursorpop.desktop"

# ── Debian package ────────────────────────────────────────────────────────────
PKGDIR := packaging/debian

deb: $(BIN) mo
	rm -rf $(STAGING)
	$(MAKE) install DESTDIR=$(STAGING) PREFIX=/usr
	mkdir -p $(STAGING)/DEBIAN
	sed -e 's/@VERSION@/$(VERSION)/g' -e 's/@ARCH@/$(DEB_ARCH)/g' \
	  $(PKGDIR)/control > $(STAGING)/DEBIAN/control
	install -m755 $(PKGDIR)/postinst $(STAGING)/DEBIAN/postinst
	install -m755 $(PKGDIR)/prerm    $(STAGING)/DEBIAN/prerm
	install -m755 $(PKGDIR)/postrm   $(STAGING)/DEBIAN/postrm
	rm -f $(STAGING)/usr/share/applications/mimeinfo.cache
	dpkg-deb --root-owner-group --build $(STAGING) $(DEB_FILE)
	rm -rf $(STAGING)
	@echo
	@echo "Package ready: $(DEB_FILE)"
	@echo "To install: sudo dpkg -i $(DEB_FILE)"

.PHONY: all clean install autostart uninstall deb mo pot
