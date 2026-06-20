/* overlay.h — Ventana ARGB override-redirect, transparente al click, que
 * dibuja el sprite del cursor por encima de todo. */
#ifndef CURSORPOP_OVERLAY_H
#define CURSORPOP_OVERLAY_H

#include <X11/Xlib.h>
#include <stdint.h>

typedef struct {
    Display *dpy;
    Window   root;
    Window   win;
    GC       gc;
    Visual  *visual;
    int      depth;
    int      mapped;
    int      cursor_hidden;
} Overlay;

/* Crea la ventana overlay (oculta al inicio). Devuelve 0 si OK. */
int overlay_init(Overlay *o, Display *dpy);

void overlay_destroy(Overlay *o);

/* Muestra el sprite ARGB (w x h) de modo que su hotspot (xhot,yhot) quede
 * exactamente en el punto del puntero (px,py) en coordenadas de pantalla.
 * Oculta el cursor real y mapea la ventana si hace falta. */
void overlay_show(Overlay *o, const uint32_t *argb, int w, int h,
                  int px, int py, double xhot, double yhot);

/* Reposiciona el overlay ya visible (para seguir al puntero sin redibujar). */
void overlay_move(Overlay *o, int px, int py, double xhot, double yhot);

/* Oculta el overlay y restaura el cursor real. */
void overlay_hide(Overlay *o);

#endif
