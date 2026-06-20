# Contribuir a cursorpop

¡Gracias por tu interés! cursorpop es un proyecto chico en C sobre X11.

## Compilar para desarrollo

```bash
make            # binario en ./cursorpop
make clean
```

Recomendado compilar con warnings y sanitizers al desarrollar:

```bash
make CFLAGS="-O0 -g -std=c11 -Wall -Wextra -fsanitize=address,undefined -D_GNU_SOURCE"
```

## Estructura del código

| Archivo            | Responsabilidad                                            |
|--------------------|------------------------------------------------------------|
| `src/cursorpop.c`  | `main`, loop de eventos y máquina de estados de animación  |
| `src/overlay.c`    | Ventana ARGB override-redirect transparente al click       |
| `src/capture.c`    | Captura del cursor (XFixes) y escalado bilineal            |
| `src/wiggle.c`     | Detección de sacudida                                       |
| `src/easing.c`     | Curvas cubic-bézier y presets                              |
| `src/config.c`     | Defaults y parseo de CLI                                    |

Los `spikes/` son experimentos de validación de la arquitectura; no forman
parte del binario.

## Estilo

- C11, sin dependencias más allá de Xlib + XFixes + XInput2 + XShape.
- Mantené el código sin warnings (`-Wall -Wextra`).
- Funciones y módulos chicos y con una sola responsabilidad.

## Pull requests

1. Abrí un issue describiendo el cambio si es grande.
2. Asegurate de que `make` compile sin warnings.
3. Probá en al menos un entorno X11 y aclaralo en el PR.
