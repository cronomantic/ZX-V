[English](README.md) · **Español**

# Stardust

Un juego pequeño para ZX-V escrito en ZX Basic, el compilador de Boriel, con
su target para ZX-V: coge las gemas que caen y esquiva las rocas. Sirve de
ejemplo de la librería `zxv.bas`, que trae a ZX Basic las órdenes propias del
BASIC de ZX-V con sus mismos nombres: el doble buffer (`SCREEN` y `PAGE`),
`WAIT`, los sprites de una cadena (`GRABSTR`) con choques al píxel (`HIT`),
`BOX`, `KEY`, `JOY` y `SOUND`. Va a 50 imágenes por segundo.

| Fichero | Contenido |
|---|---|
| `stardust.bas` | El programa, en ZX Basic |

## Compilarlo

Hace falta la rama `zxv` del fork de ZX Basic (`cronomantic/zxbasic`) y el
compilador de C de RISC-V, el mismo que usa el SDK:

```
zxbc --arch zxv stardust.bas
```

Sale `stardust.zxv`: cópialo a la tarjeta y cárgalo con
`LOAD "stardust"`.

| Control | Qué hace |
|---|---|
| Cursores izquierda y derecha, u O y P | Mover la nave |
| Mando | La cruceta mueve; el fuego empieza la partida en el título |
| Espacio | Empieza la partida en el título |

Cada gema da 10 puntos; cada roca quita una de las tres vidas. Cuantos más
puntos, más cosas caen y más deprisa.
