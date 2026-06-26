/* wiggle.c — Shake detection: distance + direction changes + velocity. */
#include "wiggle.h"
#include <math.h>
#include <string.h>

void wiggle_init(Wiggle *w, const Config *c) {
    memset(w, 0, sizeof(*w));
    w->window_ms    = c->wiggle_window_ms;
    w->min_distance = c->wiggle_min_distance;
    w->min_flips    = c->wiggle_min_flips;
    w->min_velocity = c->wiggle_min_velocity;
}

void wiggle_reset(Wiggle *w) {
    w->head = 0;
    w->count = 0;
}

int wiggle_feed(Wiggle *w, long t_ms, double x, double y) {
    /* Store the sample in the circular buffer */
    w->buf[w->head] = (WSample){ t_ms, x, y };
    w->head = (w->head + 1) % WIGGLE_MAX;
    if (w->count < WIGGLE_MAX) w->count++;

    /* Walk the samples within the time window, oldest to newest, accumulating
     * distance, direction changes and velocity. */
    long cutoff = t_ms - w->window_ms;
    double distance = 0.0;
    int flips = 0;
    int last_sign = 0;
    long first_t = t_ms, last_t = t_ms;
    int have_prev = 0;
    double prev_x = 0, prev_y = 0;
    int seen = 0;

    for (int i = 0; i < w->count; i++) {
        int idx = (w->head - w->count + i + WIGGLE_MAX) % WIGGLE_MAX;
        WSample *s = &w->buf[idx];
        if (s->t < cutoff) continue;

        if (!seen) { first_t = s->t; seen = 1; }
        last_t = s->t;

        if (have_prev) {
            double dx = s->x - prev_x;
            double dy = s->y - prev_y;
            distance += hypot(dx, dy);

            /* Direction changes on the X axis (shakes are usually horizontal).
             * Ignore micro-movements to avoid noise. */
            if (fabs(dx) > 1.0) {
                int sign = (dx > 0) ? 1 : -1;
                if (last_sign != 0 && sign != last_sign) flips++;
                last_sign = sign;
            }
        }
        prev_x = s->x; prev_y = s->y; have_prev = 1;
    }

    long duration = last_t - first_t;
    if (duration < 1) duration = 1;
    double velocity = distance / (double)duration;

    if (distance >= w->min_distance &&
        flips    >= w->min_flips &&
        velocity >= w->min_velocity) {
        return 1;
    }
    return 0;
}
