/* config.c — Defaults and command-line parsing. */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>

#ifndef CURSORPOP_VERSION
#define CURSORPOP_VERSION "0.3.0"
#endif

void config_defaults(Config *c) {
    c->press_enabled       = 1;
    c->press_scale         = 0.80;
    c->press_delay_ms      = 120;   /* clicks shorter than this animate nothing */
    c->press_duration_ms   = 100;
    c->release_duration_ms = 240;
    easing_parse("easeOut",     &c->press_ease);
    easing_parse("easeOutBack", &c->release_ease);   /* bounce on release */

    c->wiggle_enabled      = 1;
    c->grow_scale          = 2.0;
    c->grow_duration_ms    = 250;
    c->grow_hold_ms        = 200;   /* stays big this long after the shake stops */
    c->grow_shrink_ms      = 200;
    easing_parse("easeOut",    &c->grow_ease);
    easing_parse("easeInOut",  &c->grow_shrink_ease);

    c->wiggle_window_ms    = 600;
    c->wiggle_min_distance = 1200.0;
    c->wiggle_min_flips    = 4;
    c->wiggle_min_velocity = 2.0;

    c->fps                 = 60;
}

static void usage(const char *prog) {
    printf(
"cursorpop — animates the X11 cursor: shrinks it on click (Mac style)\n"
"            and grows it when you shake the mouse.\n\n"
"Usage: %s [options]\n\n"
"Click effect:\n"
"  --no-press                 Disable the click effect\n"
"  --press-scale <f>          Size while pressed (default 0.80)\n"
"  --press-delay <ms>         Min hold time to trigger; ignores taps (default 120)\n"
"  --press-duration <ms>      Shrink duration (default 100)\n"
"  --release-duration <ms>    Return-with-bounce duration (default 240)\n"
"  --press-ease <curve>       Shrink easing (default easeOut)\n"
"  --release-ease <curve>     Return easing (default easeOutBack)\n\n"
"Shake effect (grow):\n"
"  --no-wiggle                Disable the shake effect\n"
"  --grow-scale <f>           Max grow factor (default 2.0)\n"
"  --grow-duration <ms>       Grow duration (default 250)\n"
"  --grow-hold <ms>           Stay big this long after the shake stops (default 200)\n"
"  --grow-shrink <ms>         Return duration (default 200)\n"
"  --wiggle-window <ms>       Detection window (default 600)\n"
"  --wiggle-distance <px>     Minimum distance (default 1200)\n"
"  --wiggle-flips <n>         Minimum direction changes (default 4)\n"
"  --wiggle-velocity <px/ms>  Minimum velocity (default 2.0)\n\n"
"General:\n"
"  --fps <n>                  Frames per second (default 60)\n"
"  -h, --help                 This help\n"
"  -v, --version              Version\n\n"
"Easing curves: linear, ease, easeIn, easeOut, easeInOut, easeOutCubic,\n"
"  easeInCubic, easeOutExpo, easeOutBack, easeInOutBack, or custom x1,y1,x2,y2\n",
        prog);
}

enum {
    OPT_NO_PRESS = 256, OPT_PRESS_SCALE, OPT_PRESS_DELAY, OPT_PRESS_DUR, OPT_REL_DUR,
    OPT_PRESS_EASE, OPT_REL_EASE,
    OPT_NO_WIGGLE, OPT_GROW_SCALE, OPT_GROW_DUR, OPT_GROW_HOLD, OPT_GROW_SHRINK,
    OPT_W_WINDOW, OPT_W_DIST, OPT_W_FLIPS, OPT_W_VEL,
    OPT_FPS,
};

int config_parse(Config *c, int argc, char **argv) {
    static const struct option longopts[] = {
        { "no-press",         no_argument,       0, OPT_NO_PRESS },
        { "press-scale",      required_argument, 0, OPT_PRESS_SCALE },
        { "press-delay",      required_argument, 0, OPT_PRESS_DELAY },
        { "press-duration",   required_argument, 0, OPT_PRESS_DUR },
        { "release-duration", required_argument, 0, OPT_REL_DUR },
        { "press-ease",       required_argument, 0, OPT_PRESS_EASE },
        { "release-ease",     required_argument, 0, OPT_REL_EASE },
        { "no-wiggle",        no_argument,       0, OPT_NO_WIGGLE },
        { "grow-scale",       required_argument, 0, OPT_GROW_SCALE },
        { "grow-duration",    required_argument, 0, OPT_GROW_DUR },
        { "grow-hold",        required_argument, 0, OPT_GROW_HOLD },
        { "grow-shrink",      required_argument, 0, OPT_GROW_SHRINK },
        { "wiggle-window",    required_argument, 0, OPT_W_WINDOW },
        { "wiggle-distance",  required_argument, 0, OPT_W_DIST },
        { "wiggle-flips",     required_argument, 0, OPT_W_FLIPS },
        { "wiggle-velocity",  required_argument, 0, OPT_W_VEL },
        { "fps",              required_argument, 0, OPT_FPS },
        { "help",             no_argument,       0, 'h' },
        { "version",          no_argument,       0, 'v' },
        { 0, 0, 0, 0 },
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "hv", longopts, NULL)) != -1) {
        switch (opt) {
        case 'h': usage(argv[0]); return 1;
        case 'v': printf("cursorpop %s\n", CURSORPOP_VERSION); return 1;
        case OPT_NO_PRESS:    c->press_enabled = 0; break;
        case OPT_PRESS_SCALE: c->press_scale = atof(optarg); break;
        case OPT_PRESS_DELAY: c->press_delay_ms = atoi(optarg); break;
        case OPT_PRESS_DUR:   c->press_duration_ms = atoi(optarg); break;
        case OPT_REL_DUR:     c->release_duration_ms = atoi(optarg); break;
        case OPT_PRESS_EASE:
            if (easing_parse(optarg, &c->press_ease)) {
                fprintf(stderr, "Invalid easing: %s\n", optarg); return -1;
            }
            break;
        case OPT_REL_EASE:
            if (easing_parse(optarg, &c->release_ease)) {
                fprintf(stderr, "Invalid easing: %s\n", optarg); return -1;
            }
            break;
        case OPT_NO_WIGGLE:   c->wiggle_enabled = 0; break;
        case OPT_GROW_SCALE:  c->grow_scale = atof(optarg); break;
        case OPT_GROW_DUR:    c->grow_duration_ms = atoi(optarg); break;
        case OPT_GROW_HOLD:   c->grow_hold_ms = atoi(optarg); break;
        case OPT_GROW_SHRINK: c->grow_shrink_ms = atoi(optarg); break;
        case OPT_W_WINDOW:    c->wiggle_window_ms = atoi(optarg); break;
        case OPT_W_DIST:      c->wiggle_min_distance = atof(optarg); break;
        case OPT_W_FLIPS:     c->wiggle_min_flips = atoi(optarg); break;
        case OPT_W_VEL:       c->wiggle_min_velocity = atof(optarg); break;
        case OPT_FPS:
            c->fps = atoi(optarg);
            if (c->fps < 1) c->fps = 1;
            if (c->fps > 240) c->fps = 240;
            break;
        default: return -1;
        }
    }
    return 0;
}

void config_default_path(char *buf, size_t n) {
    const char *xdg  = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");
    if (xdg && *xdg)
        snprintf(buf, n, "%s/cursorpop/cursorpop.conf", xdg);
    else if (home && *home)
        snprintf(buf, n, "%s/.config/cursorpop/cursorpop.conf", home);
    else
        snprintf(buf, n, "cursorpop.conf");
}

static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    if (!*s) return s;
    char *e = s + strlen(s) - 1;
    while (e > s && isspace((unsigned char)*e)) *e-- = '\0';
    return s;
}

static void apply_kv(Config *c, const char *k, const char *v) {
    if      (!strcmp(k, "press_enabled"))     c->press_enabled = atoi(v);
    else if (!strcmp(k, "press_scale"))       c->press_scale = atof(v);
    else if (!strcmp(k, "press_delay"))       c->press_delay_ms = atoi(v);
    else if (!strcmp(k, "press_duration"))    c->press_duration_ms = atoi(v);
    else if (!strcmp(k, "release_duration"))  c->release_duration_ms = atoi(v);
    else if (!strcmp(k, "press_ease"))        easing_parse(v, &c->press_ease);
    else if (!strcmp(k, "release_ease"))      easing_parse(v, &c->release_ease);
    else if (!strcmp(k, "wiggle_enabled"))    c->wiggle_enabled = atoi(v);
    else if (!strcmp(k, "grow_scale"))        c->grow_scale = atof(v);
    else if (!strcmp(k, "grow_duration"))     c->grow_duration_ms = atoi(v);
    else if (!strcmp(k, "grow_hold"))         c->grow_hold_ms = atoi(v);
    else if (!strcmp(k, "grow_shrink"))       c->grow_shrink_ms = atoi(v);
    else if (!strcmp(k, "grow_ease"))         easing_parse(v, &c->grow_ease);
    else if (!strcmp(k, "grow_shrink_ease"))  easing_parse(v, &c->grow_shrink_ease);
    else if (!strcmp(k, "wiggle_window"))     c->wiggle_window_ms = atoi(v);
    else if (!strcmp(k, "wiggle_distance"))   c->wiggle_min_distance = atof(v);
    else if (!strcmp(k, "wiggle_flips"))      c->wiggle_min_flips = atoi(v);
    else if (!strcmp(k, "wiggle_velocity"))   c->wiggle_min_velocity = atof(v);
    else if (!strcmp(k, "fps"))               c->fps = atoi(v);
}

int config_load_file(Config *c, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    char line[256];
    while (fgets(line, sizeof line, f)) {
        char *s = trim(line);
        if (!*s || *s == '#' || *s == ';') continue;
        char *eq = strchr(s, '=');
        if (!eq) continue;
        *eq = '\0';
        apply_kv(c, trim(s), trim(eq + 1));
    }
    fclose(f);
    return 0;
}
