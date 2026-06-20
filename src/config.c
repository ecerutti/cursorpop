/* config.c — Defaults y parseo de línea de comandos. */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>

#ifndef CURSORPOP_VERSION
#define CURSORPOP_VERSION "0.1.0"
#endif

void config_defaults(Config *c) {
    c->press_enabled       = 1;
    c->press_scale         = 0.80;
    c->press_delay_ms      = 120;   /* clicks más cortos que esto no animan nada */
    c->press_duration_ms   = 100;
    c->release_duration_ms = 240;
    easing_parse("easeOut",     &c->press_ease);
    easing_parse("easeOutBack", &c->release_ease);   /* rebote al soltar */

    c->wiggle_enabled      = 1;
    c->grow_scale          = 2.0;
    c->grow_duration_ms    = 250;
    c->grow_hold_ms        = 200;   /* sigue grande este tiempo tras parar de sacudir */
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
"cursorpop — anima el cursor en X11: lo achica al hacer click (estilo Mac)\n"
"            y lo agranda al sacudir el mouse.\n\n"
"Uso: %s [opciones]\n\n"
"Efecto de click:\n"
"  --no-press                 Desactiva el efecto de click\n"
"  --press-scale <f>          Tamaño al apretar (def. 0.80)\n"
"  --press-delay <ms>         Mínimo apretado para disparar; ignora taps (def. 120)\n"
"  --press-duration <ms>      Duración del achique (def. 100)\n"
"  --release-duration <ms>    Duración del retorno con rebote (def. 240)\n"
"  --press-ease <curva>       Easing del achique (def. easeOut)\n"
"  --release-ease <curva>     Easing del retorno (def. easeOutBack)\n\n"
"Efecto de sacudida (agrandar):\n"
"  --no-wiggle                Desactiva el efecto de sacudida\n"
"  --grow-scale <f>           Factor máximo al agrandar (def. 2.0)\n"
"  --grow-duration <ms>       Duración del crecimiento (def. 250)\n"
"  --grow-hold <ms>           Sigue grande este tiempo tras dejar de sacudir (def. 200)\n"
"  --grow-shrink <ms>         Duración del retorno (def. 200)\n"
"  --wiggle-window <ms>       Ventana de detección (def. 600)\n"
"  --wiggle-distance <px>     Distancia mínima (def. 1200)\n"
"  --wiggle-flips <n>         Cambios de dirección mínimos (def. 4)\n"
"  --wiggle-velocity <px/ms>  Velocidad mínima (def. 2.0)\n\n"
"General:\n"
"  --fps <n>                  Cuadros por segundo (def. 60)\n"
"  -h, --help                 Esta ayuda\n"
"  -v, --version              Versión\n\n"
"Curvas de easing: linear, ease, easeIn, easeOut, easeInOut, easeOutCubic,\n"
"  easeInCubic, easeOutExpo, easeOutBack, easeInOutBack, o custom x1,y1,x2,y2\n",
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
                fprintf(stderr, "Easing inválido: %s\n", optarg); return -1;
            }
            break;
        case OPT_REL_EASE:
            if (easing_parse(optarg, &c->release_ease)) {
                fprintf(stderr, "Easing inválido: %s\n", optarg); return -1;
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
