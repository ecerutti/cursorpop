/* capture.h — Capture the current cursor (XFixes) and bilinear scaling. */
#ifndef CURSORPOP_CAPTURE_H
#define CURSORPOP_CAPTURE_H

#include <X11/Xlib.h>
#include <stdint.h>

typedef struct {
    int width, height;
    int xhot, yhot;
    uint32_t *pixels;   /* premultiplied ARGB, width*height (malloc'd) */
} CursorImage;

/* Capture the cursor currently on screen. Returns 0 on success.
 * Does not depend on the cursor name: it copies the actual pixels. */
int capture_current_cursor(Display *dpy, CursorImage *out);

void cursor_image_free(CursorImage *c);

/* Scale 'src' by 'factor' (bilinear, over premultiplied ARGB).
 * Returns a malloc'd buffer (freed by the caller) and sets the dimensions
 * and the scaled hotspot. Returns NULL on failure. */
uint32_t *scale_bilinear(const CursorImage *src, double factor,
                         int *out_w, int *out_h,
                         double *out_xhot, double *out_yhot);

#endif
