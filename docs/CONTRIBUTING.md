# Contribuir a cursorpop

¡Gracias por tu interés! cursorpop es un proyecto chico: un daemon en C para X11
y una GUI en Python + GTK. El código es simple y está dividido en módulos con
responsabilidades claras, así que es fácil entrar.

## Entorno de desarrollo

### Dependencias para compilar

```bash
# Debian / Ubuntu / Linux Mint / LMDE
sudo apt install build-essential libx11-dev libxfixes-dev libxi-dev libxext-dev \
                 python3-gi gir1.2-gtk-3.0 gir1.2-xapp-1.0
```

### Compilar

```bash
make            # binario en ./cursorpop
make clean      # elimina objetos y binario
```

Para desarrollo, compilá con sanitizers para detectar bugs de memoria:

```bash
make CFLAGS="-O0 -g -std=c11 -Wall -Wextra -fsanitize=address,undefined -D_GNU_SOURCE"
```

### Probar la GUI sin instalar

```bash
python3 gui/cursorpop-settings.py
```

La GUI encuentra el binario `./cursorpop` automáticamente si no está instalado en
el `PATH`.

## Estructura del código

```
src/            Daemon en C
  cursorpop.c   main, loop de eventos, máquina de estados de animación
  overlay.c     Ventana ARGB override-redirect transparente al click
  capture.c     Captura del cursor (XFixes) y escalado bilineal
  wiggle.c      Detección de sacudida
  easing.c      Curvas cubic-bézier y presets
  config.c      Defaults, parseo de CLI y lectura del archivo de config

gui/            GUI en Python + GTK
  cursorpop-settings.py

data/           Archivos de escritorio
  cursorpop-settings.desktop   Entrada en menús del sistema (Preferencias)
  cursorpop.desktop             Entrada de autostart (~/.config/autostart/)

packaging/      Archivos para generar el paquete Debian
  debian/
    control     Metadatos del paquete (versión y arch se sustituyen con sed)
    postinst    Script post-instalación
    prerm       Script pre-desinstalación

man/            Página de manual
  cursorpop.1

spikes/         Experimentos de validación de arquitectura (no van al binario)
```

Para una descripción técnica detallada de cómo funciona la animación, ver
[architecture.md](architecture.md).

## Guía de estilo

**C (daemon):**
- Estándar C11. Sin dependencias más allá de Xlib, XFixes, XInput2 y XExt.
- Sin warnings con `-Wall -Wextra`.
- Funciones cortas con una sola responsabilidad.
- Nombres en `snake_case`.

**Python (GUI):**
- Python 3.6+, sin dependencias fuera de PyGObject.
- La GUI no contiene lógica de animación: solo edita el archivo de config y
  controla el daemon.

## Proceso para contribuir

1. **Abrí un issue** antes de empezar si el cambio es grande, para evitar trabajo
   duplicado.
2. **Hacé un fork** y trabajá en una rama descriptiva (`fix/overlay-resize`,
   `feat/wayland-backend`, etc.).
3. Asegurate de que `make` compile sin warnings.
4. **Probá en al menos un entorno X11** y aclaralo en el PR (distribución,
   escritorio, versión).
5. Para cambios en la GUI, probá también el flujo de autostart y el ícono de
   bandeja.

## Ideas para contribuir

- Soporte para temas de iconos personalizados en la GUI.
- Página de manual más completa.
- Empaquetado para otras distribuciones (RPM, AUR).
- Tests automatizados para la detección de sacudida.
- Internacionalización (i18n) de la GUI.
