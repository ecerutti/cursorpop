# cursorpop

Dos pequeños efectos visuales para el cursor del mouse en Linux, inspirados en macOS:

- **Al hacer click**, el cursor se achica un instante y vuelve con un pequeño rebote.
- **Al sacudir el mouse** rápido de un lado a otro, el cursor crece temporalmente para que lo encuentres en pantalla.

Funciona en cualquier escritorio sobre X11: Cinnamon, GNOME, KDE, XFCE, MATE y otros.

> v0.1.0 — Probado en Linux Mint Debian Edition (Cinnamon).

---

## Instalación

### Opción 1 — Paquete Debian (más fácil)

Descargá el archivo `.deb` desde la [página de releases](https://github.com/ecerutti/cursorpop/releases) e instalalo con doble click, o desde la terminal:

```bash
sudo dpkg -i cursorpop_0.1.0_amd64.deb
```

Si faltara alguna dependencia, ejecutá después:

```bash
sudo apt install -f
```

### Opción 2 — Compilar desde el código fuente

**1. Instalá las dependencias:**

```bash
# Debian / Ubuntu / Linux Mint / LMDE
sudo apt install build-essential libx11-dev libxfixes-dev libxi-dev libxext-dev \
                 python3-gi gir1.2-gtk-3.0 gir1.2-xapp-1.0
```

**2. Compilá e instalá:**

```bash
make
sudo make install
```

**3. (Opcional) Para generar un paquete `.deb` vos mismo:**

```bash
make deb
sudo dpkg -i cursorpop_0.1.0_amd64.deb
```

---

## Primeros pasos

Después de instalar, abrí **Configuración de Cursorpop** desde el menú de tu escritorio (está en Preferencias o Accesorios).

Se abre una ventana donde podés:

- Activar o desactivar cada efecto por separado.
- Ajustar cuánto se achica el cursor al hacer click y cuánto crece al sacudir.
- **Activar el inicio automático** con la sesión gráfica — solo marcá el tilde "Iniciar con la sesión gráfica" y listo.

Al hacer click en **Aplicar**, los cambios se aplican de inmediato. Al cerrar la ventana, cursorpop queda corriendo en la bandeja del sistema (el ícono del mouse en la esquina del panel).

### Bandeja del sistema

Desde el ícono de la bandeja podés:

- Activar o desactivar cursorpop con un solo click.
- Abrir la ventana de configuración.
- Cerrar la aplicación completamente.

---

## Desinstalar

Si instalaste con el paquete `.deb`:

```bash
sudo apt remove cursorpop
```

Si instalaste con `make install`:

```bash
sudo make uninstall
```

---

## Preguntas frecuentes

**¿Funciona con Wayland?**
No. Wayland no permite este tipo de efecto global sobre el cursor. Funciona solo en sesiones X11.

**¿El cursor se ve borroso al agrandarse?**
Con factores de escala muy grandes (más de 2×) puede verse algo borroso, ya que la imagen del cursor se escala por software. Con el valor por defecto (2×) se ve bien.

**¿Necesita algún compositor especial?**
Necesita que haya un compositor activo para la transparencia del efecto. La mayoría de los escritorios modernos lo tienen por defecto (en Cinnamon, GNOME, KDE y XFCE ya viene activado).

**La GUI no aparece en el menú después de instalar**
Probá cerrar sesión y volver a entrar, o ejecutar en la terminal:
```bash
update-desktop-database ~/.local/share/applications
```

---

## Configuración avanzada (línea de comandos)

Para usuarios que prefieren no usar la GUI, el daemon acepta opciones al correr:

```bash
cursorpop --press-scale 0.75 --grow-scale 2.5 --no-wiggle
```

Y lee `~/.config/cursorpop/cursorpop.conf` al iniciar. Para ver todas las opciones:

```bash
cursorpop --help
```

<details>
<summary>Lista completa de opciones</summary>

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

Curvas de easing: `linear`, `ease`, `easeIn`, `easeOut`, `easeInOut`,
`easeOutCubic`, `easeInCubic`, `easeOutExpo`, `easeOutBack`, `easeInOutBack`,
o una curva custom `x1,y1,x2,y2`.

</details>

---

## Cómo funciona (para curiosos)

cursorpop tiene dos componentes:

- **`cursorpop`** — un daemon liviano escrito en C que hace la animación. Corre en segundo plano, sin interfaz gráfica.
- **`cursorpop-settings`** — la GUI escrita en Python + GTK que configura el daemon y muestra el ícono en la bandeja.

El daemon escucha eventos del mouse con XInput2, captura la imagen del cursor actual con `XFixesGetCursorImage`, y dibuja una versión escalada en una ventana transparente que sigue al puntero. El cursor real se oculta solo durante el efecto. No usa *pointer grabs*, así que nunca interfiere con los clicks ni el arrastre.

La idea del efecto de sacudida está inspirada en [wiggle-grow](https://github.com/dvanmh/wiggle-grow).

---

## Licencia

MIT — ver [LICENSE](LICENSE).
