/*
 * spike2_overlay.c — Validación v2 para "cursorpop" (enfoque OVERLAY)
 *
 * Basado en la solución probada de wiggle-grow (window_mode): en vez de
 * intentar cambiar el cursor de hardware (que roba clicks o no re-renderiza),
 * dibujamos una ventana ARGB override-redirect, transparente al click, que
 * muestra un sprite del cursor. El cursor real se oculta SOLO mientras el
 * efecto está activo, y se restaura al soltar.
 *
 * Diferencia clave con el spike anterior: usa eventos XInput2 NORMALES
 * (XI_ButtonPress/Release/Motion) con deviceid=XIAllDevices sobre la root,
 * que es EXACTAMENTE como wiggle-grow detecta input (probado en máquinas
 * reales). El spike anterior usaba RAW events, que quizá no se entregaban.
 *
 * Comportamiento de la prueba:
 *   - Mantené apretado un botón -> aparece el cursor achicado (al 55%).
 *   - Movés con el botón apretado -> el sprite sigue al puntero.
 *   - Soltás -> vuelve el cursor real normal.
 *   - Imprime un "latido" cada segundo con cuántos eventos recibió,
 *     para diagnosticar si la detección de clicks funciona.
 *   - Ctrl-C -> restaura y sale.
 *
 * Compilar:
 *   gcc -O2 -Wall spike2_overlay.c -o spike2_overlay \
 *       $(pkg-config --cflags --libs x11 xfixes xi xcursor xext)
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/shape.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <sys/select.h>

#define SHRINK 0.55   /* factor de achique para la prueba */

static volatile sig_atomic_t g_stop = 0;
static void on_sig(int s) { (void)s; g_stop = 1; }

/* Estado del overlay */
static Display *dpy;
static Window   root, win;
static GC       gc;
static XVisualInfo vinfo;
static int      pressed = 0;
static int      cursor_hidden = 0;
static int      cur_xhot = 0, cur_yhot = 0;   /* hotspot del sprite actual */

/* Oculta/muestra el cursor real, con guardas para no descompensar el contador
 * interno del servidor (cada Hide necesita su Show). */
static void hide_real_cursor(void) {
    if (!cursor_hidden) { XFixesHideCursor(dpy, root); cursor_hidden = 1; }
}
static void show_real_cursor(void) {
    if (cursor_hidden) { XFixesShowCursor(dpy, root); cursor_hidden = 0; }
}

/* Construye una XImage ARGB con el cursor actual escalado por 'factor'.
 * Devuelve la imagen (hay que XDestroyImage-arla) y setea *out_w/h/xhot/yhot. */
static XImage *make_scaled_image(double factor,
                                 int *out_w, int *out_h,
                                 int *out_xhot, int *out_yhot) {
    XFixesCursorImage *ci = XFixesGetCursorImage(dpy);
    if (!ci) return NULL;

    int sw = (int)(ci->width  * factor); if (sw < 1) sw = 1;
    int sh = (int)(ci->height * factor); if (sh < 1) sh = 1;
    *out_xhot = (int)(ci->xhot * factor);
    *out_yhot = (int)(ci->yhot * factor);
    *out_w = sw; *out_h = sh;

    uint32_t *buf = malloc((size_t)sw * sh * 4);
    if (!buf) { XFree(ci); return NULL; }
    for (int y = 0; y < sh; y++) {
        int syi = (int)(y / factor); if (syi >= ci->height) syi = ci->height - 1;
        for (int x = 0; x < sw; x++) {
            int sxi = (int)(x / factor); if (sxi >= ci->width) sxi = ci->width - 1;
            /* XFixes entrega ARGB premultiplicado en los 32 bits bajos de cada long */
            buf[y * sw + x] = (uint32_t)ci->pixels[syi * ci->width + sxi];
        }
    }
    printf("  [capturado cursor '%s' %ux%u -> sprite %dx%d]\n",
           ci->name ? ci->name : "(sin nombre)", ci->width, ci->height, sw, sh);
    XFree(ci);

    /* XCreateImage toma posesión de 'buf'; XDestroyImage lo libera. */
    XImage *img = XCreateImage(dpy, vinfo.visual, 32, ZPixmap, 0,
                               (char *)buf, sw, sh, 32, 0);
    if (!img) { free(buf); return NULL; }
    return img;
}

static void show_overlay_at(int px, int py, double factor) {
    int sw, sh, xhot, yhot;
    XImage *img = make_scaled_image(factor, &sw, &sh, &xhot, &yhot);
    if (!img) return;
    cur_xhot = xhot; cur_yhot = yhot;

    XResizeWindow(dpy, win, sw, sh);
    XMoveWindow(dpy, win, px - xhot, py - yhot);
    hide_real_cursor();
    XMapRaised(dpy, win);
    XClearWindow(dpy, win);
    XPutImage(dpy, win, gc, img, 0, 0, 0, 0, sw, sh);
    XFlush(dpy);
    XDestroyImage(img);   /* libera img y su buffer */
}

static void hide_overlay(void) {
    XUnmapWindow(dpy, win);
    show_real_cursor();
    XFlush(dpy);
}

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    dpy = XOpenDisplay(NULL);
    if (!dpy) { fprintf(stderr, "No pude abrir el display.\n"); return 1; }
    root = DefaultRootWindow(dpy);
    int screen = DefaultScreen(dpy);

    int xf_ev, xf_err;
    if (!XFixesQueryExtension(dpy, &xf_ev, &xf_err)) {
        fprintf(stderr, "XFixes no disponible.\n"); return 1;
    }
    int shape_ev, shape_err;
    if (!XShapeQueryExtension(dpy, &shape_ev, &shape_err)) {
        fprintf(stderr, "XShape no disponible.\n"); return 1;
    }
    int xi_opcode, xi_ev, xi_err;
    if (!XQueryExtension(dpy, "XInputExtension", &xi_opcode, &xi_ev, &xi_err)) {
        fprintf(stderr, "XInput no disponible.\n"); return 1;
    }
    int ximajor = 2, ximinor = 0;
    if (XIQueryVersion(dpy, &ximajor, &ximinor) != Success) {
        fprintf(stderr, "XInput2 no disponible.\n"); return 1;
    }
    printf("XInput2 %d.%d / XFixes OK / XShape OK\n", ximajor, ximinor);

    /* Visual ARGB de 32 bits para el overlay */
    if (!XMatchVisualInfo(dpy, screen, 32, TrueColor, &vinfo)) {
        fprintf(stderr, "No hay visual ARGB de 32 bits.\n"); return 1;
    }

    XSetWindowAttributes attrs;
    memset(&attrs, 0, sizeof(attrs));
    attrs.colormap = XCreateColormap(dpy, root, vinfo.visual, AllocNone);
    attrs.background_pixel = 0;
    attrs.border_pixel = 0;
    attrs.override_redirect = True;
    win = XCreateWindow(dpy, root, 0, 0, 64, 64, 0, vinfo.depth, InputOutput,
                        vinfo.visual,
                        CWColormap | CWBackPixel | CWBorderPixel | CWOverrideRedirect,
                        &attrs);

    /* Ventana transparente al click: región de input vacía */
    XShapeCombineRectangles(dpy, win, ShapeInput, 0, 0, NULL, 0, ShapeSet, Unsorted);

    gc = XCreateGC(dpy, win, 0, NULL);

    /* --- Selección de eventos: IGUAL que wiggle-grow (probado) ---
     * Eventos XI2 NORMALES (no raw), deviceid = XIAllDevices, sobre root. */
    unsigned char mask[XIMaskLen(XI_LASTEVENT)];
    memset(mask, 0, sizeof(mask));
    XISetMask(mask, XI_ButtonPress);
    XISetMask(mask, XI_ButtonRelease);
    XISetMask(mask, XI_Motion);
    XIEventMask em = { .deviceid = XIAllDevices,
                       .mask_len = sizeof(mask), .mask = mask };
    if (XISelectEvents(dpy, root, &em, 1) != Success) {
        fprintf(stderr, "XISelectEvents fallo.\n"); return 1;
    }
    XSync(dpy, False);

    signal(SIGINT, on_sig);
    signal(SIGTERM, on_sig);

    printf("\nListo. Apreta y solta un boton del mouse sobre cualquier ventana.\n");
    printf("Deberias ver el cursor achicado mientras manten apretado.\n");
    printf("Ctrl-C para salir.\n\n");

    long n_press = 0, n_release = 0, n_motion = 0;
    int fd = ConnectionNumber(dpy);

    while (!g_stop) {
        fd_set fds; FD_ZERO(&fds); FD_SET(fd, &fds);
        struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
        int r = select(fd + 1, &fds, NULL, NULL, &tv);

        if (r == 0) {
            /* latido de diagnóstico */
            printf("  ...vivo (press=%ld release=%ld motion=%ld)\n",
                   n_press, n_release, n_motion);
            continue;
        }
        if (r < 0) continue;

        while (XPending(dpy)) {
            XEvent ev;
            XNextEvent(dpy, &ev);
            if (ev.xcookie.type != GenericEvent ||
                ev.xcookie.extension != xi_opcode) continue;
            if (!XGetEventData(dpy, &ev.xcookie)) continue;

            XIDeviceEvent *de = ev.xcookie.data;
            int px = (int)de->root_x, py = (int)de->root_y;

            switch (de->evtype) {
            case XI_ButtonPress:
                n_press++;
                if (!pressed) {
                    pressed = 1;
                    printf("PRESS  boton=%d en (%d,%d)\n", de->detail, px, py);
                    show_overlay_at(px, py, SHRINK);
                }
                break;
            case XI_ButtonRelease:
                n_release++;
                if (pressed) {
                    pressed = 0;
                    printf("RELEASE boton=%d\n", de->detail);
                    hide_overlay();
                }
                break;
            case XI_Motion:
                n_motion++;
                if (pressed) {
                    XMoveWindow(dpy, win, px - cur_xhot, py - cur_yhot);
                    XRaiseWindow(dpy, win);
                    XFlush(dpy);
                }
                break;
            }
            XFreeEventData(dpy, &ev.xcookie);
        }
    }

    printf("\nSaliendo, restaurando cursor...\n");
    if (pressed) hide_overlay();
    show_real_cursor();   /* por las dudas */
    XFlush(dpy);
    XCloseDisplay(dpy);
    return 0;
}
