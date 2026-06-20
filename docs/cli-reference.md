# Referencia CLI — cursorpop

## Uso

```bash
cursorpop [opciones]
```

El daemon corre en primer plano. Para correrlo en segundo plano: `cursorpop &`

Para recargar la config sin reiniciar: `kill -HUP $(cat $XDG_RUNTIME_DIR/cursorpop.pid)`

## Opciones

### Efecto de click

| Opción | Valor | Default | Descripción |
|--------|-------|---------|-------------|
| `--no-press` | — | — | Desactiva el efecto de click completamente |
| `--press-scale` | float | `0.80` | Tamaño del cursor al apretar (0.5 = mitad, 1.0 = sin cambio) |
| `--press-delay` | ms | `120` | Tiempo mínimo apretado para disparar el efecto. `0` lo activa en todos los clicks, incluso taps rápidos |
| `--press-duration` | ms | `100` | Duración de la animación de achique |
| `--release-duration` | ms | `240` | Duración de la animación de retorno (incluye el rebote) |
| `--press-ease` | curva | `easeOut` | Curva de easing del achique |
| `--release-ease` | curva | `easeOutBack` | Curva de easing del retorno. `easeOutBack` produce el rebote |

### Efecto de sacudida

| Opción | Valor | Default | Descripción |
|--------|-------|---------|-------------|
| `--no-wiggle` | — | — | Desactiva el efecto de sacudida completamente |
| `--grow-scale` | float | `2.0` | Factor de agrandado máximo (2.0 = el doble del tamaño) |
| `--grow-duration` | ms | `250` | Duración del crecimiento |
| `--grow-hold` | ms | `200` | Tiempo que se mantiene grande tras dejar de sacudir |
| `--grow-shrink` | ms | `200` | Duración del retorno al tamaño normal |
| `--grow-ease` | curva | `easeOut` | Curva de easing del crecimiento |
| `--grow-shrink-ease` | curva | `easeInOut` | Curva de easing del retorno |
| `--wiggle-window` | ms | `600` | Ventana de tiempo en la que se mide la sacudida |
| `--wiggle-distance` | px | `1200` | Distancia total mínima del movimiento para disparar |
| `--wiggle-flips` | n | `4` | Cambios de dirección horizontal mínimos |
| `--wiggle-velocity` | px/ms | `2.0` | Velocidad media mínima |

### General

| Opción | Valor | Default | Descripción |
|--------|-------|---------|-------------|
| `--fps` | n | `60` | Cuadros por segundo de la animación |
| `--help` | — | — | Muestra la ayuda y sale |
| `--version` | — | — | Muestra la versión y sale |

## Curvas de easing disponibles

| Nombre | Descripción |
|--------|-------------|
| `linear` | Sin suavizado |
| `ease` | Suavizado estándar CSS |
| `easeIn` | Empieza lento, termina rápido |
| `easeOut` | Empieza rápido, termina lento |
| `easeInOut` | Lento en ambos extremos |
| `easeOutCubic` | `easeOut` más pronunciado |
| `easeInCubic` | `easeIn` más pronunciado |
| `easeOutExpo` | Desaceleración exponencial |
| `easeOutBack` | Rebasa levemente el destino y vuelve (rebote) |
| `easeInOutBack` | Rebote en ambos extremos |
| `x1,y1,x2,y2` | Curva cubic-bézier custom (igual que CSS) |

## Archivo de configuración

Ubicación: `~/.config/cursorpop/cursorpop.conf`

Ejemplo:

```ini
# cursorpop — generado por cursorpop-settings
enabled=1
press_enabled=1
press_scale=0.80
press_delay=120
press_duration=100
release_duration=240
press_ease=easeOut
release_ease=easeOutBack
wiggle_enabled=1
grow_scale=2.0
grow_duration=250
grow_hold=200
grow_shrink=200
grow_ease=easeOut
grow_shrink_ease=easeInOut
wiggle_window=600
wiggle_distance=1200
wiggle_flips=4
wiggle_velocity=2.0
fps=60
```

Los flags de CLI tienen prioridad sobre los valores del archivo. El daemon recarga
el archivo con `SIGHUP` (la GUI lo hace automáticamente al aplicar cambios).

La clave `enabled` la usa solo la GUI para saber si debe arrancar el daemon; el
daemon la ignora.
