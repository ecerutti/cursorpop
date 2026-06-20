# Arquitectura de cursorpop

## Componentes

cursorpop está dividido en dos programas independientes que viven en el mismo repositorio:

### `cursorpop` — el daemon (C)

Hace la animación. Corre en segundo plano sin interfaz gráfica. Lee
`~/.config/cursorpop/cursorpop.conf` al iniciar y recarga la config con `SIGHUP`.
Los flags de CLI tienen prioridad sobre el archivo de config.

| Archivo           | Responsabilidad                                           |
|-------------------|-----------------------------------------------------------|
| `src/cursorpop.c` | `main`, loop de eventos, máquina de estados de animación  |
| `src/overlay.c`   | Ventana ARGB override-redirect transparente al click      |
| `src/capture.c`   | Captura del cursor (XFixes) y escalado bilineal           |
| `src/wiggle.c`    | Detección de sacudida                                     |
| `src/easing.c`    | Curvas cubic-bézier y presets                             |
| `src/config.c`    | Defaults, parseo de CLI y lectura del archivo de config   |

Los `spikes/` son experimentos de validación de la arquitectura; no forman parte del binario.

### `cursorpop-settings` — la GUI (Python + GTK)

Solo edita `~/.config/cursorpop/cursorpop.conf` y controla el daemon (lo arranca,
detiene y le manda `SIGHUP` para recargar). No tiene lógica de animación.

Escrito en Python 3 + GTK 3. Soporta `XApp.StatusIcon` (bandeja nativa de
Cinnamon/Mint) con fallback a `Gtk.StatusIcon`.

## Cómo funciona la animación

X11 no permite animar el cursor de hardware directamente sin hacer un *pointer
grab* (que robaría los clicks) o depender de extensiones no universales. cursorpop
resuelve esto así:

1. **Escucha eventos sin interceptarlos.** Usa XInput2 (`XISelectEvents`) para
   recibir eventos de botón y movimiento del mouse. Los eventos siguen propagándose
   normalmente; los clicks funcionan igual.

2. **Captura la imagen del cursor actual.** Cuando empieza un efecto, llama a
   `XFixesGetCursorImage` para obtener los píxeles exactos del cursor en ese
   momento. Por eso funciona con cualquier forma: flecha, manito, cursor de texto, etc.

3. **Dibuja un sprite escalado en una ventana overlay.** Crea una ventana ARGB de
   32 bits con `override-redirect` y `WM_CLASS` que la hace transparente a los
   clicks (mediante `XShapeCombineRectangles` con `ShapeInput` vacío). Esta ventana
   sigue al puntero con `XMoveWindow` en cada frame.

4. **Oculta el cursor real solo durante el efecto.** Usa `XFixesHideCursor` /
   `XFixesShowCursor` para evitar que se vea el cursor original debajo del sprite.
   Se restaura inmediatamente al terminar la animación.

5. **Anima la escala con curvas de easing.** Cada frame calcula el factor de
   escala según la curva configurada, escala la imagen capturada por software
   (interpolación bilineal en `src/capture.c`) y la dibuja con `XPutImage`.

## Detección de sacudida

`src/wiggle.c` mantiene un historial circular de posiciones recientes. En cada
evento de movimiento acumula la distancia recorrida y cuenta los cambios de
dirección horizontal dentro de una ventana de tiempo (`wiggle_window`). El efecto
se dispara cuando se cumplen simultáneamente tres condiciones:

- Distancia total ≥ `wiggle_distance`
- Cambios de dirección ≥ `wiggle_flips`
- Velocidad media ≥ `wiggle_velocity`

## Curvas de easing

`src/easing.c` implementa easing mediante cubic-bézier parametrizado, igual que
CSS `cubic-bezier()`. Los presets (`easeOut`, `easeOutBack`, etc.) son alias de
curvas estándar. También se puede pasar una curva custom como `x1,y1,x2,y2`.

La evaluación usa bisección numérica para invertir el polinomio de Bernstein, con
tolerancia de `1e-5` en ~8 iteraciones.

## Diagrama de estados (efecto de click)

```
IDLE ──press──► SHRINKING ──timeout──► HOLDING
                                          │
                                       release
                                          │
                                          ▼
                                      RELEASING ──done──► IDLE
```

## Archivo de configuración

Ubicación: `~/.config/cursorpop/cursorpop.conf`

Formato: `clave=valor`, una por línea. Líneas que empiezan con `#` o `;` son
comentarios. La GUI regenera el archivo completo al aplicar cambios.

La clave `enabled` es propia de la GUI (indica si debe arrancar el daemon al
iniciar). El daemon la ignora.
