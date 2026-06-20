/* config.h — Parámetros de cursorpop (defaults + parseo de CLI). */
#ifndef CURSORPOP_CONFIG_H
#define CURSORPOP_CONFIG_H

#include "easing.h"

typedef struct {
    /* Efecto de click (estilo Mac: achica al apretar, rebota al soltar) */
    int    press_enabled;
    double press_scale;          /* tamaño al mantener apretado (ej. 0.80) */
    int    press_delay_ms;       /* mínimo apretado para disparar (ignora taps) */
    int    press_duration_ms;    /* duración del achique */
    int    release_duration_ms;  /* duración del retorno (con rebote) */
    Easing press_ease;
    Easing release_ease;

    /* Efecto de agrandar al sacudir (estilo "shake to locate") */
    int    wiggle_enabled;
    double grow_scale;           /* factor máximo al agrandar (ej. 2.0) */
    int    grow_duration_ms;
    int    grow_hold_ms;
    int    grow_shrink_ms;
    Easing grow_ease;
    Easing grow_shrink_ease;

    /* Detección de sacudida */
    int    wiggle_window_ms;     /* ventana temporal de análisis */
    double wiggle_min_distance;  /* distancia mínima recorrida (px) */
    int    wiggle_min_flips;     /* cambios de dirección mínimos */
    double wiggle_min_velocity;  /* velocidad mínima (px/ms) */

    /* General */
    int    fps;                  /* cuadros por segundo de la animación */
} Config;

#include <stddef.h>

void config_defaults(Config *c);

/* Parsea argv. Devuelve 0 para continuar, 1 para salir con éxito
 * (--help/--version ya impresos), -1 si hubo error de argumentos. */
int config_parse(Config *c, int argc, char **argv);

/* Ruta por defecto del archivo de config: ~/.config/cursorpop/cursorpop.conf */
void config_default_path(char *buf, size_t n);

/* Carga valores desde un archivo "key=value". Devuelve 0 si se leyó, -1 si no
 * existe (no es error). Las claves ausentes dejan el valor previo intacto.
 * Es lo que escribe la GUI; el daemon lo recarga con SIGHUP. */
int config_load_file(Config *c, const char *path);

#endif
