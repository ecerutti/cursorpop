# cursorpop

Anima el puntero del mouse en Linux/X11, imitando el comportamiento de macOS:

- 🖱️ **Click → achica:** al apretar un botón del mouse, el cursor se encoge; al
  soltar vuelve a su tamaño con un pequeño rebote.
- 🫨 **Sacudida → agranda:** al mover el mouse rápido de un lado a otro, el cursor
  crece momentáneamente para que lo encuentres (estilo "shake to locate").

Funciona con **cualquier cursor** (flecha, manito de links, cursor de texto, etc.)
y en **cualquier entorno de escritorio sobre X11**: Cinnamon, GNOME, KDE Plasma,
Xfce, MATE, i3 y demás.

Incluye una **GUI de configuración** (ícono en la bandeja del sistema + ventana
de ajustes), además de la configuración por línea de comandos.

> Estado: v0.1.0 — funcional. Probado en Linux Mint Debian Edition (Cinnamon).

## Arquitectura

Son dos programas que viven en este mismo repo:

- **`cursorpop`** — el daemon (C). Hace la animación. Liviano y sin dependencias
  de GUI; corre en segundo plano. Lee `~/.config/cursorpop/cursorpop.conf` y los
  flags CLI (los flags tienen prioridad), y recarga la config con `SIGHUP`.
- **`cursorpop-settings`** — la GUI (Python + GTK). Sólo edita el archivo de
  config y controla el daemon (lo arranca/detiene y le manda `SIGHUP`). No corre
  de forma residente más allá del ícono de bandeja.

## Cómo funciona

X11 no permite animar el cursor de hardware sin, o bien hacer un *pointer grab*
(que robaría los clicks), o bien que el servidor no re-renderice el cambio. Para
evitar ambos problemas, cursorpop:

1. Escucha los eventos del mouse con **XInput2** (sin interceptarlos, así los
   clicks siguen funcionando normalmente).
2. Cuando empieza un efecto, **captura los píxeles del cursor actual** con
   `XFixesGetCursorImage` (por eso funciona con cualquier forma de cursor).
3. Dibuja un sprite escalado en una **ventana ARGB override-redirect transparente
   al click** que sigue al puntero, y **oculta el cursor real sólo mientras dura
   el efecto** (`XFixesHideCursor`).
4. Anima la escala con curvas **cubic-bézier** (easing configurable).

No usa pointer grabs, así que nunca interfiere con los clicks ni el arrastre.

La idea del efecto de sacudida está inspirada en
[wiggle-grow](https://github.com/dvanmh/wiggle-grow).

## Dependencias

Bibliotecas de desarrollo de X11:

```bash
# Debian / Ubuntu / Linux Mint / LMDE
sudo apt install build-essential libx11-dev libxfixes-dev libxi-dev libxext-dev

# Fedora
sudo dnf install gcc make libX11-devel libXfixes-devel libXi-devel libXext-devel

# Arch
sudo pacman -S base-devel libx11 libxfixes libxi libxext
```

Para la **GUI** (opcional) hacen falta Python 3 + GTK (en Linux Mint ya vienen):

```bash
# Debian / Ubuntu / Mint / LMDE
sudo apt install python3-gi gir1.2-gtk-3.0
# Ícono de bandeja nativo en Mint (recomendado; si no, usa Gtk.StatusIcon):
sudo apt install gir1.2-xapp-1.0
```

Requiere un **compositor activo** (para el visual ARGB de 32 bits). La mayoría de
los escritorios modernos lo tienen por defecto.

## Compilar e instalar

```bash
make
sudo make install      # instala en /usr/local/bin (PREFIX configurable)
```

O simplemente ejecutar el binario sin instalar: `./cursorpop`

## Uso

```bash
cursorpop            # corre en primer plano; Ctrl-C para salir
cursorpop &          # en segundo plano
cursorpop --help     # todas las opciones
```

### Configuración con interfaz gráfica

```bash
cursorpop-settings           # abre la ventana de configuración
cursorpop-settings --tray    # corre en la bandeja del sistema
```

Desde la ventana podés elegir, entre otras cosas, si el cursor se achica en
**todos los clicks** o **solo al mantener apretado** (con el tiempo
configurable), el tamaño del achique y del agrandado, y la sensibilidad de la
sacudida. Al **Aplicar**, escribe el archivo de config y le avisa al daemon.

El ícono de la bandeja tiene un toggle para activar/desactivar y un acceso a la
configuración.

> En desarrollo (sin instalar), corré la GUI con:
> `python3 gui/cursorpop-settings.py` — encuentra el binario `./cursorpop`.

### Arranque automático

Para que se inicie con tu sesión (lanza el ícono de bandeja, que a su vez
arranca el daemon si está activado):

```bash
mkdir -p ~/.config/autostart
cp data/cursorpop.desktop ~/.config/autostart/
```

## Opciones

```
Efecto de click:
  --no-press                 Desactiva el efecto de click
  --press-scale <f>          Tamaño al apretar (def. 0.80)
  --press-delay <ms>         Mínimo apretado para disparar; ignora taps (def. 120)
  --press-duration <ms>      Duración del achique (def. 100)
  --release-duration <ms>    Duración del retorno con rebote (def. 240)
  --press-ease <curva>       Easing del achique (def. easeOut)
  --release-ease <curva>     Easing del retorno (def. easeOutBack)

Efecto de sacudida:
  --no-wiggle                Desactiva el efecto de sacudida
  --grow-scale <f>           Factor máximo al agrandar (def. 2.0)
  --grow-duration <ms>       Duración del crecimiento (def. 250)
  --grow-hold <ms>           Sigue grande este tiempo tras dejar de sacudir (def. 200)
  --grow-shrink <ms>         Duración del retorno (def. 200)
  --wiggle-window <ms>       Ventana de detección (def. 600)
  --wiggle-distance <px>     Distancia mínima (def. 1200)
  --wiggle-flips <n>         Cambios de dirección mínimos (def. 4)
  --wiggle-velocity <px/ms>  Velocidad mínima (def. 2.0)

General:
  --fps <n>                  Cuadros por segundo (def. 60)
```

Curvas de easing disponibles: `linear`, `ease`, `easeIn`, `easeOut`,
`easeInOut`, `easeOutCubic`, `easeInCubic`, `easeOutExpo`, `easeOutBack`,
`easeInOutBack`, o una curva custom `x1,y1,x2,y2`.

## Limitaciones conocidas

- **Sólo X11.** Wayland no permite este tipo de manipulación global del cursor.
- Al **agrandar** cursores chicos, la imagen se escala por software (bilineal),
  así que puede verse algo borrosa con factores grandes.
- Requiere compositor para la transparencia del overlay.

## Licencia

MIT — ver [LICENSE](LICENSE).
