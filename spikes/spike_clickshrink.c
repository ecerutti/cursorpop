/*
 * spike_clickshrink.c — Feasibility validation for "cursorpop"
 *
 * Goal: check whether on this X server (Xorg) we can shrink the HARDWARE cursor
 * currently on screen, live, without grabbing the pointer (without stealing
 * clicks), using XFixesChangeCursorByName.
 *
 * Behaviour:
 *   - Hold a mouse button down -> the cursor shrinks to 60%.
 *   - Release the button -> it returns to its original size.
 *   - Ctrl-C -> restores and exits safely.
 *
 * This is NOT the final project: it is a throwaway ~1-file experiment to
 * de-risk the architecture before building cursorpop for real.
 *
 * Build:
 *   gcc -O2 -Wall spike_clickshrink.c -o spike_clickshrink \
 *       $(pkg-config --cflags --libs x11 xfixes xi xcursor)
 * Run:
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

#define SHRINK 0.60   /* shrink factor for the test */

static volatile sig_atomic_t g_stop = 0;
static void on_sigint(int sig) { (void)sig; g_stop = 1; }

/* Original cursor captured on press; NULL = nothing is pressed.
 * Note: XFixesCursorImage includes the ->name field when XFixes >= 2
 * (here 6.0), so XFixesGetCursorImage already gives us image + name. */
static XFixesCursorImage *g_saved = NULL;

/* Build an X Cursor from an ARGB image scaled by 'factor'
 * (nearest-neighbour, good enough for the spike), keeping the same name. */
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
            /* XFixes delivers each ARGB pixel in an unsigned long (low 32 bits). */
            img->pixels[y * nw + x] = (XcursorPixel)pixels[sy * w + sx];
        }
    }

    Cursor c = XcursorImageLoadCursor(dpy, img);
    XcursorImageDestroy(img);
    return c;
}

/* Build an X Cursor from a 1:1 ARGB image (to restore). */
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
    if (!dpy) { fprintf(stderr, "Could not open the X display.\n"); return 1; }
    Window root = DefaultRootWindow(dpy);

    /* --- Check extensions --- */
    int xf_ev, xf_err;
    if (!XFixesQueryExtension(dpy, &xf_ev, &xf_err)) {
        fprintf(stderr, "XFixes not available.\n"); return 1;
    }
    int xf_major = 0, xf_minor = 0;
    XFixesQueryVersion(dpy, &xf_major, &xf_minor);
    printf("XFixes version: %d.%d (need >= 2.0 for ChangeCursorByName)\n",
           xf_major, xf_minor);

    int xi_opcode, xi_ev, xi_err;
    if (!XQueryExtension(dpy, "XInputExtension", &xi_opcode, &xi_ev, &xi_err)) {
        fprintf(stderr, "XInput not available.\n"); return 1;
    }
    int xi_major = 2, xi_minor = 0;
    if (XIQueryVersion(dpy, &xi_major, &xi_minor) != Success) {
        fprintf(stderr, "XInput2 not available.\n"); return 1;
    }
    printf("XInput2 version: %d.%d\n", xi_major, xi_minor);

    /* --- Subscribe to RAW button press/release (does not steal events) --- */
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

    printf("\nReady. Press and release a mouse button over any window.\n");
    printf("The cursor should shrink on press and return on release.\n");
    printf("Ctrl-C to exit.\n\n");

    int fd = ConnectionNumber(dpy);
    while (!g_stop) {
        /* select with timeout so we can handle Ctrl-C even with no events. */
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
                           ci->name ? ci->name : "(unnamed)",
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
                        g_saved = ci;   /* keep the original to restore it */
                    } else {
                        printf("    (unnamed cursor: ChangeCursorByName does not apply)\n");
                        XFree(ci);
                    }
                }
            } else if (re->evtype == XI_RawButtonRelease) {
                if (g_saved) printf("  release-> restoring\n");
                restore_saved(dpy);
            }

            XFreeEventData(dpy, &ev.xcookie);
        }
    }

    printf("\nExiting, restoring cursor...\n");
    restore_saved(dpy);
    XCloseDisplay(dpy);
    return 0;
}
