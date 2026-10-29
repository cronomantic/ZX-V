[English](README.md) · **Español**

# Platformer: un juego en BASIC

Un juego de plataformas pequeño escrito en el BASIC de ZX-V, para ver
juntas sus extensiones: un robot recorre un nivel de cinco pantallas con
scroll, salta entre plataformas, recoge monedas, esquiva pinchos y fosos, y
pisa a los bichos que patrullan. Si no se toca nada en el título, juega solo.

| Fichero | Contenido |
|---|---|
| `platformer.bas` | El programa (texto con números de línea) |
| `tiles.png` | Los 8 tiles de 8 × 8, en este orden: cielo, hierba, tierra, ladrillo (los tres sólidos), moneda, pinchos, meta y nube |
| `sprites.png` | Los sprites de 16 × 16: el robot (quieto y andando) y el bicho (dos pasos) |
| `level.png` | El nivel dibujado entero (1280 × 160) con esos tiles |
| `Makefile` | Hace `build/platformer.gfx` y copia el programa a `build/` |

## Probarlo

Viene en el disco ROM de la máquina: `CD "ROM:"` y `LOAD "platformer"` (el
programa carga sus gráficos de la carpeta actual).

Para hacerlo tú hace falta Python 3 (para los gráficos, con
`sdk/tools/png2gfx.py`); `run` abre el emulador, el `zxv` del PATH o el que
le digas con `ZXV=`:

```sh
make -C examples/basic/platformer run ZXV=ruta/al/zxv
```

Copia `build/platformer.gfx` y el programa a la tarjeta para jugar en otra
parte: `LOAD "platformer"` lo carga y `RUN` lo arranca.

| Teclas | Mando |
|---|---|
| O, P o cursores: andar | Izquierda y derecha |
| Q, espacio o cursor arriba: saltar | Arriba o fuego |

## Cómo está hecho

- **Gráficos:** `png2gfx` convierte los tres PNG en un solo `.gfx`. La hoja de
  tiles va primero, así que el nivel reaprovecha esos tiles y cada uno
  conserva su número: el programa sabe que 1–3 son sólidos, 4 es una moneda,
  5 son pinchos y 6 la meta. Para cambiar el nivel basta con editar
  `level.png` usando esos tiles.
- **Dibujo:** cada frame se dibuja en la página oculta (`SCREEN s`) y se
  enseña con `PAGE s`; `WAIT 2` da 30 frames por segundo fijos (la máquina
  hace 60). Las velocidades van en píxeles por vuelta del bucle: 2,5 al andar
  (75 por segundo) y, en los saltos, la misma parábola que cuando iba a 25.
  El mapa (`MAP`) se dibuja desplazado según la cámara, encima los bichos y
  el robot (`SPRITE`) y arriba el marcador (`PRINT AT`). `CLIP` limita el
  dibujo a la zona del nivel, que es la que se redibuja cada frame: sin él, el
  robot que cae por un foso se dibuja debajo y ahí se queda.
- **Colisiones:** con el escenario, `CELL` en los puntos del borde del robot
  (`DEF FN s` dice si un punto es sólido); entre el robot y los bichos,
  `HIT`. Una moneda se quita del mapa con `MAP POKE`.
- **Sonido:** los efectos escriben el AY directamente con `SOUND` (y se
  apagan poco a poco); las melodías del título, de perder una vida y del
  final son `PLAY` del 128K.
- **Récord:** en `SD:platformer.txt`, la raíz de la tarjeta (también si se
  juega desde el disco ROM, donde no se puede escribir), con `OPEN #4` e
  `INPUT #4`/`PRINT #4`; `ON ERROR` cubre que aún no exista.
- **Estructura:** procedimientos (`DEF PROC`), `REPEAT`/`UNTIL`, `ELSE` y
  etiquetas (`@titulo`, `@bichos`).

En el emulador, una vuelta del bucle del juego (unas 60 sentencias) cabe de
sobra en dos frames.
