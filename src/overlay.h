/* overlay.h — ARGB override-redirect, click-through window that draws the
 * cursor sprite on top of everything else. */
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

/* Create the overlay window (hidden initially). Returns 0 on success. */
int overlay_init(Overlay *o, Display *dpy);

void overlay_destroy(Overlay *o);

/* Show the ARGB sprite (w x h) so that its hotspot (xhot,yhot) lands exactly
 * on the pointer position (px,py) in screen coordinates. Hides the real
 * cursor and maps the window if needed. */
void overlay_show(Overlay *o, const uint32_t *argb, int w, int h,
                  int px, int py, double xhot, double yhot);

/* Reposition the already-visible overlay (to follow the pointer without
 * redrawing the sprite). */
void overlay_move(Overlay *o, int px, int py, double xhot, double yhot);

/* Hide the overlay and restore the real cursor. */
void overlay_hide(Overlay *o);

#endif
