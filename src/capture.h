/* capture.h — Captura del cursor actual (XFixes) y escalado bilineal. */
#ifndef CURSORPOP_CAPTURE_H
#define CURSORPOP_CAPTURE_H

#include <X11/Xlib.h>
#include <stdint.h>

typedef struct {
    int width, height;
    int xhot, yhot;
    uint32_t *pixels;   /* ARGB premultiplicado, width*height (malloc) */
} CursorImage;

/* Captura el cursor que está en pantalla ahora mismo. Devuelve 0 si OK.
 * No depende del nombre del cursor: copia los píxeles reales. */
int capture_current_cursor(Display *dpy, CursorImage *out);

void cursor_image_free(CursorImage *c);

/* Escala 'src' por 'factor' (bilineal, sobre ARGB premultiplicado).
 * Devuelve un buffer malloc (que libera el llamador) y setea las dimensiones
 * y el hotspot escalado. Devuelve NULL si falla. */
uint32_t *scale_bilinear(const CursorImage *src, double factor,
                         int *out_w, int *out_h,
                         double *out_xhot, double *out_yhot);

#endif
