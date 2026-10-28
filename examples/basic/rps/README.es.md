[English](README.md) · **Español**

# Rock, Paper, Scissors

Un juego corto en el BASIC de ZX-V, para ver las estructuras nuevas en un
programa de verdad: el **IF de bloque** (`IF ... THEN`, `ELSE IF`, `ELSE`,
`END IF`), **`SELECT CASE`** y los procedimientos (`DEF PROC`). Juegas
contra la máquina, que elige al azar; el marcador lleva la cuenta hasta que
sales.

| Fichero | Contenido |
|---|---|
| `rps.bas` | El programa, en texto con números de línea |

## Probarlo

`rps.bas` es un listado en texto: no hace falta convertirlo. Cópialo a la
tarjeta y escribe:

```
LOAD "rps"
RUN
```

También viene en el disco ROM de la máquina: `CD "ROM:"` y `LOAD "rps"`.
Con `LIST` se ve el programa, con los bloques sangrados.

| Tecla | Qué hace |
|---|---|
| P | Piedra |
| A | Papel |
| T | Tijera |
| S | Acaba la partida y dice quién ha ganado |

## Cómo está hecho

- **La tecla:** `SELECT CASE k$` reparte la tecla pulsada (`INKEY$`) entre
  las tres jugadas, la de salir, ninguna (`CASE ""`, que espera un frame) y
  cualquier otra (`CASE ELSE`, un pitido).
- **Quién gana:** con las jugadas numeradas 1, 2 y 3, la diferencia entre la
  tuya y la de la máquina, en módulo 3, dice el resultado: 0 empate, 1 ganas
  tú, 2 gana la máquina. Otro `SELECT CASE`, en `PROC ronda`, lo enseña y
  suma el punto.
- **El final:** un IF de bloque con `ELSE IF` elige uno de los tres mensajes.
- **Los procedimientos:** `ronda(t)` recibe tu jugada y usa una variable
  local (`LOCAL m`) para la de la máquina; `marcador` vuelve a pintar el
  resultado. Tras cada ronda, el programa espera a que sueltes la tecla para
  no jugar varias veces seguidas.
