/*
 * spike2_overlay.c — Validation v2 for "cursorpop" (OVERLAY approach)
 *
 * Based on the proven wiggle-grow solution (window_mode): instead of trying to
 * change the hardware cursor (which steals clicks or does not re-render), we
 * draw an ARGB override-redirect, click-through window that shows a cursor
 * sprite. The real cursor is hidden ONLY while the effect is active, and is
 * restored on release.
 *
 * Key difference from the previous spike: it uses NORMAL XInput2 events
 * (XI_ButtonPress/Release/Motion) with deviceid=XIAllDevices on the root,
 * which is EXACTLY how wiggle-grow detects input (tested on real machines).
 * The previous spike used RAW events, which may not have been delivered.
 *
 * Test behaviour:
 *   - Hold a button down -> the shrunken cursor appears (at 55%).
 *   - Move with the button held -> the sprite follows the pointer.
 *   - Release -> the normal real cursor returns.
 *   - Prints a "heartbeat" every second with how many events it received,
 *     to diagnose whether click detection works.
 *   - Ctrl-C -> restores and exits.
 *
 * Build:
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

#define SHRINK 0.55   /* shrink factor for the test */

static volatile sig_atomic_t g_stop = 0;
static void on_sig(int s) { (void)s; g_stop = 1; }

/* Overlay state */
static Display *dpy;
static Window   root, win;
static GC       gc;
static XVisualInfo vinfo;
static int      pressed = 0;
static int      cursor_hidden = 0;
static int      cur_xhot = 0, cur_yhot = 0;   /* hotspot of the current sprite */

/* Hide/show the real cursor, with guards so we don't unbalance the server's
 * internal counter (each Hide needs its Show). */
static void hide_real_cursor(void) {
    if (!cursor_hidden) { XFixesHideCursor(dpy, root); cursor_hidden = 1; }
}
static void show_real_cursor(void) {
    if (cursor_hidden) { XFixesShowCursor(dpy, root); cursor_hidden = 0; }
}

/* Build an ARGB XImage with the current cursor scaled by 'factor'.
 * Returns the image (must be XDestroyImage'd) and sets *out_w/h/xhot/yhot. */
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
            /* XFixes delivers premultiplied ARGB in the low 32 bits of each long */
            buf[y * sw + x] = (uint32_t)ci->pixels[syi * ci->width + sxi];
        }
    }
    printf("  [captured cursor '%s' %ux%u -> sprite %dx%d]\n",
           ci->name ? ci->name : "(unnamed)", ci->width, ci->height, sw, sh);
    XFree(ci);

    /* XCreateImage takes ownership of 'buf'; XDestroyImage frees it. */
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
    XDestroyImage(img);   /* frees img and its buffer */
}

static void hide_overlay(void) {
    XUnmapWindow(dpy, win);
    show_real_cursor();
    XFlush(dpy);
}

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    dpy = XOpenDisplay(NULL);
    if (!dpy) { fprintf(stderr, "Could not open the display.\n"); return 1; }
    root = DefaultRootWindow(dpy);
    int screen = DefaultScreen(dpy);

    int xf_ev, xf_err;
    if (!XFixesQueryExtension(dpy, &xf_ev, &xf_err)) {
        fprintf(stderr, "XFixes not available.\n"); return 1;
    }
    int shape_ev, shape_err;
    if (!XShapeQueryExtension(dpy, &shape_ev, &shape_err)) {
        fprintf(stderr, "XShape not available.\n"); return 1;
    }
    int xi_opcode, xi_ev, xi_err;
    if (!XQueryExtension(dpy, "XInputExtension", &xi_opcode, &xi_ev, &xi_err)) {
        fprintf(stderr, "XInput not available.\n"); return 1;
    }
    int ximajor = 2, ximinor = 0;
    if (XIQueryVersion(dpy, &ximajor, &ximinor) != Success) {
        fprintf(stderr, "XInput2 not available.\n"); return 1;
    }
    printf("XInput2 %d.%d / XFixes OK / XShape OK\n", ximajor, ximinor);

    /* 32-bit ARGB visual for the overlay */
    if (!XMatchVisualInfo(dpy, screen, 32, TrueColor, &vinfo)) {
        fprintf(stderr, "No 32-bit ARGB visual.\n"); return 1;
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

    /* Click-through window: empty input region */
    XShapeCombineRectangles(dpy, win, ShapeInput, 0, 0, NULL, 0, ShapeSet, Unsorted);

    gc = XCreateGC(dpy, win, 0, NULL);

    /* --- Event selection: SAME as wiggle-grow (tested) ---
     * NORMAL XI2 events (not raw), deviceid = XIAllDevices, on the root. */
    unsigned char mask[XIMaskLen(XI_LASTEVENT)];
    memset(mask, 0, sizeof(mask));
    XISetMask(mask, XI_ButtonPress);
    XISetMask(mask, XI_ButtonRelease);
    XISetMask(mask, XI_Motion);
    XIEventMask em = { .deviceid = XIAllDevices,
                       .mask_len = sizeof(mask), .mask = mask };
    if (XISelectEvents(dpy, root, &em, 1) != Success) {
        fprintf(stderr, "XISelectEvents failed.\n"); return 1;
    }
    XSync(dpy, False);

    signal(SIGINT, on_sig);
    signal(SIGTERM, on_sig);

    printf("\nReady. Press and release a mouse button over any window.\n");
    printf("You should see the shrunken cursor while holding the button.\n");
    printf("Ctrl-C to exit.\n\n");

    long n_press = 0, n_release = 0, n_motion = 0;
    int fd = ConnectionNumber(dpy);

    while (!g_stop) {
        fd_set fds; FD_ZERO(&fds); FD_SET(fd, &fds);
        struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
        int r = select(fd + 1, &fds, NULL, NULL, &tv);

        if (r == 0) {
            /* diagnostic heartbeat */
            printf("  ...alive (press=%ld release=%ld motion=%ld)\n",
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
                    printf("PRESS  button=%d at (%d,%d)\n", de->detail, px, py);
                    show_overlay_at(px, py, SHRINK);
                }
                break;
            case XI_ButtonRelease:
                n_release++;
                if (pressed) {
                    pressed = 0;
                    printf("RELEASE button=%d\n", de->detail);
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

    printf("\nExiting, restoring cursor...\n");
    if (pressed) hide_overlay();
    show_real_cursor();   /* just in case */
    XFlush(dpy);
    XCloseDisplay(dpy);
    return 0;
}
