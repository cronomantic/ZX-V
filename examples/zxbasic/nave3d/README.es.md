[English](README.md) · **Español**

# Nave 3D

Un ejemplo de **3D** en ZX Basic: una nave de caras planas que gira, con luz
y sombras de trama, entre estrellas que vienen hacia la cámara. Usa
`zxv3d.bas`, el 3D sencillo de las [bibliotecas del SDK](../../../docs/es/sdk/libraries.md#3d-sencillo-zxv_3dh)
en el fork de ZX Basic, y el modelo sale de un `.obj` (el
formato de Blender) con el conversor del SDK.

| Fichero | Contenido |
|---|---|
| `nave3d.bas` | El programa |
| `nave.obj` | La nave, en Wavefront `.obj`: 28 vértices y 22 caras |
| `nave.bas` | La nave en BASIC, hecha por `obj2c.py --bas` a partir de `nave.obj` |

## Cómo está hecho

- **El modelo:** `obj2c.py --bas` escribe dos matrices (`nave_v`, los
  vértices; `nave_f`, las caras con sus colores) y un `SUB nave_model(n)`
  que las pone como el modelo `n`. Los colores salen de los materiales del
  `.obj`: uno llamado `zxv_1_9` es el azul de ZX-V (1, el oscuro; 9, el
  claro); `zxv_2_10_two`, la deriva, se ve además por los dos lados.
- **Un modelo para el pintor:** las caras se pintan de lejos a cerca por su
  distancia media, así que las alas tocan el cuerpo sin meterse en él, la
  deriva es una sola cara y el lomo va partido justo debajo de ella. Si no,
  a ratos se ve un ala o la deriva donde no toca.
- **Cada imagen:** `BEGIN3D`, un `ADD3D` por objeto (el modelo, dónde está y
  su giro) y `DRAW3D`, que lo pinta todo de atrás adelante en la página
  oculta. `ADD3D` devuelve las caras que se ven.
- **Las estrellas:** son puntos del mundo; `PROJECT3D` dice si están delante
  de la cámara y `PROJ3DX` y `PROJ3DY`, en qué píxel caen.
- **El doble buffer:** se dibuja con `SCREEN` en la página oculta y se enseña
  con `PAGE`, como en Stardust.

Para cambiar la nave, edita `nave.obj` (o haz otra en Blender y expórtala
como `.obj`) y vuelve a sacar `nave.bas`:

```
python sdk/tools/obj2c.py nave.obj nave.bas nave --size 4 --bas
```

`--size 4`: el lado más largo de la nave mide 4 unidades del mundo.

## Compilarlo

Hace falta el fork de ZX Basic (`cronomantic/zxbasic`), v1.19.0-zxv.7 o
posterior, y el compilador de C de RISC-V, el mismo que usa el SDK:

```
zxbc --arch zxv nave3d.bas
```

Sale `nave3d.zxv`: cópialo a la tarjeta y cárgalo con `LOAD "nave3d"`.

| Control | Qué hace |
|---|---|
| Cursores izquierda y derecha, o el mando | Giro más deprisa a un lado o a otro |
| Cursores arriba y abajo, o el mando | Inclinar la nave |
| Espacio o fuego | Caras o solo aristas (alambre) |
