/* easing.c — Cubic-bezier solver (WebKit UnitBezier style) + presets. */
#include "easing.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/* Solve for t such that bezierX(t) = x, then return bezierY(t).
 * Polynomial coefficients from the control points (P0=0, P3=1). */
static double solve_curve_x(double x, double x1, double x2) {
    const double cx = 3.0 * x1;
    const double bx = 3.0 * (x2 - x1) - cx;
    const double ax = 1.0 - cx - bx;

    /* Newton-Raphson */
    double t = x;
    for (int i = 0; i < 8; i++) {
        double xt = ((ax * t + bx) * t + cx) * t - x;
        if (fabs(xt) < 1e-6) return t;
        double d = (3.0 * ax * t + 2.0 * bx) * t + cx;
        if (fabs(d) < 1e-6) break;
        t -= xt / d;
    }

    /* Bisection as a fallback */
    double lo = 0.0, hi = 1.0;
    t = x;
    if (t < lo) return lo;
    if (t > hi) return hi;
    while (lo < hi) {
        double xt = ((ax * t + bx) * t + cx) * t;
        if (fabs(xt - x) < 1e-6) return t;
        if (x > xt) lo = t; else hi = t;
        t = (lo + hi) * 0.5;
    }
    return t;
}

double easing_eval(const Easing *e, double t) {
    if (t <= 0.0) return 0.0;
    if (t >= 1.0) return 1.0;
    double s = solve_curve_x(t, e->x1, e->x2);
    const double cy = 3.0 * e->y1;
    const double by = 3.0 * (e->y2 - e->y1) - cy;
    const double ay = 1.0 - cy - by;
    return ((ay * s + by) * s + cy) * s;
}

struct preset { const char *name; Easing e; };

static const struct preset PRESETS[] = {
    { "linear",         { 0.0,  0.0,  1.0,  1.0  } },
    { "ease",           { 0.25, 0.1,  0.25, 1.0  } },
    { "easeIn",         { 0.42, 0.0,  1.0,  1.0  } },
    { "easeOut",        { 0.0,  0.0,  0.58, 1.0  } },
    { "easeInOut",      { 0.42, 0.0,  0.58, 1.0  } },
    { "easeOutCubic",   { 0.33, 1.0,  0.68, 1.0  } },
    { "easeInCubic",    { 0.32, 0.0,  0.67, 0.0  } },
    { "easeOutExpo",    { 0.16, 1.0,  0.3,  1.0  } },
    { "easeOutBack",    { 0.34, 1.56, 0.64, 1.0  } },  /* overshoot: bounce */
    { "easeInOutBack",  { 0.68, -0.6, 0.32, 1.6  } },
};

int easing_parse(const char *s, Easing *out) {
    if (!s || !*s) return -1;
    for (size_t i = 0; i < sizeof(PRESETS) / sizeof(PRESETS[0]); i++) {
        if (strcmp(s, PRESETS[i].name) == 0) { *out = PRESETS[i].e; return 0; }
    }
    /* custom curve: x1,y1,x2,y2 */
    double a, b, c, d;
    if (sscanf(s, "%lf,%lf,%lf,%lf", &a, &b, &c, &d) == 4) {
        out->x1 = a; out->y1 = b; out->x2 = c; out->y2 = d;
        return 0;
    }
    return -1;
}
