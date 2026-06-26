/* overlay.c — Implementation of the click-through ARGB overlay window. */
#include "overlay.h"
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xfixes.h>
#include <string.h>

int overlay_init(Overlay *o, Display *dpy) {
    memset(o, 0, sizeof(*o));
    o->dpy  = dpy;
    o->root = DefaultRootWindow(dpy);
    int screen = DefaultScreen(dpy);

    XVisualInfo vinfo;
    if (!XMatchVisualInfo(dpy, screen, 32, TrueColor, &vinfo))
        return -1;
    o->visual = vinfo.visual;
    o->depth  = vinfo.depth;

    XSetWindowAttributes attrs;
    memset(&attrs, 0, sizeof(attrs));
    attrs.colormap          = XCreateColormap(dpy, o->root, vinfo.visual, AllocNone);
    attrs.background_pixel   = 0;
    attrs.border_pixel       = 0;
    attrs.override_redirect  = True;

    o->win = XCreateWindow(
        dpy, o->root, 0, 0, 1, 1, 0, vinfo.depth, InputOutput, vinfo.visual,
        CWColormap | CWBackPixel | CWBorderPixel | CWOverrideRedirect, &attrs);
    if (!o->win) return -1;

    /* We don't want the WM to manage it */
    XClassHint ch = { (char *)"cursorpop", (char *)"cursorpop" };
    XSetClassHint(dpy, o->win, &ch);

    /* Click-through: empty input region */
    XShapeCombineRectangles(dpy, o->win, ShapeInput, 0, 0, NULL, 0,
                            ShapeSet, Unsorted);

    o->gc = XCreateGC(dpy, o->win, 0, NULL);
    return 0;
}

void overlay_destroy(Overlay *o) {
    if (!o->dpy) return;
    if (o->cursor_hidden) { XFixesShowCursor(o->dpy, o->root); o->cursor_hidden = 0; }
    if (o->gc)  XFreeGC(o->dpy, o->gc);
    if (o->win) XDestroyWindow(o->dpy, o->win);
    XFlush(o->dpy);
}

void overlay_move(Overlay *o, int px, int py, double xhot, double yhot) {
    XMoveWindow(o->dpy, o->win, px - (int)(xhot + 0.5), py - (int)(yhot + 0.5));
    XRaiseWindow(o->dpy, o->win);
    XFlush(o->dpy);
}

void overlay_show(Overlay *o, const uint32_t *argb, int w, int h,
                  int px, int py, double xhot, double yhot) {
    XResizeWindow(o->dpy, o->win, w, h);
    XMoveWindow(o->dpy, o->win, px - (int)(xhot + 0.5), py - (int)(yhot + 0.5));

    if (!o->mapped) {
        XMapRaised(o->dpy, o->win);
        o->mapped = 1;
    } else {
        XRaiseWindow(o->dpy, o->win);
    }

    /* XCreateImage wraps the buffer without copying it; we set data=NULL before
     * XDestroyImage so we don't free memory that belongs to the caller. */
    XImage *img = XCreateImage(o->dpy, o->visual, 32, ZPixmap, 0,
                               (char *)argb, w, h, 32, 0);
    if (img) {
        XClearWindow(o->dpy, o->win);
        XPutImage(o->dpy, o->win, o->gc, img, 0, 0, 0, 0, w, h);
        img->data = NULL;
        XDestroyImage(img);
    }

    /* Hiding the real cursor AFTER drawing avoids a flicker where nothing would
     * be shown; the brief overlap (real cursor + sprite at scale ~1) is
     * invisible because they coincide. */
    if (!o->cursor_hidden) {
        XFixesHideCursor(o->dpy, o->root);
        o->cursor_hidden = 1;
    }
    XFlush(o->dpy);
}

void overlay_hide(Overlay *o) {
    /* Show the real cursor BEFORE unmapping, for the same reason. */
    if (o->cursor_hidden) {
        XFixesShowCursor(o->dpy, o->root);
        o->cursor_hidden = 0;
    }
    if (o->mapped) {
        XUnmapWindow(o->dpy, o->win);
        o->mapped = 0;
    }
    XFlush(o->dpy);
}
