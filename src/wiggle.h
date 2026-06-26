/* wiggle.h — Mouse shake detector ("shake to locate" style). */
#ifndef CURSORPOP_WIGGLE_H
#define CURSORPOP_WIGGLE_H

#include "config.h"

#define WIGGLE_MAX 512

typedef struct { long t; double x, y; } WSample;

typedef struct {
    int    window_ms;
    double min_distance;
    int    min_flips;
    double min_velocity;
    WSample buf[WIGGLE_MAX];
    int    head;   /* index of the next write */
    int    count;
} Wiggle;

void wiggle_init(Wiggle *w, const Config *c);

/* Feed a motion sample (t in ms, position x,y).
 * Returns 1 if a shake is detected at this moment. */
int  wiggle_feed(Wiggle *w, long t_ms, double x, double y);

void wiggle_reset(Wiggle *w);

#endif
