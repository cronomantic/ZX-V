[English](README.md) · **Español**

<h1 align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/images/logo-dark.svg">
    <img src="docs/images/logo-light.svg" alt="ZX-V" width="236">
  </picture>
</h1>

Un ordenador de 32 bits para aficionados, inspirado en el ZX Spectrum. No es un
clon ni un emulador del Spectrum: es una máquina nueva que conserva el
espíritu del original: la enciendes y estás en el BASIC, lista para escribir
un programa.

- **Los 15 colores del Spectrum, sin *color clash*:** cada píxel tiene su
  color.
- **Un BASIC compatible con el Sinclair BASIC** (hasta donde es razonable),
  con un editor a pantalla completa y muchas extensiones: bucles
  estructurados, procedimientos, sprites, mapas de tiles, scroll, música y
  ficheros.
- **Sonido** con dos chips AY-3-8912 y el beeper.
- **Una tarjeta SD** en lugar de la cinta, que también carga los `.TAP` y
  `.TZX` del Spectrum. Trae ejemplos y juegos de serie, en la unidad `ROM:`.
- **Programas nativos en C** para su CPU RISC-V (lo que eran los juegos en
  código máquina Z80), con un SDK, y BASIC compilado con
  [ZX Basic](docs/es/zxbasic.md).
- **Un emulador** para Windows, Linux y el navegador.
- **Hardware real** con dos Raspberry Pi Pico 2: una es la CPU y la otra la
  "ULA" (el vídeo y el sonido).

> **Estado:** el emulador y la ROM con el BASIC funcionan. El firmware de cada
> Pico se ha probado por separado, en una protoboard, pero las dos Pico todavía
> no se han probado juntas. La placa está diseñada, pero aún no se ha
> fabricado.

## Pruébalo

**En el navegador:** [cronomantic.github.io/ZX-V](https://cronomantic.github.io/ZX-V/),
sin instalar nada.

**En tu ordenador:** descarga la última versión de
[**Releases**](https://github.com/cronomantic/ZX-V/releases):

| Paquete | Qué es |
|---|---|
| `zxv-windows-x64.zip` | El emulador para Windows, con la ROM y los ejemplos |
| `zxv-linux-x86_64.tar.gz` y la `.AppImage` | El emulador para Linux |
| `zxv-web.zip` | La versión web: descomprímela y abre `index.html` (no hace falta servidor) |
| `zxv-uf2.zip` | El firmware de las dos Pico de la máquina real |

Ábrelo y estás en el BASIC. Escribe esto, pulsando Intro tras cada línea:

```
10 FOR i=1 TO 8: CIRCLE INK i;128,88,i*10: NEXT i
RUN
```

Luego prueba `CAT "ROM:"` para ver los ejemplos y juegos que trae... o empieza
por la [guía de primeros pasos](docs/es/getting-started.md).

## Documentación

| | |
|---|---|
| [Primeros pasos](docs/es/getting-started.md) | De cero a tu primer programa |
| [Aprende BASIC](docs/es/basic/README.md) | Un curso desde cero, paso a paso |
| [Referencia del BASIC](docs/es/basic/reference.md) · [El editor](docs/es/basic/editor.md) | Cada orden y cada función |
| [Programas nativos (SDK)](docs/es/sdk/README.md) | Programar la máquina en C |
| [Gráficos](docs/es/sdk/graphics.md#el-editor-de-gráficos-para-el-pc-zxv-gfx) · [Música](docs/es/sdk/README.md#el-tracker-para-el-pc-zxv-tracker) | Los editores para el PC: `zxv-gfx` y `zxv-tracker` |
| [Compilar BASIC](docs/es/zxbasic.md) | ZX Basic, con un target para ZX-V |
| [El emulador](docs/es/emulator.md) | Las versiones de escritorio y web, opciones |
| [La máquina](docs/es/machine.md) | Pantalla, colores, sonido, memoria y ficheros |
| [Construir el hardware](docs/es/hardware/README.md) | La placa, el montaje y el firmware |

Todo está también en inglés: [docs/en](docs/en/README.md).

## Cómo está hecha

```
┌────────── Pico 2 "CPU" ──────────┐  enlace de 8 bits+reloj ┌────────── Pico 2 "ULA" ──────────┐
│ la ROM: el BASIC y el sistema    │───────────────────────►│ vídeo: HDMI / DVI                │
│ tus programas                    │   línea de sonido      │ sonido: dos AY + beeper          │
│ teclado, ratón y mandos USB      │───────────────────────►│        por HDMI y salida de línea│
│ tarjeta SD, reloj                │◄───────────────────────│                                  │
└──────────────────────────────────┘ sincronía, retorno     └──────────────────────────────────┘
```

Para el software hay una sola máquina; que esté hecha de dos chips es un
detalle del hardware, y el emulador implementa la misma máquina en un solo
programa.

## Qué hay aquí

| | |
|---|---|
| [`sdk/`](sdk) | El SDK para programas nativos en C, con sus ejemplos y cuatro juegos completos en C. Licencia MIT ([`sdk/LICENSE`](sdk/LICENSE)): tus programas pueden tener la licencia que quieras |
| [`examples/`](examples) | Juegos en BASIC (`basic/`) y uno en BASIC compilado con ZX Basic (`zxbasic/stardust`). Licencia MIT ([`examples/LICENSE`](examples/LICENSE)) |
| [`docs/`](docs) | La documentación, en español y en inglés. Licencia CC BY 4.0 ([`docs/LICENSE`](docs/LICENSE)), igual que `dev/`; sus ejemplos de código son de dominio público |
| [`dev/`](dev) | La especificación técnica de la máquina ([`spec.md`](dev/spec.md)) y de su BASIC ([`basic.md`](dev/basic.md)): mapa de memoria, registros, la API de la ROM y los formatos de fichero |
| [Releases](https://github.com/cronomantic/ZX-V/releases) | El emulador, la ROM y el firmware, en binario. Son gratis, y se pueden repartir sin cambios y sin cobrar: los términos están en [`TERMS.txt`](TERMS.txt) |

El código fuente del emulador, la ROM y el firmware no es público. Los
paquetes dicen en su `THIRD_PARTY_NOTICES.txt` lo que llevan que es de otros,
con los textos de sus licencias.
