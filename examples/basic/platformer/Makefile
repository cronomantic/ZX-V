# Platformer: un juego de ejemplo en BASIC (ver README.md).
#
#   make            build/platformer.gfx y el programa en build/
#   make run        abre el emulador con build/ como tarjeta SD
#
# Con el emulador compilado aquí (cmake --build emulator/build), basic_host
# graba el programa en build/platformer.tap, que arranca solo; sin él, el
# programa va como listado en texto, build/platformer.bas, que LOAD carga igual
# (y luego RUN). Para los gráficos basta con Python 3.
ROOT       ?= ../../..
BASIC_HOST ?= $(ROOT)/emulator/build/tests/basic_host
ZXV   ?= $(firstword $(wildcard $(ROOT)/emulator/build/zxv $(ROOT)/emulator/build/zxv-sdl) zxv)
ROM        ?= $(ROOT)/rom/build/rom.bin
PYTHON     ?= python3

ifneq ($(wildcard $(BASIC_HOST) $(BASIC_HOST).exe),)
PROGRAM := build/platformer.tap
TYPE    := load "platformer"\n
else
PROGRAM := build/platformer.bas
TYPE    := load "platformer"\nrun\n
endif

all: $(PROGRAM) build/platformer.gfx

build:
	mkdir -p build

# Los tiles van primero, en su orden: el nivel reaprovecha los mismos y el
# programa sabe qué número tiene cada uno.
build/platformer.gfx: tiles.png sprites.png level.png | build
	$(PYTHON) $(ROOT)/sdk/tools/png2gfx.py $@ --tiles 8 0 tiles.png \
	    --sprites 0 sprites.png 16,16 --level 0 8 level.png

# basic_host teclea el programa y lo graba en un .TAP que arranca solo; si
# alguna línea tiene un error, la escribe (y se para aquí).
build/platformer.tap: platformer.bas | build
	@out=$$($(BASIC_HOST) --tap $@ platformer.bas); \
	if [ -n "$$out" ]; then echo "$$out"; rm -f $@; exit 1; fi

build/platformer.bas: platformer.bas | build
	cp $< $@

run: all
	$(ZXV) $(if $(wildcard $(ROM)),--rom $(ROM)) --sd build --type '$(TYPE)'

clean:
	rm -rf build

.PHONY: all run clean
