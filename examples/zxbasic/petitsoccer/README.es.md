[English](README.md) · **Español**

# Petit Soccer

Un juego de fútbol 7 para ZX-V escrito en ZX Basic, el compilador de Boriel,
con su target para ZX-V: contra la máquina o entre dos jugadores, con dos
tiempos, saques de banda, de esquina y de puerta, ocho equipos y tres niveles.
Es el ejemplo grande de la librería `zxv.bas`:

- **Un campo más grande que la pantalla** (`MODE 0, 512, 384`), que la cámara
  recorre tras el balón con `SCROLL`; cada imagen se dibuja entera en la
  página oculta (`SCREEN`, `PAGE`, `CLIP`) y va a 60 por segundo.
- **Los jugadores**, de pie y en ocho direcciones, con cinco dibujos y su
  espejo (`SPRITE` con espejo); cada uno se pinta con los colores de su
  equipo y su pelo cambiando tres colores del mismo sprite. Todo usa los 16
  colores de siempre de ZX-V.
- **Los textos del marcador**, con la fuente de la máquina convertida en
  sprites (`GRAB`), para escribirlos en cualquier sitio del campo y a
  cualquier tamaño.
- **La música y los efectos**, de un módulo de tracker y un banco AYFX
  (`MUSICLOAD`, `MUSICPLAY`, `SFXLOAD`, `SFX`).
- Las posiciones van en 1/256 de píxel, con enteros (`LONG`), para que todo
  se mueva suave sin usar números con decimales.

Sale en el idioma de la máquina, en español o en inglés.

| Fichero | Contenido |
|---|---|
| `petitsoccer.bas` | El programa, en ZX Basic |
| `sprites.png` | La hoja de sprites: los jugadores, el balón, las sombras y la portería |
| `mksprites.py` | Hace `sprites.bas`, lo que incluye el programa, de `sprites.png` |
| `sprites.bas` | Los sprites, como órdenes `GRABSTR` (no se cambia a mano) |
| `mksound.py` | Hace la música y los efectos |
| `petitsoccer.pt3`, `petitsoccer.afb` | La música del título y los efectos |
| `build.py` | Lo compila, con la música y los efectos dentro |
| `petitsoccer.zxv`, `petitsoccer.sha256` | El programa compilado y las huellas de aquello con lo que se compiló |

## Compilarlo

Hace falta la rama `zxv` del fork de ZX Basic (`cronomantic/zxbasic`),
v1.19.0-zxv.6 o posterior, y el compilador de C de RISC-V, el mismo que usa el
SDK:

```
python build.py ruta/de/zxbc
```

Es `zxbc --arch zxv --embed petitsoccer.pt3 --embed petitsoccer.afb
petitsoccer.bas`: sale `petitsoccer.zxv`, con la música y los efectos dentro,
así que funciona solo, desde la tarjeta o desde `ROM:`, donde viene incluido
(`LOAD "ROM:petitsoccer"`). Un `petitsoccer.pt3` o `petitsoccer.afb` en la
carpeta actual ocupa el lugar del de dentro.

El `.zxv` va en el repositorio, porque las compilaciones automáticas no pueden
compilar ZX Basic: `build.py` escribe también `petitsoccer.sha256`, y una prueba
del emulador falla si cambian las fuentes y no se vuelve a compilar el `.zxv`
(`python build.py --check` dice qué fichero cambió).

Si retocas `sprites.png` (con cualquier editor de imágenes), `python
mksprites.py` vuelve a hacer `sprites.bas`. Los jugadores van en casillas de
12 × 18 con los pies en la última fila; el rojo es la camiseta y las medias,
el azul el pantalón y el amarillo el pelo, que son los tres colores que el
juego cambia. A la derecha de la hoja está la portería, de pie y con su red:
es la de la derecha del campo, y la otra es su espejo. La música y los efectos se pueden retocar con `zxv-tracker`, o
cambiando `mksound.py`, que los vuelve a hacer desde cero.

## Jugar

En el título, arriba y abajo eligen una opción y los lados la cambian:
cuántos jugadores, los dos equipos, los minutos del partido y el nivel de la
máquina. Si no se toca nada, al rato empieza un partido de demostración.

| | Jugador 1 | Jugador 2 |
|---|---|---|
| Moverse | Cursores, o el mando 1 | W, A, S y D, o el mando 2 |
| Botón | Espacio, o el fuego | F, o el fuego |

Se maneja al jugador de la flecha: el que lleva el balón, el que lo va a
recibir o el que está más cerca.

| Con el balón | |
|---|---|
| Un toque al botón | Un pase al compañero que tiene delante |
| El botón mantenido | Un tiro, más fuerte y más alto cuanto más se aguanta. Mirando a la portería va a puerta: recto al centro, y en diagonal a un palo |

| Sin el balón | |
|---|---|
| El botón | Una entrada por el suelo: si llega al balón, se lo quita al rival |

En los saques se elige la dirección y se suelta el botón. El portero juega
solo. No hay fueras de juego ni faltas.
