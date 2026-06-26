/* config.h — cursorpop parameters (defaults + CLI parsing). */
#ifndef CURSORPOP_CONFIG_H
#define CURSORPOP_CONFIG_H

#include "easing.h"

typedef struct {
    /* Click effect (Mac style: shrink on press, bounce on release) */
    int    press_enabled;
    double press_scale;          /* size while held down (e.g. 0.80) */
    int    press_delay_ms;       /* min hold time to trigger (ignores taps) */
    int    press_duration_ms;    /* shrink duration */
    int    release_duration_ms;  /* return duration (with bounce) */
    Easing press_ease;
    Easing release_ease;

    /* Grow-on-shake effect ("shake to locate" style) */
    int    wiggle_enabled;
    double grow_scale;           /* max grow factor (e.g. 2.0) */
    int    grow_duration_ms;
    int    grow_hold_ms;
    int    grow_shrink_ms;
    Easing grow_ease;
    Easing grow_shrink_ease;

    /* Shake detection */
    int    wiggle_window_ms;     /* analysis time window */
    double wiggle_min_distance;  /* minimum distance travelled (px) */
    int    wiggle_min_flips;     /* minimum direction changes */
    double wiggle_min_velocity;  /* minimum velocity (px/ms) */

    /* General */
    int    fps;                  /* animation frames per second */
} Config;

#include <stddef.h>

void config_defaults(Config *c);

/* Parse argv. Returns 0 to continue, 1 to exit successfully
 * (--help/--version already printed), -1 on argument error. */
int config_parse(Config *c, int argc, char **argv);

/* Default config file path: ~/.config/cursorpop/cursorpop.conf */
void config_default_path(char *buf, size_t n);

/* Load values from a "key=value" file. Returns 0 if read, -1 if it does not
 * exist (not an error). Missing keys leave the previous value untouched.
 * This is what the GUI writes; the daemon reloads it on SIGHUP. */
int config_load_file(Config *c, const char *path);

#endif
