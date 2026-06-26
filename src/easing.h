/* easing.h — CSS-style cubic-bezier(x1,y1,x2,y2) animation curves. */
#ifndef CURSORPOP_EASING_H
#define CURSORPOP_EASING_H

typedef struct {
    double x1, y1, x2, y2;
} Easing;

/* Evaluate the curve at t ∈ [0,1] and return the eased value (it may go past
 * [0,1] if the curve overshoots, e.g. easeOutBack for the bounce). */
double easing_eval(const Easing *e, double t);

/* Parse a preset by name ("easeOut", "easeOutBack", ...) or a custom curve
 * "x1,y1,x2,y2". Returns 0 on success, -1 if the string is invalid. */
int easing_parse(const char *s, Easing *out);

#endif
