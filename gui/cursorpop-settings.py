#!/usr/bin/env python3
"""
cursorpop-settings — GTK GUI to configure and control the cursorpop daemon.

It holds no cursor logic: it only edits ~/.config/cursorpop/cursorpop.conf,
starts/stops the daemon and sends it SIGHUP to reload.

Usage:
    cursorpop-settings           Open the settings window.
    cursorpop-settings --tray    Run in the system tray (autostart).
"""
import os
import sys
import signal
import shutil
import subprocess
import gettext
import locale

import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, GLib, Gio

# Tray: prefer XApp (native to Mint/Cinnamon); otherwise fall back to Gtk.StatusIcon.
HAVE_XAPP = False
try:
    gi.require_version("XApp", "1.0")
    from gi.repository import XApp
    HAVE_XAPP = True
except (ValueError, ImportError):
    pass

# --------------------------- internationalization ---------------------------
# The UI shows the system language when a translation for it exists, and falls
# back to English (the source strings) otherwise.
APP = "cursorpop"


def _find_localedir():
    """Locate the compiled .mo translations, covering both the installed
    layout and the source tree. Returns None to let gettext use the system
    default (which still falls back to English)."""
    env = os.environ.get("CURSORPOP_LOCALEDIR")
    if env:
        return env
    here = os.path.dirname(os.path.abspath(__file__))
    # Installed: <prefix>/bin/cursorpop-settings -> <prefix>/share/locale
    installed = os.path.normpath(os.path.join(here, "..", "share", "locale"))
    if os.path.isdir(installed):
        return installed
    # Source tree: ./locale built by `make mo`
    dev = os.path.normpath(os.path.join(here, "..", "locale"))
    if os.path.isdir(dev):
        return dev
    return None


try:
    locale.setlocale(locale.LC_ALL, "")
except locale.Error:
    pass

_ = gettext.translation(APP, _find_localedir(), fallback=True).gettext


def N_(s):
    """No-op marker: tags a literal for extraction; translate later with _()."""
    return s


# Display name (title case) vs the lowercase "cursorpop" used for the binary,
# package, paths and the gettext domain.
APP_NAME      = "CursorPop"
# Window/menu icon: the white tray icon installed under the canonical app name.
# In the source tree (not installed) it is not in the theme, so the window
# falls back to a generic icon; the installed package resolves it.
APP_ICON      = "cursorpop"
CONF_DIR      = os.path.join(GLib.get_user_config_dir(), "cursorpop")
CONF_PATH     = os.path.join(CONF_DIR, "cursorpop.conf")
AUTOSTART_DIR  = os.path.join(GLib.get_user_config_dir(), "autostart")
AUTOSTART_PATH = os.path.join(AUTOSTART_DIR, "cursorpop.desktop")

AUTOSTART_DESKTOP = """\
[Desktop Entry]
Type=Application
Name=CursorPop
Comment=Animated cursor effects (daemon)
Comment[es]=Efectos animados del cursor (daemon)
Exec=cursorpop-settings --tray
Icon=cursorpop
Terminal=false
NoDisplay=true
X-GNOME-Autostart-enabled=true
"""


TRAY_VARIANTS = ("white", "black")   # white = for dark panels, black = for light


def _auto_variant():
    """Variant chosen by 'Automatic'. The panel/tray background colour is not
    reliably queryable from an app — and no app-level signal predicts it (a
    light app theme often sits next to a dark panel, e.g. Cinnamon's Mint-Y).
    Linux tray panels are predominantly dark, so 'Automatic' uses the white
    icon, which reads well on dark panels. Users with a light panel can pick
    'Black' in Settings."""
    return "white"


def tray_icon(cfg=None):
    """Resolve the tray icon. Returns (is_name, value):
      - (True, "cursorpop-tray-<variant>") when installed in the icon theme
        (preferred: works reliably with both XApp and Gtk status icons);
      - (False, "/abs/path.svg") for the source tree;
      - (True, APP_ICON) as a last resort.
    The variant comes from the 'tray_icon' config key (auto|white|black); 'auto'
    follows the theme darkness. The CURSORPOP_TRAY_ICON env var (absolute path)
    overrides everything."""
    env = os.environ.get("CURSORPOP_TRAY_ICON")
    if env and os.path.exists(env):
        return (False, env)
    if cfg is None:
        cfg = load_conf()
    choice = cfg.get("tray_icon", "auto")
    if choice not in TRAY_VARIANTS:
        choice = _auto_variant()
    name = f"cursorpop-tray-{choice}"
    if Gtk.IconTheme.get_default().has_icon(name):
        return (True, name)
    here = os.path.dirname(os.path.abspath(__file__))
    dev = os.path.normpath(os.path.join(here, "..", "data", name + ".svg"))
    if os.path.exists(dev):
        return (False, dev)
    return (True, APP_ICON)


def app_version():
    """Return the installed daemon version (e.g. '0.3.0'), or '' if unknown.
    Single source of truth: the daemon prints 'cursorpop X.Y.Z' with --version."""
    try:
        out = subprocess.run([find_daemon(), "--version"],
                             capture_output=True, text=True, timeout=3)
        parts = out.stdout.split()
        if len(parts) >= 2:
            return parts[1]
    except (OSError, subprocess.SubprocessError):
        pass
    return ""

# Defaults: must match src/config.c. 'enabled' is GUI state (whether the daemon
# should run); the daemon ignores that key.
DEFAULTS = {
    "enabled": "1",
    "press_enabled": "1",
    "press_scale": "0.80",
    "press_delay": "120",
    "press_duration": "100",
    "release_duration": "240",
    "press_ease": "easeOut",
    "release_ease": "easeOutBack",
    "wiggle_enabled": "1",
    "grow_scale": "2.0",
    "grow_duration": "250",
    "grow_hold": "200",
    "grow_shrink": "200",
    "grow_ease": "easeOut",
    "grow_shrink_ease": "easeInOut",
    "wiggle_window": "600",
    "wiggle_distance": "1200",
    "wiggle_flips": "4",
    "wiggle_velocity": "2.0",
    "fps": "60",
    "tray_icon": "auto",     # auto | white | black (GUI-only; daemon ignores it)
}

# Shake sensitivity levels -> wiggle_distance (px). Names are marked with N_()
# and translated at display time.
SENS_LEVELS = [(N_("Low"), 1800), (N_("Medium"), 1200), (N_("High"), 700)]


# ----------------------------- config I/O -----------------------------------
def load_conf():
    cfg = dict(DEFAULTS)
    try:
        with open(CONF_PATH, "r") as f:
            for line in f:
                line = line.strip()
                if not line or line[0] in "#;" or "=" not in line:
                    continue
                k, v = line.split("=", 1)
                cfg[k.strip()] = v.strip()
    except FileNotFoundError:
        pass
    return cfg


def save_conf(cfg):
    os.makedirs(CONF_DIR, exist_ok=True)
    order = ["enabled", "press_enabled", "press_scale", "press_delay",
             "press_duration", "release_duration", "press_ease", "release_ease",
             "wiggle_enabled", "grow_scale", "grow_duration", "grow_hold",
             "grow_shrink", "grow_ease", "grow_shrink_ease", "wiggle_window",
             "wiggle_distance", "wiggle_flips", "wiggle_velocity", "fps",
             "tray_icon"]
    with open(CONF_PATH, "w") as f:
        f.write("# cursorpop — generated by cursorpop-settings\n")
        for k in order:
            f.write(f"{k}={cfg.get(k, DEFAULTS.get(k, ''))}\n")


# ---------------------------- autostart -------------------------------------
def autostart_enabled():
    return os.path.exists(AUTOSTART_PATH)


def set_autostart(enable):
    if enable:
        os.makedirs(AUTOSTART_DIR, exist_ok=True)
        with open(AUTOSTART_PATH, "w") as f:
            f.write(AUTOSTART_DESKTOP)
    else:
        try:
            os.remove(AUTOSTART_PATH)
        except FileNotFoundError:
            pass


# --------------------------- daemon control ---------------------------------
def pidfile_path():
    rt = GLib.get_user_runtime_dir()
    if rt:
        return os.path.join(rt, "cursorpop.pid")
    return os.path.join(os.path.expanduser("~"), ".cursorpop.pid")


def daemon_pid():
    try:
        with open(pidfile_path()) as f:
            pid = int(f.read().strip())
        os.kill(pid, 0)   # still alive?
        return pid
    except (FileNotFoundError, ValueError, ProcessLookupError, PermissionError):
        return None


def find_daemon():
    p = shutil.which("cursorpop")
    if p:
        return p
    here = os.path.dirname(os.path.abspath(__file__))
    cand = os.path.normpath(os.path.join(here, "..", "cursorpop"))
    return cand if os.path.exists(cand) else "cursorpop"


def start_daemon():
    if daemon_pid():
        return
    try:
        subprocess.Popen([find_daemon()],
                         stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL,
                         start_new_session=True)
    except FileNotFoundError:
        pass


def stop_daemon():
    pid = daemon_pid()
    if pid:
        try:
            os.kill(pid, signal.SIGTERM)
        except ProcessLookupError:
            pass


def reload_daemon():
    pid = daemon_pid()
    if pid:
        os.kill(pid, signal.SIGHUP)


# ------------------------------ window --------------------------------------
class SettingsWindow(Gtk.Window):
    def __init__(self, app):
        super().__init__(title=_("CursorPop Settings"))
        self.app = app
        self.cfg = load_conf()
        self.set_default_size(420, -1)
        self.set_border_width(16)
        self.set_resizable(False)
        try:
            self.set_icon_name(APP_ICON)
        except Exception:
            pass

        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)
        self.add(box)

        # ---- Section: click effect ----
        box.pack_start(self._heading(_("Click effect (Mac style)")), False, False, 0)

        self.sw_press = Gtk.Switch()
        self.sw_press.set_active(self.cfg["press_enabled"] == "1")
        self.sw_press.connect("notify::active", self._on_press_toggled)
        box.pack_start(self._row(_("Enable click effect"), self.sw_press), False, False, 0)

        self.rb_all = Gtk.RadioButton.new_with_label_from_widget(
            None, _("Shrink on every click"))
        self.rb_hold = Gtk.RadioButton.new_with_label_from_widget(
            self.rb_all, _("Only when held down"))
        delay = int(float(self.cfg["press_delay"]))
        if delay <= 0:
            self.rb_all.set_active(True)
        else:
            self.rb_hold.set_active(True)

        self.spin_delay = Gtk.SpinButton.new_with_range(50, 1000, 10)
        self.spin_delay.set_value(delay if delay > 0 else 120)
        hold_row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        hold_row.pack_start(self.rb_hold, False, False, 0)
        hold_row.pack_start(self.spin_delay, False, False, 0)
        hold_row.pack_start(Gtk.Label(label=_("ms")), False, False, 0)

        box.pack_start(self.rb_all, False, False, 0)
        box.pack_start(hold_row, False, False, 0)
        self.rb_hold.connect("toggled", self._sync_sensitivity)

        self.sc_press = self._scale(0.50, 0.95, 0.01,
                                    float(self.cfg["press_scale"]))
        box.pack_start(self._row(_("Size while pressed"), self.sc_press),
                       False, False, 0)

        box.pack_start(Gtk.Separator(), False, False, 0)

        # ---- Section: shake effect ----
        box.pack_start(self._heading(_("Shake effect (grow)")), False, False, 0)

        self.sw_wiggle = Gtk.Switch()
        self.sw_wiggle.set_active(self.cfg["wiggle_enabled"] == "1")
        self.sw_wiggle.connect("notify::active", self._on_wiggle_toggled)
        box.pack_start(self._row(_("Enable shake effect"), self.sw_wiggle),
                       False, False, 0)

        self.sc_grow = self._scale(1.2, 3.0, 0.1, float(self.cfg["grow_scale"]))
        box.pack_start(self._row(_("Size when grown"), self.sc_grow),
                       False, False, 0)

        self.cmb_sens = Gtk.ComboBoxText()
        for name, _dist in SENS_LEVELS:
            self.cmb_sens.append_text(_(name))
        self.cmb_sens.set_active(self._sens_index(float(self.cfg["wiggle_distance"])))
        box.pack_start(self._row(_("Sensitivity"), self.cmb_sens), False, False, 0)

        box.pack_start(Gtk.Separator(), False, False, 0)

        # ---- Section: appearance ----
        box.pack_start(self._heading(_("Appearance")), False, False, 0)

        # Tray icon variant: auto (follow the panel theme), white or black.
        self.tray_values = ["auto", "white", "black"]
        self.cmb_tray = Gtk.ComboBoxText()
        self.cmb_tray.append_text(_("Automatic"))
        self.cmb_tray.append_text(_("White (dark panel)"))
        self.cmb_tray.append_text(_("Black (light panel)"))
        cur = self.cfg.get("tray_icon", "auto")
        self.cmb_tray.set_active(self.tray_values.index(cur)
                                 if cur in self.tray_values else 0)
        box.pack_start(self._row(_("Tray icon"), self.cmb_tray), False, False, 0)

        box.pack_start(Gtk.Separator(), False, False, 0)

        # ---- Section: autostart ----
        box.pack_start(self._heading(_("Autostart")), False, False, 0)

        self.sw_autostart = Gtk.Switch()
        self.sw_autostart.set_active(autostart_enabled())
        self.sw_autostart.connect("notify::active", self._on_autostart_toggled)
        box.pack_start(
            self._row(_("Start with the graphical session"), self.sw_autostart),
            False, False, 0)

        box.pack_start(Gtk.Separator(), False, False, 0)

        # ---- Footer: version (left) + buttons (right) ----
        footer = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        ver = app_version()
        ver_lbl = Gtk.Label(label=f"{APP_NAME} {ver}".strip())
        ver_lbl.get_style_context().add_class("dim-label")
        ver_lbl.set_halign(Gtk.Align.START)
        footer.pack_start(ver_lbl, True, True, 0)

        b_close = Gtk.Button.new_with_label(_("Close"))
        b_close.connect("clicked", lambda *_a: self.close())
        b_apply = Gtk.Button.new_with_label(_("Apply"))
        b_apply.get_style_context().add_class("suggested-action")
        b_apply.connect("clicked", self._on_apply)
        footer.pack_end(b_apply, False, False, 0)
        footer.pack_end(b_close, False, False, 0)
        box.pack_start(footer, False, False, 0)

        self._sync_sensitivity()
        self.connect("destroy", lambda *_a: self.app.on_window_closed())

    # UI helpers
    def _heading(self, text):
        lbl = Gtk.Label()
        lbl.set_markup(f"<b>{text}</b>")
        lbl.set_halign(Gtk.Align.START)
        return lbl

    def _row(self, text, widget):
        row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        lbl = Gtk.Label(label=text)
        lbl.set_halign(Gtk.Align.START)
        row.pack_start(lbl, True, True, 0)
        widget.set_halign(Gtk.Align.END)
        row.pack_start(widget, False, False, 0)
        return row

    def _scale(self, lo, hi, step, value):
        sc = Gtk.Scale.new_with_range(Gtk.Orientation.HORIZONTAL, lo, hi, step)
        sc.set_value(value)
        sc.set_size_request(180, -1)
        sc.set_draw_value(True)
        sc.set_value_pos(Gtk.PositionType.RIGHT)
        return sc

    def _sens_index(self, distance):
        best, bi = 1e9, 1
        for i, (_name, d) in enumerate(SENS_LEVELS):
            if abs(d - distance) < best:
                best, bi = abs(d - distance), i
        return bi

    def _sync_sensitivity(self, *_a):
        self.spin_delay.set_sensitive(self.rb_hold.get_active())

    def _on_press_toggled(self, *_a):
        pass

    def _on_wiggle_toggled(self, *_a):
        pass

    def _on_autostart_toggled(self, switch, *_a):
        set_autostart(switch.get_active())

    def _on_apply(self, *_a):
        self.cfg["press_enabled"] = "1" if self.sw_press.get_active() else "0"
        self.cfg["press_delay"] = "0" if self.rb_all.get_active() \
            else str(int(self.spin_delay.get_value()))
        self.cfg["press_scale"] = f"{self.sc_press.get_value():.2f}"
        self.cfg["wiggle_enabled"] = "1" if self.sw_wiggle.get_active() else "0"
        self.cfg["grow_scale"] = f"{self.sc_grow.get_value():.1f}"
        idx = self.cmb_sens.get_active()
        if idx >= 0:
            self.cfg["wiggle_distance"] = str(SENS_LEVELS[idx][1])
        tidx = self.cmb_tray.get_active()
        if tidx >= 0:
            self.cfg["tray_icon"] = self.tray_values[tidx]
        save_conf(self.cfg)
        self.app.apply_config()


# ------------------------------ app / tray ----------------------------------
class CursorpopApp:
    def __init__(self, tray_mode):
        self.tray_mode = tray_mode
        self.window = None
        self.status_icon = None
        cfg = load_conf()
        if tray_mode:
            self._build_tray()
            if cfg.get("enabled", "1") == "1":
                start_daemon()
            self._refresh_tray()
        else:
            self.open_settings()

    # --- daemon ---
    def set_enabled(self, on):
        cfg = load_conf()
        cfg["enabled"] = "1" if on else "0"
        save_conf(cfg)
        if on:
            start_daemon()
        else:
            stop_daemon()
        self._refresh_tray()

    def apply_config(self):
        cfg = load_conf()
        if cfg.get("enabled", "1") == "1":
            if daemon_pid():
                reload_daemon()
            else:
                start_daemon()
        self._refresh_tray_icon(cfg)
        self._refresh_tray()

    # --- window ---
    def open_settings(self, *_a):
        if self.window is None:
            self.window = SettingsWindow(self)
            self.window.show_all()
        else:
            self.window.present()

    def on_window_closed(self):
        self.window = None
        if self.tray_mode:
            return
        # Transition to tray: the app keeps running as an icon in the tray.
        self.tray_mode = True
        cfg = load_conf()
        if cfg.get("enabled", "1") == "1" and not daemon_pid():
            start_daemon()
        self._build_tray()
        self._refresh_tray()

    def quit(self, *_a):
        Gtk.main_quit()

    # --- tray ---
    def _build_menu(self):
        menu = Gtk.Menu()
        self.mi_enabled = Gtk.CheckMenuItem(label=_("Enabled"))
        self.mi_enabled.set_active(daemon_pid() is not None)
        self._enabled_handler = self.mi_enabled.connect(
            "toggled", lambda w: self.set_enabled(w.get_active()))
        menu.append(self.mi_enabled)
        menu.append(Gtk.SeparatorMenuItem())
        mi_cfg = Gtk.MenuItem(label=_("Settings…"))
        mi_cfg.connect("activate", self.open_settings)
        menu.append(mi_cfg)
        mi_quit = Gtk.MenuItem(label=_("Quit"))
        mi_quit.connect("activate", self.quit)
        menu.append(mi_quit)
        menu.show_all()
        return menu

    def _build_tray(self):
        self.menu = self._build_menu()
        is_name, icon_ref = tray_icon()
        tooltip = f"{APP_NAME} {app_version()}".strip()
        if HAVE_XAPP:
            icon = XApp.StatusIcon()
            icon.set_icon_name(icon_ref)   # themed name (installed) or path (dev)
            icon.set_tooltip_text(tooltip)
            icon.set_primary_menu(self.menu)
            icon.set_secondary_menu(self.menu)
        else:
            icon = Gtk.StatusIcon()
            if is_name:
                icon.set_from_icon_name(icon_ref)
            else:
                icon.set_from_file(icon_ref)
            icon.set_tooltip_text(tooltip)
            icon.connect("activate", self.open_settings)
            icon.connect("popup-menu", self._gtk_popup)
        self.status_icon = icon
        self._watch_config()

    def _watch_config(self):
        """Watch the config file so the tray reflects changes applied from a
        separate settings process (e.g. the tray icon variant) without a
        restart."""
        if getattr(self, "_conf_monitor", None) is not None:
            return
        try:
            gf = Gio.File.new_for_path(CONF_PATH)
            self._conf_monitor = gf.monitor_file(Gio.FileMonitorFlags.NONE, None)
            self._conf_monitor.connect("changed", self._on_conf_changed)
        except Exception:
            self._conf_monitor = None

    def _on_conf_changed(self, monitor, gfile, other, event):
        if event in (Gio.FileMonitorEvent.CHANGES_DONE_HINT,
                     Gio.FileMonitorEvent.CREATED,
                     Gio.FileMonitorEvent.CHANGED):
            self._refresh_tray_icon()
            self._refresh_tray()

    def _refresh_tray_icon(self, cfg=None):
        """Re-apply the tray icon to the live status icon (same-process tray).
        A tray running in a separate process picks the change up on restart."""
        icon = getattr(self, "status_icon", None)
        if icon is None:
            return
        is_name, ref = tray_icon(cfg)
        if HAVE_XAPP:
            icon.set_icon_name(ref)
        elif is_name:
            icon.set_from_icon_name(ref)
        else:
            icon.set_from_file(ref)

    def _gtk_popup(self, icon, button, time):
        self.menu.popup(None, None, Gtk.StatusIcon.position_menu,
                        icon, button, time)

    def _refresh_tray(self):
        if getattr(self, "mi_enabled", None) is not None:
            self.mi_enabled.handler_block(self._enabled_handler)
            self.mi_enabled.set_active(daemon_pid() is not None)
            self.mi_enabled.handler_unblock(self._enabled_handler)


def _sigint_quit():
    Gtk.main_quit()
    return False


def main():
    tray_mode = "--tray" in sys.argv[1:]
    CursorpopApp(tray_mode)
    # Ctrl-C exits cleanly when run from a terminal.
    GLib.unix_signal_add(GLib.PRIORITY_DEFAULT, signal.SIGINT, _sigint_quit)
    Gtk.main()


if __name__ == "__main__":
    main()
