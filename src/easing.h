/* easing.h — Curvas de animación tipo CSS cubic-bezier(x1,y1,x2,y2). */
#ifndef CURSORPOP_EASING_H
#define CURSORPOP_EASING_H

typedef struct {
    double x1, y1, x2, y2;
} Easing;

/* Evalúa la curva en t ∈ [0,1] y devuelve el valor eased (puede pasarse de
 * [0,1] si la curva tiene overshoot, p.ej. easeOutBack para el rebote). */
double easing_eval(const Easing *e, double t);

/* Parsea un preset por nombre ("easeOut", "easeOutBack", ...) o una curva
 * custom "x1,y1,x2,y2". Devuelve 0 si OK, -1 si el string es inválido. */
int easing_parse(const char *s, Easing *out);

#endif
