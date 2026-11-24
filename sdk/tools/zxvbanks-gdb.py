"""
zxvbanks-gdb.py - GDB con los bancos de código (D109): que vea en cada zona
el banco que está cargado.

    gdb-multiarch juego.elf -x sdk/tools/zxvbanks-gdb.py -ex "target remote :1234"

o, ya dentro de GDB y con el ELF abierto, «source sdk/tools/zxvbanks-gdb.py».

Los bancos de una zona están en las mismas direcciones, y GDB solo sabe
tener una función por dirección. Por eso zxvbanks.py deja, en
juego.bancos/gdb/, los símbolos de lo residente (residente.elf) y los de cada
banco (BANCO.elf), y este script:

  - usa residente.elf en vez de juego.elf;
  - lee del programa qué banco está cargado en cada zona (la tabla
    _ovly_table de zxvbankcode.c) cada vez que se carga uno (un punto de
    ruptura en _ovly_debug_event, que no para) y cada vez que el programa se
    para, y añade o quita los símbolos de ese banco (add-symbol-file,
    remove-symbol-file).

Así, «break FUNCIÓN» de un banco queda pendiente hasta que el banco se carga
y para solo en él; bt, list y print van con lo que hay de verdad. Los
puentes se llaman FUNCIÓN.puente. «zxvbanks» enseña los bancos y cuáles
están cargados. MIT, como el SDK.
"""
import os
import struct

import gdb

ENTRY = 16                              # vma, size, lma, mapped
BANKS_MAX = 4096                        # los bancos de un programa, como mucho (elf2zxv.py, zxvbank.c)
_added = {}                             # banco -> fichero de símbolos añadido
_gdir = None                            # juego.bancos/gdb


def _u32(addr):
    b = gdb.selected_inferior().read_memory(addr, 4).tobytes()
    return int.from_bytes(b, "little")


def _addr(name):
    return int(gdb.parse_and_eval("(unsigned int)&'%s'" % name))


def _string(addr, limit=32):
    b = gdb.selected_inferior().read_memory(addr, limit).tobytes()
    return b.split(b"\0", 1)[0].decode("ascii", "replace")


def _setup():
    """La carpeta de los símbolos de los bancos, y residente.elf en vez del
    ELF entero. False si el programa no tiene bancos de código."""
    global _gdir
    if _gdir:
        return True
    elf = gdb.current_progspace().filename
    if not elf:
        return False
    elf = os.path.abspath(elf)
    if os.path.basename(elf) == "residente.elf":
        gdir = os.path.dirname(elf)
    else:
        gdir = os.path.splitext(elf)[0] + ".bancos" + os.sep + "gdb"
    resident = os.path.join(gdir, "residente.elf")
    if not os.path.isfile(resident):
        return False
    if os.path.normcase(elf) != os.path.normcase(resident):
        gdb.execute("symbol-file '%s'" % resident.replace("\\", "/"), to_string=True)
    _gdir = gdir
    return True


def _banks(names=True):
    """[(nombre, vma, tamaño, cargado)] de la memoria del programa; [] si
    aún no está cargado. Con names False, el nombre solo de los cargados (el
    de los demás, None). Las tablas se leen de una vez: con miles de bancos,
    una lectura por campo serían miles de idas y vueltas con el emulador."""
    if gdb.selected_inferior().pid == 0:
        return []
    try:
        n = _u32(_addr("_novlys"))
        if not 0 < n <= BANKS_MAX or _u32(_addr("__zxvbank_nbanks")) != n:
            return []
        mem = gdb.selected_inferior()
        table = mem.read_memory(_addr("_ovly_table"), n * ENTRY).tobytes()
        banks = mem.read_memory(_addr("__zxvbank_banks"), n * 8).tobytes()
        out = []
        for i in range(n):
            vma, size, _lma, mapped = struct.unpack_from("<4I", table, i * ENTRY)
            name = _string(struct.unpack_from("<I", banks, 8 * i)[0]) if names or mapped else None
            out.append((name, vma, size, mapped))
        return out
    except (gdb.error, gdb.MemoryError):
        return []


def sync():
    """Añade los símbolos de los bancos cargados y quita los de los demás."""
    if not _setup():
        return
    banks = _banks(names=False)
    loaded = {name for name, _, _, mapped in banks if mapped}
    for name in [b for b in _added if b not in loaded]:     # primero, los que se van
        gdb.execute("remove-symbol-file '%s'" % _added.pop(name), to_string=True)
    for name in sorted(loaded - set(_added)):
        path = os.path.join(_gdir, name + ".elf").replace("\\", "/")
        if os.path.isfile(path):
            gdb.execute("add-symbol-file '%s'" % path, to_string=True)
            _added[name] = path


class _LoadEvent(gdb.Breakpoint):
    """Tras cada carga (zxvbankcode.c llama a _ovly_debug_event): pone al
    día los símbolos y sigue."""

    def __init__(self):
        super().__init__("_ovly_debug_event", internal=True)
        self.silent = True

    def stop(self):
        sync()
        return False


class _Command(gdb.Command):
    """Los bancos de código y cuáles están cargados."""

    def __init__(self):
        super().__init__("zxvbanks", gdb.COMMAND_STATUS)

    def invoke(self, arg, from_tty):
        _setup()
        banks = _banks()
        if not banks:
            print("Sin bancos de código (o el programa aún no está cargado).")
            return
        for name, vma, size, mapped in banks:
            print("%-16s %08x-%08x %s" % (name, vma, vma + size, "cargado" if mapped else "-"))


def _in_bridge():
    # Por el nombre del marco, no con «info symbol»: con MI (la extensión de
    # VS Code), gdb.execute(..., to_string=True) lo da en el formato de MI.
    try:
        return gdb.newest_frame().name() == "__zxvbank_call"
    except gdb.error:
        return False


def _on_stop(event):
    sync()
    # Una función de un banco vuelve a su puente (__zxvbank_call), que no
    # tiene líneas: «finish» sigue hasta el que la llamó.
    details = getattr(event, "details", None) or {}
    if details.get("reason") == "function-finished" and _in_bridge():
        gdb.post_event(lambda: gdb.execute("finish"))


gdb.execute("set breakpoint pending on")
_setup()
_LoadEvent()
_Command()
gdb.events.stop.connect(_on_stop)
