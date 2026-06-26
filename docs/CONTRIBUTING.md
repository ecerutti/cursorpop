# Contributing to CursorPop

Thanks for your interest! cursorpop is a small project: a C daemon for X11 and a
Python + GTK GUI. The code is simple and split into modules with clear
responsibilities, so it is easy to get into.

## Development environment

### Build dependencies

```bash
# Debian / Ubuntu / Linux Mint / LMDE
sudo apt install build-essential gettext libx11-dev libxfixes-dev libxi-dev libxext-dev \
                 python3-gi gir1.2-gtk-3.0 gir1.2-xapp-1.0
```

`gettext` provides `msgfmt`/`xgettext`, used to compile and extract the GUI
translations. If it is missing, `make` still builds the daemon and skips the
translations (the GUI then falls back to English).

### Build

```bash
make            # daemon in ./cursorpop, translations in ./locale
make clean      # remove objects, binary and locale
```

For development, build with sanitizers to catch memory bugs:

```bash
make CFLAGS="-O0 -g -std=c11 -Wall -Wextra -fsanitize=address,undefined -D_GNU_SOURCE"
```

### Run the GUI without installing

```bash
python3 gui/cursorpop-settings.py
```

The GUI finds the `./cursorpop` binary automatically if it is not on the `PATH`, and
finds the compiled translations under `./locale` (run `make mo` first to see the GUI in
another language while developing).

## Code structure

```
src/            C daemon
  cursorpop.c   main, event loop, animation state machine
  overlay.c     ARGB override-redirect, click-through window
  capture.c     Cursor capture (XFixes) and bilinear scaling
  wiggle.c      Shake detection
  easing.c      Cubic-bézier curves and presets
  config.c      Defaults, CLI parsing and config-file reading

gui/            Python + GTK GUI
  cursorpop-settings.py

po/             Translations (gettext)
  cursorpop.pot   String template (regenerate with `make pot`)
  es.po           Spanish translation

data/           Desktop files
  cursorpop-settings.desktop   System menu entry (Preferences)
  cursorpop.desktop             Autostart entry (~/.config/autostart/)

packaging/      Files to build the Debian package
  debian/
    control     Package metadata (version and arch substituted with sed)
    postinst    Post-install script
    prerm       Pre-removal script
    postrm      Post-removal script

man/            Manual page
  cursorpop.1

spikes/         Architecture-validation experiments (not part of the binary)
```

For a detailed technical description of how the animation works, see
[architecture.md](architecture.md).

## Style guide

**C (daemon):**
- C11 standard. No dependencies beyond Xlib, XFixes, XInput2 and XExt.
- No warnings with `-Wall -Wextra`.
- Short functions with a single responsibility.
- `snake_case` names.

**Python (GUI):**
- Python 3.6+, no dependencies beyond PyGObject.
- The GUI contains no animation logic: it only edits the config file and controls the
  daemon.
- Wrap every user-facing string in `_()` so it can be translated.

## Translations (i18n)

The GUI is localized with `gettext`. Source strings are in English; translations live
under `po/`.

To add a new language (for example, French — `fr`):

```bash
make pot                       # refresh po/cursorpop.pot from the source
cp po/cursorpop.pot po/fr.po   # create the new catalog
# translate every msgstr in po/fr.po, then:
make mo                        # compile to locale/fr/LC_MESSAGES/cursorpop.mo
LANGUAGE=fr python3 gui/cursorpop-settings.py   # check it
```

Add the new language code to `LINGUAS` in the `Makefile` so it gets compiled and
installed. After changing GUI strings, run `make pot` and `msgmerge` the existing
catalogs.

## How to contribute

1. **Open an issue** before starting if the change is large, to avoid duplicate work.
2. **Fork** and work on a descriptive branch (`fix/overlay-resize`,
   `feat/wayland-backend`, etc.).
3. Make sure `make` builds without warnings.
4. **Test on at least one X11 environment** and note it in the PR (distribution,
   desktop, version).
5. For GUI changes, also test the autostart flow and the tray icon.

## Ideas to contribute

- Support for custom icon themes in the GUI.
- A manual language selector in the GUI (override the system language).
- More translations (see [Translations](#translations-i18n)).
- Packaging for other distributions (RPM, AUR).
- Automated tests for shake detection.
- A more complete manual page.
