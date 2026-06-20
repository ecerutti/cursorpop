/*
 * spike_clickshrink.c — Validación de viabilidad para "cursorpop"
 *
 * Objetivo: comprobar si en este servidor X (Xorg) se puede achicar el
 * cursor de HARDWARE que está en pantalla, en vivo, sin hacer grab del
 * puntero (sin robar los clicks), usando XFixesChangeCursorByName.
 *
 * Comportamiento:
 *   - Mantené apretado un botón del mouse -> el cursor se achica al 60%.
 *   - Soltá el botón -> vuelve al tamaño original.
 *   - Ctrl-C -> restaura y sale de forma segura.
 *
 * Esto NO es el proyecto final: es un experimento descartable de ~1 archivo
 * para des-arriesgar la arquitectura antes de construir cursorpop en serio.
 *
 * Compilar:
 *   gcc -O2 -Wall spike_clickshrink.c -o spike_clickshrink \
 *       $(pkg-config --cflags --libs x11 xfixes xi xcursor)
 * Correr:
 *   ./spike_clickshrink
 */

#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/XInput2.h>
#include <X11/Xcursor/Xcursor.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/select.h>

#define SHRINK 0.60   /* factor de achique para la prueba */

static volatile sig_atomic_t g_stop = 0;
static void on_sigint(int sig) { (void)sig; g_stop = 1; }

/* Cursor original capturado al apretar; NULL = no hay nada apretado.
 * Nota: XFixesCursorImage incluye el campo ->name cuando XFixes >= 2
 * (acá 6.0), asi que XFixesGetCursorImage ya nos da imagen + nombre. */
static XFixesCursorImage *g_saved = NULL;

/* Construye un Cursor X a partir de una imagen ARGB escalada por 'factor'
 * (nearest-neighbour, suficiente para el spike) y conserva el mismo nombre. */
static Cursor build_scaled_cursor(Display *dpy,
                                  const unsigned long *pixels,
                                  int w, int h, int xhot, int yhot,
                                  double factor)
{
    int nw = (int)(w * factor); if (nw < 1) nw = 1;
    int nh = (int)(h * factor); if (nh < 1) nh = 1;

    XcursorImage *img = XcursorImageCreate(nw, nh);
    if (!img) return None;
    img->xhot = (unsigned)(xhot * factor);
    img->yhot = (unsigned)(yhot * factor);

    for (int y = 0; y < nh; y++) {
        int sy = (int)(y / factor); if (sy >= h) sy = h - 1;
        for (int x = 0; x < nw; x++) {
            int sx = (int)(x / factor); if (sx >= w) sx = w - 1;
            /* XFixes entrega cada pixel ARGB en un unsigned long (32 bits bajos). */
            img->pixels[y * nw + x] = (XcursorPixel)pixels[sy * w + sx];
        }
    }

    Cursor c = XcursorImageLoadCursor(dpy, img);
    XcursorImageDestroy(img);
    return c;
}

/* Construye un Cursor X a partir de una imagen ARGB a tamaño 1:1 (para restaurar). */
static Cursor build_exact_cursor(Display *dpy, const XFixesCursorImage *ci)
{
    XcursorImage *img = XcursorImageCreate(ci->width, ci->height);
    if (!img) return None;
    img->xhot = ci->xhot;
    img->yhot = ci->yhot;
    for (unsigned i = 0; i < (unsigned)ci->width * ci->height; i++)
        img->pixels[i] = (XcursorPixel)ci->pixels[i];
    Cursor c = XcursorImageLoadCursor(dpy, img);
    XcursorImageDestroy(img);
    return c;
}

static void restore_saved(Display *dpy)
{
    if (!g_saved) return;
    if (g_saved->name && g_saved->name[0]) {
        Cursor orig = build_exact_cursor(dpy, g_saved);
        if (orig != None) {
            XFixesChangeCursorByName(dpy, orig, g_saved->name);
            XFlush(dpy);
            XFreeCursor(dpy, orig);
        }
    }
    XFree(g_saved);
    g_saved = NULL;
}

int main(void)
{
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) { fprintf(stderr, "No pude abrir el display X.\n"); return 1; }
    Window root = DefaultRootWindow(dpy);

    /* --- Verificar extensiones --- */
    int xf_ev, xf_err;
    if (!XFixesQueryExtension(dpy, &xf_ev, &xf_err)) {
        fprintf(stderr, "XFixes no disponible.\n"); return 1;
    }
    int xf_major = 0, xf_minor = 0;
    XFixesQueryVersion(dpy, &xf_major, &xf_minor);
    printf("XFixes version: %d.%d (se necesita >= 2.0 para ChangeCursorByName)\n",
           xf_major, xf_minor);

    int xi_opcode, xi_ev, xi_err;
    if (!XQueryExtension(dpy, "XInputExtension", &xi_opcode, &xi_ev, &xi_err)) {
        fprintf(stderr, "XInput no disponible.\n"); return 1;
    }
    int xi_major = 2, xi_minor = 0;
    if (XIQueryVersion(dpy, &xi_major, &xi_minor) != Success) {
        fprintf(stderr, "XInput2 no disponible.\n"); return 1;
    }
    printf("XInput2 version: %d.%d\n", xi_major, xi_minor);

    /* --- Suscribirse a RAW button press/release (no roba eventos) --- */
    unsigned char mask[XIMaskLen(XI_LASTEVENT)];
    memset(mask, 0, sizeof(mask));
    XISetMask(mask, XI_RawButtonPress);
    XISetMask(mask, XI_RawButtonRelease);
    XIEventMask em = { .deviceid = XIAllMasterDevices,
                       .mask_len = sizeof(mask), .mask = mask };
    XISelectEvents(dpy, root, &em, 1);
    XFlush(dpy);

    signal(SIGINT, on_sigint);
    signal(SIGTERM, on_sigint);

    printf("\nListo. Apretá y soltá un botón del mouse sobre cualquier ventana.\n");
    printf("El cursor deberia achicarse al apretar y volver al soltar.\n");
    printf("Ctrl-C para salir.\n\n");

    int fd = ConnectionNumber(dpy);
    while (!g_stop) {
        /* select con timeout para poder atender Ctrl-C aunque no haya eventos. */
        fd_set fds; FD_ZERO(&fds); FD_SET(fd, &fds);
        struct timeval tv = { .tv_sec = 0, .tv_usec = 200000 };
        if (select(fd + 1, &fds, NULL, NULL, &tv) <= 0) continue;

        while (XPending(dpy)) {
            XEvent ev;
            XNextEvent(dpy, &ev);
            if (ev.xcookie.type != GenericEvent ||
                ev.xcookie.extension != xi_opcode) continue;
            if (!XGetEventData(dpy, &ev.xcookie)) continue;

            XIRawEvent *re = ev.xcookie.data;
            if (re->evtype == XI_RawButtonPress && !g_saved) {
                XFixesCursorImage *ci = XFixesGetCursorImage(dpy);
                if (ci) {
                    printf("  press  -> cursor name='%s'  %ux%u  hot=(%u,%u)\n",
                           ci->name ? ci->name : "(sin nombre)",
                           ci->width, ci->height, ci->xhot, ci->yhot);
                    if (ci->name && ci->name[0]) {
                        Cursor small = build_scaled_cursor(
                            dpy, ci->pixels, ci->width, ci->height,
                            ci->xhot, ci->yhot, SHRINK);
                        if (small != None) {
                            XFixesChangeCursorByName(dpy, small, ci->name);
                            XFlush(dpy);
                            XFreeCursor(dpy, small);
                        }
                        g_saved = ci;   /* guardamos el original para restaurar */
                    } else {
                        printf("    (cursor sin nombre: ChangeCursorByName no aplica)\n");
                        XFree(ci);
                    }
                }
            } else if (re->evtype == XI_RawButtonRelease) {
                if (g_saved) printf("  release-> restauro\n");
                restore_saved(dpy);
            }

            XFreeEventData(dpy, &ev.xcookie);
        }
    }

    printf("\nSaliendo, restaurando cursor...\n");
    restore_saved(dpy);
    XCloseDisplay(dpy);
    return 0;
}
