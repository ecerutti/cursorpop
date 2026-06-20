/* capture.c — Captura del cursor vía XFixes y escalado bilineal premultiplicado. */
#include "capture.h"
#include <X11/extensions/Xfixes.h>
#include <stdlib.h>
#include <math.h>

int capture_current_cursor(Display *dpy, CursorImage *out) {
    XFixesCursorImage *ci = XFixesGetCursorImage(dpy);
    if (!ci) return -1;

    size_t n = (size_t)ci->width * ci->height;
    out->pixels = malloc(n * sizeof(uint32_t));
    if (!out->pixels) { XFree(ci); return -1; }

    /* XFixes entrega cada pixel ARGB premultiplicado en los 32 bits bajos
     * de un 'unsigned long' (8 bytes en 64-bit). Lo empaquetamos a uint32. */
    for (size_t i = 0; i < n; i++)
        out->pixels[i] = (uint32_t)ci->pixels[i];

    out->width  = ci->width;
    out->height = ci->height;
    out->xhot   = ci->xhot;
    out->yhot   = ci->yhot;

    XFree(ci);
    return 0;
}

void cursor_image_free(CursorImage *c) {
    if (c && c->pixels) { free(c->pixels); c->pixels = NULL; }
}

static inline uint32_t sample(const CursorImage *s, int x, int y) {
    if (x < 0) x = 0; else if (x >= s->width)  x = s->width  - 1;
    if (y < 0) y = 0; else if (y >= s->height) y = s->height - 1;
    return s->pixels[(size_t)y * s->width + x];
}

/* Interpola un canal (byte) entre 4 muestras. */
static inline int lerp_ch(uint32_t a, uint32_t b, uint32_t c, uint32_t d,
                          int shift, double fx, double fy) {
    double top = ((a >> shift) & 0xFF) * (1 - fx) + ((b >> shift) & 0xFF) * fx;
    double bot = ((c >> shift) & 0xFF) * (1 - fx) + ((d >> shift) & 0xFF) * fx;
    double v = top * (1 - fy) + bot * fy;
    int iv = (int)(v + 0.5);
    return iv < 0 ? 0 : (iv > 255 ? 255 : iv);
}

uint32_t *scale_bilinear(const CursorImage *src, double factor,
                         int *out_w, int *out_h,
                         double *out_xhot, double *out_yhot) {
    int dw = (int)lround(src->width  * factor); if (dw < 1) dw = 1;
    int dh = (int)lround(src->height * factor); if (dh < 1) dh = 1;

    uint32_t *dst = malloc((size_t)dw * dh * sizeof(uint32_t));
    if (!dst) return NULL;

    for (int y = 0; y < dh; y++) {
        double sy = (y + 0.5) / factor - 0.5;
        int y0 = (int)floor(sy);
        double fy = sy - y0;
        for (int x = 0; x < dw; x++) {
            double sx = (x + 0.5) / factor - 0.5;
            int x0 = (int)floor(sx);
            double fx = sx - x0;

            uint32_t c00 = sample(src, x0,     y0);
            uint32_t c10 = sample(src, x0 + 1, y0);
            uint32_t c01 = sample(src, x0,     y0 + 1);
            uint32_t c11 = sample(src, x0 + 1, y0 + 1);

            int a = lerp_ch(c00, c10, c01, c11, 24, fx, fy);
            int r = lerp_ch(c00, c10, c01, c11, 16, fx, fy);
            int g = lerp_ch(c00, c10, c01, c11,  8, fx, fy);
            int b = lerp_ch(c00, c10, c01, c11,  0, fx, fy);
            dst[(size_t)y * dw + x] =
                ((uint32_t)a << 24) | ((uint32_t)r << 16) |
                ((uint32_t)g << 8)  |  (uint32_t)b;
        }
    }

    *out_w = dw; *out_h = dh;
    *out_xhot = src->xhot * factor;
    *out_yhot = src->yhot * factor;
    return dst;
}
