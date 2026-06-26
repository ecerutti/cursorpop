<!-- Language: **English** · [Español](README.es.md) -->

# CursorPop

[![CI](https://github.com/ecerutti/cursorpop/actions/workflows/ci.yml/badge.svg)](https://github.com/ecerutti/cursorpop/actions/workflows/ci.yml)

Two small visual effects for the mouse cursor on Linux, inspired by macOS:

- **On click**, the cursor briefly shrinks and comes back with a little bounce.
- **On shaking the mouse** quickly side to side, the cursor grows for a moment so you can find it on screen.

Works on any X11 desktop: Cinnamon, GNOME, KDE, XFCE, MATE and others.

> v0.3.0 — Tested on Linux Mint Debian Edition (Cinnamon).

The settings window follows your **system language** (English and Spanish are
included; anything else falls back to English).

---

## Installation

### Option 1 — Debian package (easiest)

Download the `.deb` file from the [releases page](https://github.com/ecerutti/cursorpop/releases) and install it with a double click, or from the terminal:

```bash
sudo dpkg -i cursorpop_0.3.0_amd64.deb
```

If a dependency is missing, run afterwards:

```bash
sudo apt install -f
```

### Option 2 — Build from source

**1. Install the dependencies:**

```bash
# Debian / Ubuntu / Linux Mint / LMDE
sudo apt install build-essential gettext libx11-dev libxfixes-dev libxi-dev libxext-dev \
                 python3-gi gir1.2-gtk-3.0 gir1.2-xapp-1.0
```

**2. Build and install:**

```bash
make
sudo make install
```

**3. (Optional) Build a `.deb` package yourself:**

```bash
make deb
sudo dpkg -i cursorpop_0.3.0_amd64.deb
```

---

## Getting started

After installing, open **CursorPop Settings** from your desktop menu (under Preferences or Accessories).

A window opens where you can:

- Turn each effect on or off independently.
- Adjust how much the cursor shrinks on click and how much it grows on shake.
- **Enable autostart** with the graphical session — just tick "Start with the graphical session" and you're done.

When you click **Apply**, the changes take effect immediately. When you close the window, cursorpop keeps running in the system tray (the mouse icon in the corner of the panel).

### System tray

From the tray icon you can:

- Turn cursorpop on or off with a single click.
- Open the settings window.
- Quit the application entirely.

---

## Uninstall

### If you installed the `.deb` package

```bash
sudo apt remove cursorpop
```

### If you installed with `make install`

```bash
sudo make uninstall
```

### Optional — remove your personal data

The steps above remove the program but leave your per-user files (settings and
the autostart entry) untouched. To remove those too:

```bash
rm -rf ~/.config/cursorpop ~/.config/autostart/cursorpop.desktop
```

---

## FAQ

**Does it work on Wayland?**
No. Wayland does not allow this kind of global cursor effect. It only works in X11 sessions.

**The cursor looks blurry when it grows.**
With very large scale factors (above 2×) it can look a bit blurry, since the cursor image is scaled in software. At the default value (2×) it looks fine.

**Does it need a special compositor?**
It needs an active compositor for the effect's transparency. Most modern desktops have one by default (Cinnamon, GNOME, KDE and XFCE enable it out of the box).

**The GUI does not appear in the menu after installing.**
Try logging out and back in, or run in the terminal:
```bash
update-desktop-database ~/.local/share/applications
```

---

## Advanced configuration (command line)

For users who prefer not to use the GUI, the daemon accepts options when run:

```bash
cursorpop --press-scale 0.75 --grow-scale 2.5 --no-wiggle
```

It also reads `~/.config/cursorpop/cursorpop.conf` at startup. See the
[full CLI and configuration reference](docs/cli-reference.md).

---

## For developers

| Document | Contents |
|----------|----------|
| [docs/architecture.md](docs/architecture.md) | How it works internally: X11, overlay, easing, shake detection |
| [docs/cli-reference.md](docs/cli-reference.md) | All CLI options and the config file format |
| [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md) | How to build, code structure, style guide and how to contribute |
| [CHANGELOG.md](CHANGELOG.md) | Release history |

---

## License

MIT — see [LICENSE](LICENSE).
