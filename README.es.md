<!-- Idioma: [English](README.md) · **Español** -->

# CursorPop

[![CI](https://github.com/ecerutti/cursorpop/actions/workflows/ci.yml/badge.svg)](https://github.com/ecerutti/cursorpop/actions/workflows/ci.yml)

Dos pequeños efectos visuales para el cursor del mouse en Linux, inspirados en macOS:

- **Al hacer click**, el cursor se achica un instante y vuelve con un pequeño rebote.
- **Al sacudir el mouse** rápido de un lado a otro, el cursor crece temporalmente para que lo encuentres en pantalla.

Funciona en cualquier escritorio sobre X11: Cinnamon, GNOME, KDE, XFCE, MATE y otros.

> v0.3.0 — Probado en Linux Mint Debian Edition (Cinnamon).

La ventana de configuración usa el **idioma del sistema** (vienen incluidos
inglés y español; cualquier otro idioma cae a inglés).

---

## Instalación

### Opción 1 — Paquete Debian (más fácil)

Descargá el archivo `.deb` desde la [página de releases](https://github.com/ecerutti/cursorpop/releases) e instalalo con doble click, o desde la terminal:

```bash
sudo dpkg -i cursorpop_0.3.0_amd64.deb
```

Si faltara alguna dependencia, ejecutá después:

```bash
sudo apt install -f
```

### Opción 2 — Compilar desde el código fuente

**1. Instalá las dependencias:**

```bash
# Debian / Ubuntu / Linux Mint / LMDE
sudo apt install build-essential gettext libx11-dev libxfixes-dev libxi-dev libxext-dev \
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
sudo dpkg -i cursorpop_0.3.0_amd64.deb
```

---

## Primeros pasos

Después de instalar, abrí **Configuración de CursorPop** desde el menú de tu escritorio (está en Preferencias o Accesorios).

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

### Si instalaste con el paquete `.deb`

```bash
sudo apt remove cursorpop
```

### Si instalaste con `make install`

```bash
sudo make uninstall
```

### Opcional — borrar tus datos personales

Los pasos anteriores eliminan el programa pero dejan tus archivos por-usuario
(la configuración y la entrada de autostart). Para borrarlos también:

```bash
rm -rf ~/.config/cursorpop ~/.config/autostart/cursorpop.desktop
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

Y lee `~/.config/cursorpop/cursorpop.conf` al iniciar. Ver la
[referencia completa de CLI y configuración](docs/cli-reference.md) (en inglés).

---

## Para desarrolladores

La documentación técnica está en inglés:

| Documento | Contenido |
|-----------|-----------|
| [docs/architecture.md](docs/architecture.md) | Cómo funciona internamente: X11, overlay, easing, detección de sacudida |
| [docs/cli-reference.md](docs/cli-reference.md) | Todas las opciones CLI y el formato del archivo de config |
| [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md) | Cómo compilar, estructura del código, guía de estilo y cómo contribuir |
| [CHANGELOG.md](CHANGELOG.md) | Historial de versiones |

---

## Licencia

MIT — ver [LICENSE](LICENSE).
