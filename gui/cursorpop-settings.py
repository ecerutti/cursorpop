#!/usr/bin/env python3
"""
cursorpop-settings — GUI (GTK) para configurar y controlar el daemon cursorpop.

No contiene la lógica del cursor: sólo edita ~/.config/cursorpop/cursorpop.conf,
arranca/detiene el daemon y le manda SIGHUP para que recargue.

Uso:
    cursorpop-settings           Abre la ventana de configuración.
    cursorpop-settings --tray    Corre en la bandeja del sistema (autostart).
"""
import os
import sys
import signal
import shutil
import subprocess

import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, GLib

# Bandeja: preferimos XApp (nativo de Mint/Cinnamon); si no, Gtk.StatusIcon.
HAVE_XAPP = False
try:
    gi.require_version("XApp", "1.0")
    from gi.repository import XApp
    HAVE_XAPP = True
except (ValueError, ImportError):
    pass

APP_ICON = "input-mouse"
CONF_DIR = os.path.join(GLib.get_user_config_dir(), "cursorpop")
CONF_PATH = os.path.join(CONF_DIR, "cursorpop.conf")

# Defaults: deben coincidir con src/config.c. 'enabled' es estado de la GUI
# (si el daemon debe correr); el daemon ignora esa clave.
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
}

# Niveles de sensibilidad de la sacudida -> wiggle_distance (px).
SENS_LEVELS = [("Baja", 1800), ("Media", 1200), ("Alta", 700)]


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
             "wiggle_distance", "wiggle_flips", "wiggle_velocity", "fps"]
    with open(CONF_PATH, "w") as f:
        f.write("# cursorpop — generado por cursorpop-settings\n")
        for k in order:
            f.write(f"{k}={cfg.get(k, DEFAULTS.get(k, ''))}\n")


# --------------------------- control del daemon -----------------------------
def pidfile_path():
    rt = GLib.get_user_runtime_dir()
    if rt:
        return os.path.join(rt, "cursorpop.pid")
    return os.path.join(os.path.expanduser("~"), ".cursorpop.pid")


def daemon_pid():
    try:
        with open(pidfile_path()) as f:
            pid = int(f.read().strip())
        os.kill(pid, 0)   # ¿sigue vivo?
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


# ------------------------------ ventana -------------------------------------
class SettingsWindow(Gtk.Window):
    def __init__(self, app):
        super().__init__(title="Configuración de cursorpop")
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

        # ---- Sección: efecto de click ----
        box.pack_start(self._heading("Efecto de click (estilo Mac)"), False, False, 0)

        self.sw_press = Gtk.Switch()
        self.sw_press.set_active(self.cfg["press_enabled"] == "1")
        self.sw_press.connect("notify::active", self._on_press_toggled)
        box.pack_start(self._row("Activar efecto de click", self.sw_press), False, False, 0)

        self.rb_all = Gtk.RadioButton.new_with_label_from_widget(
            None, "Achicar en todos los clicks")
        self.rb_hold = Gtk.RadioButton.new_with_label_from_widget(
            self.rb_all, "Solo si mantengo apretado")
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
        hold_row.pack_start(Gtk.Label(label="ms"), False, False, 0)

        box.pack_start(self.rb_all, False, False, 0)
        box.pack_start(hold_row, False, False, 0)
        self.rb_hold.connect("toggled", self._sync_sensitivity)

        self.sc_press = self._scale(0.50, 0.95, 0.01,
                                    float(self.cfg["press_scale"]))
        box.pack_start(self._row("Tamaño al apretar", self.sc_press),
                       False, False, 0)

        box.pack_start(Gtk.Separator(), False, False, 0)

        # ---- Sección: efecto de sacudida ----
        box.pack_start(self._heading("Efecto de sacudida (agrandar)"), False, False, 0)

        self.sw_wiggle = Gtk.Switch()
        self.sw_wiggle.set_active(self.cfg["wiggle_enabled"] == "1")
        self.sw_wiggle.connect("notify::active", self._on_wiggle_toggled)
        box.pack_start(self._row("Activar efecto de sacudida", self.sw_wiggle),
                       False, False, 0)

        self.sc_grow = self._scale(1.2, 3.0, 0.1, float(self.cfg["grow_scale"]))
        box.pack_start(self._row("Tamaño al agrandar", self.sc_grow),
                       False, False, 0)

        self.cmb_sens = Gtk.ComboBoxText()
        for name, _ in SENS_LEVELS:
            self.cmb_sens.append_text(name)
        self.cmb_sens.set_active(self._sens_index(float(self.cfg["wiggle_distance"])))
        box.pack_start(self._row("Sensibilidad", self.cmb_sens), False, False, 0)

        box.pack_start(Gtk.Separator(), False, False, 0)

        # ---- Botones ----
        btns = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        btns.set_halign(Gtk.Align.END)
        b_close = Gtk.Button.new_with_label("Cerrar")
        b_close.connect("clicked", lambda *_: self.close())
        b_apply = Gtk.Button.new_with_label("Aplicar")
        b_apply.get_style_context().add_class("suggested-action")
        b_apply.connect("clicked", self._on_apply)
        btns.pack_start(b_close, False, False, 0)
        btns.pack_start(b_apply, False, False, 0)
        box.pack_start(btns, False, False, 0)

        self._sync_sensitivity()
        self.connect("destroy", lambda *_: self.app.on_window_closed())

    # helpers de UI
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
        for i, (_, d) in enumerate(SENS_LEVELS):
            if abs(d - distance) < best:
                best, bi = abs(d - distance), i
        return bi

    def _sync_sensitivity(self, *_):
        self.spin_delay.set_sensitive(self.rb_hold.get_active())

    def _on_press_toggled(self, *_):
        pass

    def _on_wiggle_toggled(self, *_):
        pass

    def _on_apply(self, *_):
        self.cfg["press_enabled"] = "1" if self.sw_press.get_active() else "0"
        self.cfg["press_delay"] = "0" if self.rb_all.get_active() \
            else str(int(self.spin_delay.get_value()))
        self.cfg["press_scale"] = f"{self.sc_press.get_value():.2f}"
        self.cfg["wiggle_enabled"] = "1" if self.sw_wiggle.get_active() else "0"
        self.cfg["grow_scale"] = f"{self.sc_grow.get_value():.1f}"
        idx = self.cmb_sens.get_active()
        if idx >= 0:
            self.cfg["wiggle_distance"] = str(SENS_LEVELS[idx][1])
        save_conf(self.cfg)
        self.app.apply_config()


# ------------------------------ app/bandeja ---------------------------------
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
        self._refresh_tray()

    # --- ventana ---
    def open_settings(self, *_):
        if self.window is None:
            self.window = SettingsWindow(self)
            self.window.show_all()
        else:
            self.window.present()

    def on_window_closed(self):
        self.window = None
        if not self.tray_mode:
            Gtk.main_quit()

    def quit(self, *_):
        Gtk.main_quit()

    # --- bandeja ---
    def _build_menu(self):
        menu = Gtk.Menu()
        self.mi_enabled = Gtk.CheckMenuItem(label="Activado")
        self.mi_enabled.set_active(daemon_pid() is not None)
        self._enabled_handler = self.mi_enabled.connect(
            "toggled", lambda w: self.set_enabled(w.get_active()))
        menu.append(self.mi_enabled)
        menu.append(Gtk.SeparatorMenuItem())
        mi_cfg = Gtk.MenuItem(label="Configuración…")
        mi_cfg.connect("activate", self.open_settings)
        menu.append(mi_cfg)
        mi_quit = Gtk.MenuItem(label="Salir")
        mi_quit.connect("activate", self.quit)
        menu.append(mi_quit)
        menu.show_all()
        return menu

    def _build_tray(self):
        self.menu = self._build_menu()
        if HAVE_XAPP:
            icon = XApp.StatusIcon()
            icon.set_icon_name(APP_ICON)
            icon.set_tooltip_text("cursorpop")
            icon.set_primary_menu(self.menu)
            icon.set_secondary_menu(self.menu)
        else:
            icon = Gtk.StatusIcon()
            icon.set_from_icon_name(APP_ICON)
            icon.set_tooltip_text("cursorpop")
            icon.connect("activate", self.open_settings)
            icon.connect("popup-menu", self._gtk_popup)
        self.status_icon = icon

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
    # Ctrl-C cierra limpio si se corre desde terminal.
    GLib.unix_signal_add(GLib.PRIORITY_DEFAULT, signal.SIGINT, _sigint_quit)
    Gtk.main()


if __name__ == "__main__":
    main()
