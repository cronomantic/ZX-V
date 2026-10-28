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
CFLAGS  := -march=$(MARCH) -mabi=ilp32 -Os -g -Wall -Wextra -std=gnu11 \
           -ffreestanding -fno-builtin -fno-tree-loop-distribute-patterns -Iinclude $(EXTRA_CFLAGS)
LDFLAGS := -nostdlib -nostartfiles -static -T zxv.ld -Wl,--gc-sections \
           -Wl,--no-warn-rwx-segments

# Con la biblioteca estándar de C (picolibc, con zxvc.c debajo): stdio con
# ficheros, <dirent.h>, malloc, time...
LIBC_CFLAGS  := -march=$(MARCH) -mabi=ilp32 -Os -g -Wall -Wextra -std=gnu11 --specs=picolibc.specs -Iinclude
LIBC_LDFLAGS := -nostartfiles -static -T zxv.ld -Wl,--gc-sections \
                -Wl,--no-warn-rwx-segments

EXAMPLES := hello graphics scroll digital estela boardtest route128 mine street
# Sin picolibc (MSYS2 no la tiene), sin estos: lo dice y compila los demás.
LIBC_EXAMPLES := $(if $(filter /% %:/%,$(subst \,/,$(shell $(CC) -print-file-name=picolibc.specs))),files)
ifeq ($(LIBC_EXAMPLES),)
$(info Sin picolibc: no se compila files)
endif
ZXV      := $(EXAMPLES:%=build/%.zxv) $(LIBC_EXAMPLES:%=build/%.zxv)
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

# Los de la biblioteca de C: zxvc.c en vez de zxvlib.c.
define LIBC_RULE
build/$(1).elf: $(wildcard examples/$(1)/*.c) crt0.S zxvc.c zxv.ld $(wildcard include/*.h include/sys/*.h) | build
	$$(CC) $$(LIBC_CFLAGS) $$(LIBC_LDFLAGS) crt0.S $$(filter %.c,$$^) -o $$@
endef
$(foreach e,$(LIBC_EXAMPLES),$(eval $(call LIBC_RULE,$(e))))

build/estela.elf: examples/estela/estela.h
build/route128.elf: examples/route128/route128.h
build/mine.elf: examples/mine/mine.h examples/mine/cave.h
build/street.elf: examples/street/street.h

build/%.zxv: build/%.elf tools/elf2zxv.py
	$(PYTHON) tools/elf2zxv.py $< $@

run: $(ZXV)
	mkdir -p $(SD)
	cp $(ZXV) $(SD)/
	$(EMU) $(if $(wildcard $(ROM)),--rom $(ROM)) --sd $(SD)

debug: $(ZXV)
	mkdir -p $(SD)
	cp $(ZXV) $(SD)/
	@echo "En otra terminal: riscv64-unknown-elf-gdb build/PROGRAMA.elf -ex 'target remote :$(GDB_PORT)'"
	$(EMU) $(if $(wildcard $(ROM)),--rom $(ROM)) --sd $(SD) --gdb $(GDB_PORT)

clean:
	rm -rf build

.PRECIOUS: build/%.elf
.PHONY: all run debug clean
