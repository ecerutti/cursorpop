# Empaquetado Debian

Este directorio contiene los archivos de control para generar el paquete `.deb` de cursorpop.

## Cómo construir el paquete

Desde la raíz del repositorio:

```bash
make deb
```

Eso compila el binario, hace un `make install` a un directorio temporal de staging,
aplica los archivos de control de este directorio y produce un archivo `.deb` listo para distribuir:

```
cursorpop_0.1.0_amd64.deb
```

Para instalarlo localmente después de generarlo:

```bash
sudo dpkg -i cursorpop_0.1.0_amd64.deb
```

Si faltara alguna dependencia:

```bash
sudo apt install -f
```

## Archivos de este directorio

| Archivo | Propósito |
|---------|-----------|
| `control` | Metadatos del paquete (nombre, versión, dependencias, descripción). Los marcadores `@VERSION@` y `@ARCH@` son reemplazados por `sed` al construir. |
| `postinst` | Se ejecuta después de instalar. Actualiza la base de datos de aplicaciones del escritorio. |
| `prerm` | Se ejecuta antes de desinstalar. Detiene el daemon si está corriendo. |

## Dependencias del sistema para construir

```bash
sudo apt install build-essential dpkg-dev \
                 libx11-dev libxfixes-dev libxi-dev libxext-dev \
                 python3-gi gir1.2-gtk-3.0 gir1.2-xapp-1.0
```

## Notas

- El staging se hace en `/tmp/cursorpop-deb-staging` y se borra automáticamente al terminar.
- El paquete se genera con `dpkg-deb --root-owner-group` para evitar warnings de permisos.
- `mimeinfo.cache` se elimina del staging antes de empaquetar (lo genera `update-desktop-database` durante el install y no debe ir en el paquete).
