/*
 * cursorpop.c — Daemon que anima el cursor en X11.
 *
 *   • Al apretar un botón del mouse, el cursor se achica (estilo macOS) y al
 *     soltar vuelve con un pequeño rebote.
 *   • Al sacudir el mouse rápido, el cursor se agranda y vuelve.
 *
 * Funciona en cualquier entorno X11 (Cinnamon, GNOME, KDE, Xfce, i3, ...).
 * Técnica: ventana ARGB override-redirect transparente al click que muestra
 * un sprite del cursor; el cursor real se oculta sólo mientras dura el efecto.
 * No hace pointer grab, así que no interfiere con los clicks.
 */
#include "config.h"
#include "capture.h"
#include "overlay.h"
#include "wiggle.h"

#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/XInput2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <sys/select.h>

enum State {
    ST_IDLE,
    ST_PRESS_IN,    /* achicando */
    ST_HELD,        /* mantenido, a press_scale */
    ST_RELEASE_OUT, /* volviendo a 1.0 con rebote */
    ST_GROW_IN,     /* agrandando por sacudida */
    ST_GROW_HELD,   /* agrandado, se mantiene mientras siga la sacudida */
    ST_GROW_OUT,    /* volviendo a 1.0 */
};

typedef struct {
    Display *dpy;
    int      xi_opcode;
    Config   cfg;
    Overlay  overlay;
    Wiggle   wiggle;

    enum State  state;
    CursorImage base;
    int         has_base;

    double scale;          /* escala actual del cursor */

    /* tween en curso */
    double          tw_from, tw_to;
    struct timespec tw_start;
    int             tw_dur_ms;
    Easing          tw_ease;

    int px, py;            /* última posición conocida del puntero */
    int buttons_down;      /* botones de click (no rueda) apretados */
    long grow_keep_until;  /* mantener agrandado hasta este instante (ms) */
    int  press_pending;    /* apretado esperando superar el umbral de tiempo */
    long press_pending_at; /* instante del apretado (ms) */
} Engine;

static volatile sig_atomic_t g_stop = 0;
static volatile sig_atomic_t g_reload = 0;
static void on_sig(int s) { (void)s; g_stop = 1; }
static void on_hup(int s) { (void)s; g_reload = 1; }

/* PID file: deja que la GUI encuentre el daemon para mandarle SIGHUP/SIGTERM. */
static char g_pidfile[512];

static void write_pidfile(void) {
    const char *rt = getenv("XDG_RUNTIME_DIR");
    const char *home = getenv("HOME");
    if (rt && *rt)        snprintf(g_pidfile, sizeof g_pidfile, "%s/cursorpop.pid", rt);
    else if (home && *home) snprintf(g_pidfile, sizeof g_pidfile, "%s/.cursorpop.pid", home);
    else return;
    FILE *f = fopen(g_pidfile, "w");
    if (f) { fprintf(f, "%ld\n", (long)getpid()); fclose(f); }
}

static void remove_pidfile(void) {
    if (g_pidfile[0]) unlink(g_pidfile);
}

/* ---- helpers de tiempo ---- */
static long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}
static long elapsed_ms(struct timespec *start) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec - start->tv_sec) * 1000L +
           (ts.tv_nsec - start->tv_nsec) / 1000000L;
}

/* ---- animación ---- */
static void start_tween(Engine *e, double from, double to, int dur_ms, Easing ease) {
    e->tw_from = from;
    e->tw_to   = to;
    e->tw_dur_ms = dur_ms < 1 ? 1 : dur_ms;
    e->tw_ease = ease;
    clock_gettime(CLOCK_MONOTONIC, &e->tw_start);
    e->scale = from;
}

/* Dibuja el cursor base a la escala actual, posicionado en el puntero. */
static void redraw(Engine *e) {
    if (!e->has_base) return;
    int w, h;
    double xhot, yhot;
    uint32_t *buf = scale_bilinear(&e->base, e->scale, &w, &h, &xhot, &yhot);
    if (!buf) return;
    overlay_show(&e->overlay, buf, w, h, e->px, e->py, xhot, yhot);
    free(buf);
}

/* Reposiciona el overlay ya visible (en movimiento, sin redibujar el sprite). */
static void reposition(Engine *e) {
    if (!e->has_base || !e->overlay.mapped) return;
    overlay_move(&e->overlay, e->px, e->py,
                 e->base.xhot * e->scale, e->base.yhot * e->scale);
}

static void begin_effect(Engine *e) {
    if (e->has_base) cursor_image_free(&e->base);
    if (capture_current_cursor(e->dpy, &e->base) == 0) {
        e->has_base = 1;
    } else {
        e->has_base = 0;
    }
}

static void end_effect(Engine *e) {
    overlay_hide(&e->overlay);
    if (e->has_base) { cursor_image_free(&e->base); e->has_base = 0; }
    e->state = ST_IDLE;
    e->scale = 1.0;
    wiggle_reset(&e->wiggle);   /* evita re-disparos con muestras viejas */
}

/* ---- eventos ---- */
static int is_click_button(int detail) {
    /* Ignora la rueda del mouse (4-7). 1=izq 2=medio 3=der 8/9=laterales. */
    return !(detail >= 4 && detail <= 7);
}

static void on_button_press(Engine *e, int detail) {
    if (!is_click_button(detail)) return;
    e->buttons_down++;
    if (e->buttons_down != 1) return;           /* ya había uno apretado */
    if (!e->cfg.press_enabled) return;

    if (e->state == ST_IDLE) {
        if (e->cfg.press_delay_ms <= 0) {
            /* Modo "todos los clicks": el achique arranca ya. */
            begin_effect(e);
            e->state = ST_PRESS_IN;
            start_tween(e, 1.0, e->cfg.press_scale,
                        e->cfg.press_duration_ms, e->cfg.press_ease);
            redraw(e);
        } else {
            /* Modo "solo al mantener": esperamos a ver si es un tap corto o un
             * apretado deliberado. Arranca al superar press_delay_ms. */
            e->press_pending = 1;
            e->press_pending_at = now_ms();
        }
    } else {
        /* Veníamos de un efecto de sacudida: el click toma el control ya. */
        e->state = ST_PRESS_IN;
        start_tween(e, e->scale, e->cfg.press_scale,
                    e->cfg.press_duration_ms, e->cfg.press_ease);
        redraw(e);
    }
}

static void on_button_release(Engine *e, int detail) {
    if (!is_click_button(detail)) return;
    if (e->buttons_down > 0) e->buttons_down--;
    if (e->buttons_down != 0) return;

    if (e->press_pending) {
        e->press_pending = 0;   /* fue un tap corto: no mostramos nada */
        return;
    }
    if (e->state == ST_PRESS_IN || e->state == ST_HELD) {
        e->state = ST_RELEASE_OUT;
        start_tween(e, e->scale, 1.0,
                    e->cfg.release_duration_ms, e->cfg.release_ease);
    }
}

static void on_motion(Engine *e, int px, int py) {
    e->px = px; e->py = py;

    /* La sacudida sólo actúa cuando no hay un efecto de click en curso. */
    int click_busy = (e->state == ST_PRESS_IN || e->state == ST_HELD ||
                      e->state == ST_RELEASE_OUT) || e->buttons_down > 0;

    if (e->cfg.wiggle_enabled && !click_busy &&
        wiggle_feed(&e->wiggle, now_ms(), px, py)) {
        /* Mientras se siga detectando sacudida, mantenemos el cursor grande. */
        e->grow_keep_until = now_ms() + e->cfg.grow_hold_ms;
        switch (e->state) {
        case ST_IDLE:
            begin_effect(e);
            e->state = ST_GROW_IN;
            start_tween(e, 1.0, e->cfg.grow_scale,
                        e->cfg.grow_duration_ms, e->cfg.grow_ease);
            redraw(e);
            break;
        case ST_GROW_OUT:   /* estaba volviendo: re-agrandar */
            e->state = ST_GROW_IN;
            start_tween(e, e->scale, e->cfg.grow_scale,
                        e->cfg.grow_duration_ms, e->cfg.grow_ease);
            break;
        default:            /* GROW_IN / GROW_HELD: sólo refrescar el timer */
            break;
        }
    }

    if (e->state != ST_IDLE) reposition(e);
}

/* Avanza la animación. Devuelve 1 si hay una animación activa (necesita más
 * cuadros), 0 si está en reposo o sólo esperando un temporizador. */
static int tick(Engine *e) {
    /* ¿El apretado superó el umbral? Recién ahí arranca el achique (los taps
     * cortos se cancelan en on_button_release antes de llegar acá). */
    if (e->press_pending &&
        now_ms() - e->press_pending_at >= e->cfg.press_delay_ms) {
        e->press_pending = 0;
        if (e->buttons_down > 0) {
            begin_effect(e);
            e->state = ST_PRESS_IN;
            start_tween(e, 1.0, e->cfg.press_scale,
                        e->cfg.press_duration_ms, e->cfg.press_ease);
            redraw(e);
        }
    }

    switch (e->state) {
    case ST_PRESS_IN:
    case ST_RELEASE_OUT:
    case ST_GROW_IN:
    case ST_GROW_OUT: {
        double p = (double)elapsed_ms(&e->tw_start) / e->tw_dur_ms;
        if (p > 1.0) p = 1.0;
        e->scale = e->tw_from +
                   (e->tw_to - e->tw_from) * easing_eval(&e->tw_ease, p);
        redraw(e);

        if (p >= 1.0) {
            switch (e->state) {
            case ST_PRESS_IN:
                e->state = ST_HELD;              /* el release pudo cambiarlo */
                break;
            case ST_RELEASE_OUT:
                end_effect(e);
                break;
            case ST_GROW_IN:
                e->state = ST_GROW_HELD;
                break;
            case ST_GROW_OUT:
                end_effect(e);
                break;
            default: break;
            }
        }
        return e->state == ST_PRESS_IN || e->state == ST_RELEASE_OUT ||
               e->state == ST_GROW_IN  || e->state == ST_GROW_OUT;
    }
    case ST_GROW_HELD:
        /* Se queda grande hasta que dejás de sacudir (el timer no se refresca). */
        if (now_ms() >= e->grow_keep_until) {
            e->state = ST_GROW_OUT;
            start_tween(e, e->scale, 1.0,
                        e->cfg.grow_shrink_ms, e->cfg.grow_shrink_ease);
        }
        return 1;   /* sigue necesitando temporizador */
    case ST_HELD:
    case ST_IDLE:
    default:
        return 0;
    }
}

static int setup_xinput(Display *dpy, int *opcode) {
    int ev, err;
    if (!XQueryExtension(dpy, "XInputExtension", opcode, &ev, &err)) return -1;
    int major = 2, minor = 0;
    if (XIQueryVersion(dpy, &major, &minor) != Success) return -1;

    unsigned char mask[XIMaskLen(XI_LASTEVENT)];
    memset(mask, 0, sizeof(mask));
    XISetMask(mask, XI_ButtonPress);
    XISetMask(mask, XI_ButtonRelease);
    XISetMask(mask, XI_Motion);
    XIEventMask em = { .deviceid = XIAllDevices,
                       .mask_len = sizeof(mask), .mask = mask };
    if (XISelectEvents(dpy, DefaultRootWindow(dpy), &em, 1) != Success) return -1;
    XSync(dpy, False);
    return 0;
}

int main(int argc, char **argv) {
    Engine e;
    memset(&e, 0, sizeof(e));

    char cfgpath[512];
    config_default_path(cfgpath, sizeof cfgpath);

    config_defaults(&e.cfg);
    config_load_file(&e.cfg, cfgpath);          /* archivo (lo escribe la GUI) */
    int pr = config_parse(&e.cfg, argc, argv);  /* los flags CLI tienen prioridad */
    if (pr != 0) return pr < 0 ? 2 : 0;

    e.dpy = XOpenDisplay(NULL);
    if (!e.dpy) { fprintf(stderr, "cursorpop: no pude abrir el display X.\n"); return 1; }

    int xf_ev, xf_err;
    if (!XFixesQueryExtension(e.dpy, &xf_ev, &xf_err)) {
        fprintf(stderr, "cursorpop: se requiere la extensión XFixes.\n"); return 1;
    }
    if (setup_xinput(e.dpy, &e.xi_opcode) != 0) {
        fprintf(stderr, "cursorpop: se requiere XInput2.\n"); return 1;
    }
    if (overlay_init(&e.overlay, e.dpy) != 0) {
        fprintf(stderr, "cursorpop: no hay un visual ARGB de 32 bits (¿compositor activo?).\n");
        return 1;
    }
    wiggle_init(&e.wiggle, &e.cfg);
    e.state = ST_IDLE;
    e.scale = 1.0;

    signal(SIGINT, on_sig);
    signal(SIGTERM, on_sig);
    signal(SIGHUP, on_hup);
    write_pidfile();

    int fd = ConnectionNumber(e.dpy);
    int frame_ms = 1000 / e.cfg.fps;
    if (frame_ms < 1) frame_ms = 1;

    while (!g_stop) {
        if (g_reload) {
            g_reload = 0;
            Config nc;
            config_defaults(&nc);
            config_load_file(&nc, cfgpath);
            e.cfg = nc;
            wiggle_init(&e.wiggle, &e.cfg);
            frame_ms = 1000 / e.cfg.fps;
            if (frame_ms < 1) frame_ms = 1;
        }

        /* Necesitamos timer de cuadro cuando hay animación, cuando estamos
         * agrandados esperando (GROW_HELD) o cuando un press está pendiente. */
        int busy = (e.state != ST_IDLE && e.state != ST_HELD) || e.press_pending;

        fd_set fds; FD_ZERO(&fds); FD_SET(fd, &fds);
        struct timeval tv;
        if (busy) { tv.tv_sec = 0; tv.tv_usec = frame_ms * 1000; }
        else      { tv.tv_sec = 0; tv.tv_usec = 200 * 1000; }

        int r = select(fd + 1, &fds, NULL, NULL, &tv);

        if (r > 0) {
            while (XPending(e.dpy)) {
                XEvent ev;
                XNextEvent(e.dpy, &ev);
                if (ev.xcookie.type != GenericEvent ||
                    ev.xcookie.extension != e.xi_opcode) continue;
                if (!XGetEventData(e.dpy, &ev.xcookie)) continue;

                XIDeviceEvent *de = ev.xcookie.data;
                switch (de->evtype) {
                case XI_ButtonPress:
                    on_motion(&e, (int)de->root_x, (int)de->root_y);
                    on_button_press(&e, de->detail);
                    break;
                case XI_ButtonRelease:
                    on_button_release(&e, de->detail);
                    break;
                case XI_Motion:
                    on_motion(&e, (int)de->root_x, (int)de->root_y);
                    break;
                }
                XFreeEventData(e.dpy, &ev.xcookie);
            }
        }

        tick(&e);
    }

    end_effect(&e);
    overlay_destroy(&e.overlay);
    XCloseDisplay(e.dpy);
    remove_pidfile();
    return 0;
}
