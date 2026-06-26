# CursorPop architecture

## Components

cursorpop is split into two independent programs that live in the same repository:

### `cursorpop` — the daemon (C)

It does the animation. It runs in the background with no graphical interface. It reads
`~/.config/cursorpop/cursorpop.conf` at startup and reloads the config on `SIGHUP`.
CLI flags take precedence over the config file.

| File              | Responsibility                                           |
|-------------------|----------------------------------------------------------|
| `src/cursorpop.c` | `main`, event loop, animation state machine              |
| `src/overlay.c`   | ARGB override-redirect, click-through window             |
| `src/capture.c`   | Cursor capture (XFixes) and bilinear scaling             |
| `src/wiggle.c`    | Shake detection                                          |
| `src/easing.c`    | Cubic-bézier curves and presets                          |
| `src/config.c`    | Defaults, CLI parsing and config-file reading            |

The `spikes/` are architecture-validation experiments; they are not part of the binary.

### `cursorpop-settings` — the GUI (Python + GTK)

It only edits `~/.config/cursorpop/cursorpop.conf` and controls the daemon (starts it,
stops it, and sends `SIGHUP` to reload). It contains no animation logic.

Written in Python 3 + GTK 3. It supports `XApp.StatusIcon` (Cinnamon/Mint native tray)
with a fallback to `Gtk.StatusIcon`. Its strings are localized with `gettext` (see
[Internationalization](#internationalization)).

## How the animation works

X11 does not allow animating the hardware cursor directly without doing a *pointer
grab* (which would steal clicks) or relying on non-universal extensions. cursorpop
solves this as follows:

1. **Listen to events without intercepting them.** It uses XInput2 (`XISelectEvents`)
   to receive mouse button and motion events. The events keep propagating normally;
   clicks work just the same.

2. **Capture the current cursor image.** When an effect starts, it calls
   `XFixesGetCursorImage` to get the exact pixels of the cursor at that moment. That
   is why it works with any shape: arrow, hand, text caret, etc.

3. **Draw a scaled sprite in an overlay window.** It creates a 32-bit ARGB window with
   `override-redirect` and a `WM_CLASS`, made click-through (via
   `XShapeCombineRectangles` with an empty `ShapeInput`). This window follows the
   pointer with `XMoveWindow` on every frame.

4. **Hide the real cursor only during the effect.** It uses `XFixesHideCursor` /
   `XFixesShowCursor` to keep the original cursor from showing under the sprite. It is
   restored immediately when the animation ends.

5. **Animate the scale with easing curves.** Each frame computes the scale factor
   according to the configured curve, scales the captured image in software (bilinear
   interpolation in `src/capture.c`) and draws it with `XPutImage`.

## Shake detection

`src/wiggle.c` keeps a circular history of recent positions. On every motion event it
accumulates the travelled distance and counts the horizontal direction changes within
a time window (`wiggle_window`). The effect fires when three conditions are met at the
same time:

- Total distance ≥ `wiggle_distance`
- Direction changes ≥ `wiggle_flips`
- Average velocity ≥ `wiggle_velocity`

## Easing curves

`src/easing.c` implements easing with parametric cubic-bézier curves, just like CSS
`cubic-bezier()`. The presets (`easeOut`, `easeOutBack`, etc.) are aliases for standard
curves. You can also pass a custom curve as `x1,y1,x2,y2`.

The evaluation uses numerical bisection to invert the Bernstein polynomial, with a
tolerance of `1e-6` over ~8 iterations.

## State diagram (click effect)

```
IDLE ──press──► SHRINKING ──timeout──► HOLDING
                                          │
                                       release
                                          │
                                          ▼
                                      RELEASING ──done──► IDLE
```

## Config file

Location: `~/.config/cursorpop/cursorpop.conf`

Format: `key=value`, one per line. Lines starting with `#` or `;` are comments. The GUI
rewrites the whole file when applying changes.

The `enabled` key is GUI-only (it indicates whether the daemon should start at launch).
The daemon ignores it.

## Internationalization

The GUI strings live in English in the source and are wrapped with `gettext` (`_()`).
Translations are kept under `po/` (`po/cursorpop.pot` template, `po/<lang>.po` per
language) and compiled to `locale/<lang>/LC_MESSAGES/cursorpop.mo` by `make mo`, then
installed under `$(PREFIX)/share/locale`. At startup the GUI selects the system
language and falls back to English when no translation exists. The C daemon is not
localized: its `--help` and error messages are English only.
