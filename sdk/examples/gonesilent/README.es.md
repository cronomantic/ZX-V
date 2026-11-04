[English](README.md) · **Español**

# Gone Silent

Una aventura en 3D en C para ZX-V. La colonia minera de Better Worlds no
responde desde hace semanas; el Servicio de Rescate Colonial te envía a
averiguar qué ha pasado, pero una tormenta te obliga a un aterrizaje forzoso
y el módulo no despegará sin cuatro piezas. Explora la colonia con tu traje,
lee lo que dejaron los colonos y encuentra el camino de vuelta.

El mundo está hecho de polígonos sólidos con sombreado en tramas, como los
juegos en 3D de los ochenta, y las criaturas y los objetos son sprites de
pixel art.

```sh
make -C sdk                       # build/gonesilent.zxv
make -C sdk run SD=mi_tarjeta     # y en el BASIC: LOAD "gonesilent"
```

También está en el disco ROM de la máquina: `LOAD "ROM:gonesilent"`.

## El juego

- **La colonia:** el exterior, con el módulo y las tormentas, y cuatro
  instalaciones: los habitáculos (con el invernadero y el taller), control y
  comunicaciones, la mina (tres niveles hacia abajo) y la planta de
  terraformación. Algunas puertas piden una tarjeta, un código, energía o
  algo con lo que abrirlas.
- **El traje:** su energía se gasta al andar fuera, al disparar, con cada
  golpe, en el aire tóxico y en las tormentas, y se recarga encima de los
  puntos de recarga. Si se acaba, vuelves al último punto de recarga, con todo
  lo conseguido.
- **Las criaturas** cazan cada una a su manera (al acecho, en emboscada o
  persiguiéndote) y se yerguen antes de golpear. Las torretas de las defensas
  disparan mientras sigan activas.
- **El sensor de movimiento**, en el visor, avisa con pitidos de lo que se
  mueve delante de ti; las tormentas lo confunden.
- **Los registros** de los colonos cuentan lo que pasó y, a veces, dan lo que
  hace falta para seguir. El final depende de lo que hayas hecho.
- **El mapa** (M) dibuja lo que has recorrido de la zona en la que estás.
- **La partida se guarda** al llegar a un punto de recarga, en
  `SD:gonesilent.sav`; en el título se puede continuar o empezar otra.
- **Dos modos de vídeo:** Tab cambia entre 256 × 192 y 512 × 384.
- **Textos en español o en inglés**, según el idioma de la máquina.

| Tecla | Mando | |
|---|---|---|
| Cursor arriba y abajo, o Q y A | Arriba y abajo | Andar |
| Cursores izquierda y derecha, u O y P | Izquierda y derecha | Girar |
| Z y X | | De lado, a la izquierda y a la derecha |
| W y S (C, al frente) | | Mirar arriba y abajo |
| Espacio | Botón 1 | Disparar |
| Intro o U | Botón 2 | Usar o leer lo que tienes delante |
| I | | Lo que llevas |
| M | | El mapa |
| H | | La ayuda |
| Tab | | El modo de vídeo |
| Esc | | Al BASIC |

## Sus ficheros

La música y los efectos van dentro del programa, pero si en la carpeta desde
la que se carga hay uno con el mismo nombre, usa ese: para cambiarlos no hace
falta compilar.

| Fichero | Qué tiene | Se hace con |
|---|---|---|
| [`gonesilent.gfx`](gonesilent.gfx) | Los sprites: las criaturas, la incubadora y los objetos | [`mkcreature.py`](mkcreature.py) y [`mkitems.py`](mkitems.py); se retoca con `zxv-gfx` |
| [`gonesilent-title.pt3`](gonesilent-title.pt3) | La música del título | [`mksound.py`](mksound.py); se edita con `zxv-tracker` |
| [`gonesilent-end.pt3`](gonesilent-end.pt3) | La del final | `mksound.py`; `zxv-tracker` |
| [`gonesilent.afb`](gonesilent.afb) | Los efectos, un banco AYFX | `mksound.py`; `zxv-tracker` |

`mkcreature.py` modela cada criatura en 3D, con formas que se funden unas con
otras, y la pasa a pixel art desde cinco ángulos y en cada postura (necesita
numpy); `mkitems.py` dibuja los objetos. Los dos rehacen `gonesilent.gfx`
entero, así que un retoque hecho a mano se pierde si se vuelven a ejecutar.
Al compilar, [`mkspr.py`](mkspr.py) guarda de cada fila de los sprites solo lo
que no es transparente: ocupan un tercio y se dibujan desde ahí.

## Cómo está hecho

- [`world.c`](world.c): las zonas, como listas de piezas (cajas, pirámides,
  cuñas, prismas y octaedros) con su material en cada cara; las puertas, los
  objetos, las consolas, las luces y el mobiliario también son piezas. Y los
  textos: los registros, la introducción y los finales.
- [`render.c`](render.c): la vista. Las caras se sombrean según hacia dónde
  miran, con tramas de 4 × 4 entre los tonos de cada material, y se ordenan de
  atrás adelante (el algoritmo del pintor); lo lejano se mezcla con la bruma.
  Los sprites se escalan con la distancia.
- [`main.c`](main.c): el juego: el paseo y los choques, las criaturas y las
  torretas, las consolas, las luces, el sensor, el mapa, la partida guardada
  y el marcador.
- [`video.c`](video.c): la pantalla, con dos paletas por la tabla por línea:
  la de la zona para la vista y la del visor para el marcador.
- [`sound.c`](sound.c): la música en el primer AY y los efectos en el segundo.
