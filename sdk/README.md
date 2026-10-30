**English** · Español abajo

# ZX-V SDK

Tools to write **native programs** (`.zxv`) for ZX-V in C or RISC-V assembly.
The full guide, from installing the compiler to sound, sprites and files:
[docs/en/sdk](../docs/en/sdk/README.md). The comments in the code cite the
machine's specification as "spec §N": that is [dev/spec.md](../dev/spec.md)
(in Spanish).

| File | What it is |
|---|---|
| `include/zxv.h` | Addresses, registers, constants and the numbers of the system routines |
| `include/zxv_api.h` | The system routines (the ROM API) as `static inline` C functions |
| `include/zxv_gfx.h` | The graphics engine for games: bitmaps, sprites, tile maps, mode 7 and vectors |
| `include/zxv_gfxfile.h`, `gfxfile.c` | Reads `.gfx` graphics files: sprites, tiles, maps and textures |
| `include/zxv_music.h`, `music/` | Music and sound effects for the two AY chips: PT3, TurboSound, PT2, STC, ASC and PSG modules and AYFX effects, at their tempo of 50 steps per second |
| `zxvlib.c` | `memcpy`, `memset`, `memmove` and `memcmp`, for programs without a C library |
| `zxvc.c`, `include/sys/dirent.h` | The standard C library (picolibc) on top of the system routines: files, folders, console, heap and time |
| `zxv.ld`, `crt0.S` | The linker script and the entry point |
| `tools/elf2zxv.py` | Turns the ELF into a `.zxv` |
| `tools/gfx/` | `zxv-gfx`, the graphics editor for the PC (Qt 6): `cmake -S sdk/tools/gfx -B build-gfx` |
| `tools/tracker/` | `zxv-tracker`, the tracker for the PC (Qt 6 and SDL2): `cmake -S sdk/tools/tracker -B build-tracker` |
| `tools/png2gfx.py` | Turns PNG images into `.gfx` graphics files for BASIC (`LOAD "x" GRAPHICS`) |
| `examples/` | `hello`, `graphics`, `scroll`, `digital`, `files`, `boardtest`, `player` (a music and effects player) and `gfxedit` (a graphics editor), and four complete games: `estela` (Estela), `route128` (Route 128), `mine` (The Mine) and `street` (Street Justice), each with its README |

It needs `riscv64-unknown-elf-gcc`, picolibc for it (only for `files`) and
Python 3:

```sh
make -C sdk                                 # sdk/build/hello.zxv, graphics.zxv...
make -C sdk run SD=my_card EMU=path/to/zxv  # copies them to a folder and opens the emulator on it
```

Then, in BASIC, `LOAD "hello"`. The SDK is under the MIT licence
([LICENSE](LICENSE)); what you write with it can have any licence you like,
closed too.

---

# SDK de ZX-V

Herramientas para escribir **programas nativos** (`.zxv`) para ZX-V en C o en
ensamblador RISC-V. La guía completa, desde instalar el compilador hasta el
sonido, los sprites y los ficheros: [docs/es/sdk](../docs/es/sdk/README.md).
Los comentarios del código citan la especificación de la máquina como
"spec §N": es [dev/spec.md](../dev/spec.md).

Necesita `riscv64-unknown-elf-gcc`, su picolibc (solo para `files`) y
Python 3:

```sh
make -C sdk                                    # sdk/build/hello.zxv, graphics.zxv...
make -C sdk run SD=mi_tarjeta EMU=ruta/al/zxv  # los copia a una carpeta y abre el emulador con ella
```

Luego, en el BASIC, `LOAD "hello"`. El SDK tiene licencia MIT
([LICENSE](LICENSE)); lo que escribas con él puede tener la licencia que
quieras, también cerrada.
