# SDK de ZX-V: compila los ejemplos a programas .ZXV.
#
#   make                 build/hello.zxv
#   make run SD=carpeta  copia los .ZXV a la carpeta y abre el emulador
#   make debug SD=...    lo mismo, con el servidor GDB en el puerto GDB_PORT
#   EXTRA_CFLAGS=...     opciones de más para el compilador (-DESTELA_STATS...)

CROSS   ?= riscv64-unknown-elf-
CC      := $(CROSS)gcc
PYTHON  ?= python3

# Perfil del Hazard3 (spec §2.1); el binutils de Ubuntu 24.04 aún no conoce Zcmp.
MARCH   ?= rv32imac_zicsr_zifencei_zba_zbb_zbs_zbkb_zcb
# -ffunction-sections y -fdata-sections: cada función y cada dato en su
# sección, para que --gc-sections deje fuera del .zxv lo que no se usa (sin
# ellas entra el fichero enlazado entero: gfxfile.c, la música...).
CFLAGS  := -march=$(MARCH) -mabi=ilp32 -Os -g -Wall -Wextra -std=gnu11 \
           -ffreestanding -fno-builtin -fno-tree-loop-distribute-patterns \
           -ffunction-sections -fdata-sections -Iinclude $(EXTRA_CFLAGS)
LDFLAGS := -nostdlib -nostartfiles -static -T zxv.ld -Wl,--gc-sections \
           -Wl,--no-warn-rwx-segments

# Con la biblioteca estándar de C (picolibc, con zxvc.c debajo): stdio con
# ficheros, <dirent.h>, malloc, time...
LIBC_CFLAGS  := -march=$(MARCH) -mabi=ilp32 -Os -g -Wall -Wextra -std=gnu11 --specs=picolibc.specs \
                -ffunction-sections -fdata-sections -Iinclude
LIBC_LDFLAGS := -nostartfiles -static -T zxv.ld -Wl,--gc-sections \
                -Wl,--no-warn-rwx-segments

EXAMPLES := hello asmhello graphics scroll digital estela boardtest route128 mine street umbra keep player samples gfxedit chess gonesilent
# Sin picolibc (MSYS2 no la tiene), sin estos: lo dice y compila los demás.
LIBC_EXAMPLES := $(if $(filter /% %:/%,$(subst \,/,$(shell $(CC) -print-file-name=picolibc.specs))),files)
ifeq ($(LIBC_EXAMPLES),)
$(info Sin picolibc: no se compila files)
endif
ZXV      := $(EXAMPLES:%=build/%.zxv) $(LIBC_EXAMPLES:%=build/%.zxv) build/fases.zxv
# El emulador compilado aquí al lado o, si no, el zxv del PATH (los paquetes,
# que llevan la ROM dentro: entonces no hace falta --rom).
EMU      ?= $(firstword $(wildcard ../emulator/build/zxv ../emulator/build/zxv-sdl) zxv)
ROM      ?= ../rom/build/rom.bin
SD       ?= build/sd
GDB_PORT ?= 1234

all: $(ZXV)

build:
	mkdir -p build

build/%.elf: examples/%/*.c crt0.S zxvlib.c zxv.ld include/*.h | build
	$(CC) $(CFLAGS) $(LDFLAGS) crt0.S $(filter %.c,$^) -lgcc -o $@

# asmhello: solo ensamblador, sin zxvlib.c (llama a la API con ZXV_CALL).
build/asmhello.elf: examples/asmhello/asmhello.S crt0.S zxv.ld include/zxv.h | build
	$(CC) $(CFLAGS) $(LDFLAGS) crt0.S $< -o $@

# Los de la biblioteca de C: zxvc.c en vez de zxvlib.c.
define LIBC_RULE
build/$(1).elf: $(wildcard examples/$(1)/*.c) crt0.S zxvc.c zxv.ld $(wildcard include/*.h include/sys/*.h) | build
	$$(CC) $$(LIBC_CFLAGS) $$(LIBC_LDFLAGS) crt0.S $$(filter %.c,$$^) -o $$@
endef
$(foreach e,$(LIBC_EXAMPLES),$(eval $(call LIBC_RULE,$(e))))

# La música y los efectos del AY (include/zxv_music.h).
MUSIC_SRC := $(wildcard music/*.c)
# Las bibliotecas opcionales (sdk/lib, D111) que usa cada juego: el .c entra
# en la compilación por estar entre las dependencias de su .elf.
LIB_INPUT := lib/input.c include/zxv_input.h
LIB_MATH := lib/math.c include/zxv_math.h
LIB_SAVE := lib/save.c include/zxv_save.h
LIB_M7 := lib/m7.c include/zxv_m7.h

build/graphics.elf: $(LIB_M7) $(LIB_MATH)

build/estela.elf: examples/estela/estela.h examples/estela/estela.gfx gfxfile.c include/zxv_gfxfile.h \
                  $(wildcard examples/estela/*.pt3) $(MUSIC_SRC) music/music_int.h include/zxv_music.h \
                  $(LIB_INPUT) $(LIB_MATH) $(LIB_SAVE)
build/route128.elf: examples/route128/route128.h examples/route128/route128.gfx gfxfile.c include/zxv_gfxfile.h \
                    $(wildcard examples/route128/*.pt3) $(MUSIC_SRC) music/music_int.h include/zxv_music.h \
                    $(LIB_INPUT) $(LIB_SAVE)
build/mine.elf: examples/mine/mine.h examples/mine/cave.h $(LIB_INPUT) $(LIB_SAVE)
build/street.elf: examples/street/street.h $(LIB_INPUT) $(LIB_MATH) $(LIB_SAVE)
build/umbra.elf: examples/umbra/umbra.h examples/umbra/umbra.gfx examples/umbra/umbra.afb gfxfile.c include/zxv_gfxfile.h \
                 $(wildcard examples/umbra/*.pt3) $(MUSIC_SRC) music/music_int.h include/zxv_music.h \
                 $(LIB_INPUT) $(LIB_SAVE)
build/keep.elf: examples/keep/keep.h examples/keep/keep.gfx examples/keep/keep.afb gfxfile.c include/zxv_gfxfile.h \
                $(wildcard examples/keep/*.pt3) $(MUSIC_SRC) music/music_int.h include/zxv_music.h \
                $(LIB_INPUT) $(LIB_SAVE)

build/player.elf: $(MUSIC_SRC) music/music_int.h include/zxv_music.h

# Gone Silent, la aventura en 3D: va con -O2, que el 3D lo nota.
build/gonesilent.elf: CFLAGS += -O2
build/gonesilent.elf: examples/gonesilent/gonesilent.h examples/gonesilent/gonesilent.afb build/gonesilent.spr \
                      $(wildcard examples/gonesilent/*.pt3) $(MUSIC_SRC) music/music_int.h include/zxv_music.h \
                      $(LIB_INPUT) $(LIB_MATH) $(LIB_SAVE)
# Sus sprites, comprimidos (los de gonesilent.gfx, de los que solo se dibuja
# lo que no es transparente).
build/gonesilent.spr: examples/gonesilent/gonesilent.gfx examples/gonesilent/mkspr.py | build
	"$(PYTHON)" examples/gonesilent/mkspr.py $< $@
build/gfxedit.elf: examples/gfxedit/gfxedit.h examples/gfxedit/store.h

# Las piezas y el puntero del ajedrez se dibujan en PNG: chess.gfx se hace con ellos.
build/chess.elf: examples/chess/chess.h build/chess.gfx gfxfile.c include/zxv_gfxfile.h
build/chess.gfx: examples/chess/art/pieces.png examples/chess/art/pointer.png tools/png2gfx.py | build
	"$(PYTHON)" tools/png2gfx.py $@ --sprites 0 examples/chess/art/pieces.png 24,24 \
	          --sprite 12 examples/chess/art/pointer.png

build/%.zxv: build/%.elf tools/elf2zxv.py
	"$(PYTHON)" tools/elf2zxv.py $< $@

# fases: el ejemplo de los bancos (D109). Lo residente (main.c), dos bancos
# de código que comparten una zona (valle.c, espacio.c) y dos de datos (los
# fondos de mkfondos.py), todo en un .zxv con tools/zxvbanks.py.
build/fondo1.bin build/fondo2.bin &: examples/fases/mkfondos.py | build
	"$(PYTHON)" examples/fases/mkfondos.py build/fondo1.bin build/fondo2.bin
build/fases.zxv build/fases.elf &: examples/fases/main.c examples/fases/valle.c examples/fases/espacio.c examples/fases/fases.h \
                 build/fondo1.bin build/fondo2.bin crt0.S zxvlib.c zxvbank.c zxvbankcode.c zxv.ld include/*.h \
                 tools/zxvbanks.py tools/elf2zxv.py | build
	"$(PYTHON)" tools/zxvbanks.py -o build/fases.zxv --cc $(CC) --cflags "$(CFLAGS)" --sdk . --pack \
	    --code valle=examples/fases/valle.c --code espacio=examples/fases/espacio.c \
	    --bank fondo1=build/fondo1.bin --bank fondo2=build/fondo2.bin examples/fases/main.c

run: $(ZXV)
	mkdir -p $(SD)
	cp $(ZXV) $(SD)/
	$(EMU) $(if $(wildcard $(ROM)),--rom $(ROM)) --sd $(SD)

debug: $(ZXV)
	mkdir -p $(SD)
	cp $(ZXV) $(SD)/
	@echo "En otra terminal: riscv64-unknown-elf-gdb build/PROGRAMA.elf -ex 'target remote :$(GDB_PORT)'"
	$(EMU) $(if $(wildcard $(ROM)),--rom $(ROM)) --sd $(SD) --gdb $(GDB_PORT)

# Los .zxv que hace este Makefile, uno por línea: lo que va al disco ROM:
# (rom/tools/romdisk.sh), y no un .zxv suelto que haya quedado en build/.
list-zxv:
	@printf '%s\n' $(ZXV)

clean:
	rm -rf build

.PRECIOUS: build/%.elf
.PHONY: all run debug clean list-zxv
