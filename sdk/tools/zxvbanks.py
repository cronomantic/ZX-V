#!/usr/bin/env python3
"""
Compila un programa .ZXV con bancos de código (D109, zxv_bank.h): funciones
que no están en la memoria a la vez, sino que se cargan del propio fichero
cuando se las llama.

    zxvbanks.py -o SALIDA.zxv [--cc GCC] [--cflags "..."] [--sdk CARPETA]
                RESIDENTE.c ... --code NOMBRE[@ZONA]=A.c,B.c ... [--code ...]
                [--bank NOMBRE=FICHERO ...] [--pack] [--name ...] [--screen ...]

Lo residente (lo que está siempre) va en la RAM como siempre. Cada banco de
código se compila aparte y se enlaza en su zona (0 por defecto; hasta 8): una
zona mide lo que su banco más grande, y va detrás del BSS. Lo que se hace:

  1. Cada banco, compilado (zxvbank_NOMBRE-N.o). Sus funciones públicas se
     renombran (__zxvbank_real_F), y por cada una hay un «puente» F en la
     parte residente que carga el banco si hace falta y salta a ella: el
     programa llama como siempre, y un puntero a F (tomado donde sea menos en
     el propio banco) apunta al puente. Sus secciones pasan a llamarse
     .zxvbank_in.NOMBRE.*: así solo van a su zona, y lo residente (con el
     zxv.ld de siempre) no las coge, se llamen como se llamen los ficheros.
  2. Un banco no exporta variables (hazlas static, o ponlas en la parte
     residente): se borrarían al cargar otro banco. Ni variables de hilo, ni
     en la VRAM oculta (ZXV_VRAM0_DATA, ZXV_VRAM1_DATA): esas, en la parte
     residente, donde tampoco ocupan RAM de usuario.
  3. zxv.ld con las zonas (OVERLAY) en su sitio, el enlace (con crt0.S,
     zxvlib.c, zxvbank.c y zxvbankcode.c) y elf2zxv.py, que saca cada banco
     del ELF y lo pone al final del .zxv (con los de datos de --bank).

Deja SALIDA.elf al lado (para depurar) y lo de en medio en SALIDA.bancos/.
Para GDB, que solo sabe tener una función por dirección, también los
símbolos de lo residente (SALIDA.bancos/gdb/residente.elf, con los puentes
llamados F.puente) y los de cada banco (SALIDA.bancos/gdb/BANCO.elf), que
tools/zxvbanks-gdb.py añade y quita según se cargan. MIT, como el SDK.
"""
import argparse
import os
import re
import shlex
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SDK = os.path.dirname(HERE)
REGIONS_MAX = 8
NAME_MAX = 15
NEST_MAX = 32                           # llamadas a bancos anidadas
SLOT = 48                               # lo que guarda un puente: ra, banco, función, a0-a7, sp
LMA = 0x80000000                        # dónde «se cargan» en el ELF (no en la RAM)
IN_PREFIX = ".zxvbank_in."              # las secciones de un banco: .zxvbank_in.NOMBRE.text...
CFLAGS = ("-march=rv32imac_zicsr_zifencei_zba_zbb_zbs_zbkb_zcb -mabi=ilp32 -Os -g -Wall -Wextra -std=gnu11 "
          "-ffreestanding -fno-builtin -fno-tree-loop-distribute-patterns")

def system_spanish():
    """¿Es español el idioma del sistema? El de LC_ALL, LC_MESSAGES o LANG
    o, sin ellas, en Windows el del usuario (como elf2zxv.py; sin
    locale.getdefaultlocale, que Python quita en la 3.15)."""
    for var in ("LC_ALL", "LC_MESSAGES", "LANG"):
        v = os.environ.get(var)
        if v:
            return v[:2].lower() == "es"
    if sys.platform == "win32":
        try:
            import ctypes
            buf = ctypes.create_unicode_buffer(85)
            if ctypes.windll.kernel32.GetUserDefaultLocaleName(buf, len(buf)):
                return buf.value[:2].lower() == "es"
        except (AttributeError, OSError):
            pass
    return False


ES = system_spanish()


def tr(en, es):
    return es if ES else en


def die(msg):
    sys.exit("zxvbanks: " + msg)


def run(cmd, what, env=None):
    # La salida de gcc y de binutils, en UTF-8 (las líneas del código que
    # cita, con sus tildes), y no con la codificación del sistema (cp1252 en
    # Windows, que no puede con todo).
    r = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace", env=env)
    if r.returncode:
        sys.stderr.write(r.stdout + r.stderr)
        die(tr("{} failed", "falla {}").format(what))
    return r.stdout


def split_flags(text):
    """Las opciones de --cflags como las parte la línea de órdenes del
    sistema: en Windows, con sus reglas (las \\ de las rutas se quedan; las
    comillas agrupan), y en los demás, como la shell."""
    if sys.platform != "win32":
        return shlex.split(text)
    if not text.strip():
        return []
    import ctypes
    from ctypes import wintypes
    to_argv = ctypes.windll.shell32.CommandLineToArgvW
    to_argv.argtypes = [wintypes.LPCWSTR, ctypes.POINTER(ctypes.c_int)]
    to_argv.restype = ctypes.POINTER(wintypes.LPWSTR)
    n = ctypes.c_int()
    argv = to_argv("x " + text, ctypes.byref(n))       # el primero sería el programa (otras reglas)
    if not argv:
        die("--cflags: " + text)
    try:
        return [argv[i] for i in range(1, n.value)]
    finally:
        ctypes.windll.kernel32.LocalFree(argv)


def tool(cc, name):
    """objcopy y nm, los del mismo compilador (riscv64-unknown-elf-gcc ->
    riscv64-unknown-elf-objcopy)."""
    m = re.match(r"(.*)gcc(\.exe)?$", cc)
    return (m.group(1) + name + (m.group(2) or "")) if m else name


def parse_code(spec):
    name, sep, files = spec.partition("=")
    region = 0
    if "@" in name:
        name, _, r = name.partition("@")
        if not r.isdigit() or int(r) >= REGIONS_MAX:
            die(tr("--code {}: the region goes from 0 to {}", "--code {}: la zona va de 0 a {}")
                .format(spec, REGIONS_MAX - 1))
        region = int(r)
    if not sep or not files:
        die(tr("--code {}: it must be NAME[@REGION]=A.c,B.c", "--code {}: tiene que ser NOMBRE[@ZONA]=A.c,B.c")
            .format(spec))
    if not re.fullmatch(r"[A-Za-z0-9_]{1,%d}" % NAME_MAX, name):
        die(tr("bank {!r}: 1 to {} letters, digits or _", "banco {!r}: de 1 a {} letras, cifras o _")
            .format(name, NAME_MAX))
    return name, region, [f for f in files.split(",") if f]


def symbols(nm, obj):
    """(nombre, tipo) de lo definido y global en obj, y las secciones."""
    out = []
    for line in run([nm, "--defined-only", "-g", obj], "nm").splitlines():
        parts = line.split()
        if len(parts) == 3:
            out.append((parts[2], parts[1]))
    return out


def sections(objdump, obj):
    names = []
    for line in run([objdump, "-h", obj], "objdump").splitlines():
        parts = line.split()
        if len(parts) >= 2 and parts[0].isdigit():
            names.append(parts[1])
    return names


CALLS = {"R_RISCV_CALL", "R_RISCV_CALL_PLT", "R_RISCV_JAL", "R_RISCV_BRANCH", "R_RISCV_RVC_JUMP",
         "R_RISCV_RVC_BRANCH", "R_RISCV_RELAX", "R_RISCV_ALIGN"}


def pointers(objdump, readelf, obj):
    """Las funciones del propio objeto de las que se toma la dirección (no
    llamadas): un puntero así no pasa por el puente."""
    funcs = set()
    for line in run([readelf, "-sW", obj], "readelf").splitlines():
        p = line.split()
        if len(p) >= 8 and p[3] == "FUNC" and p[6] != "UND":
            funcs.add(p[7])
    out, sec = set(), ""
    for line in run([objdump, "-r", obj], "objdump").splitlines():
        if line.startswith("RELOCATION RECORDS FOR ["):
            sec = line[len("RELOCATION RECORDS FOR ["):-2]
            continue
        p = line.split()
        if sec.startswith(".debug") or len(p) != 3 or p[1] in CALLS:
            continue
        target = p[2].split("+")[0].split("-")[0]
        if target in funcs:
            out.add(target)
    return sorted(out)


def linker_script(banks, out):
    """zxv.ld con las zonas en ZXVBANKS."""
    base = open(os.path.join(SDK, "zxv.ld"), encoding="utf-8").read()
    if "/* ZXVBANKS */" not in base:
        die(tr("the SDK's zxv.ld has no ZXVBANKS mark", "el zxv.ld del SDK no tiene la marca ZXVBANKS"))
    parts = []
    for r in sorted({b[1] for b in banks}):
        members = [b for b in banks if b[1] == r]
        lines = ["    OVERLAY ALIGN(16) : NOCROSSREFS AT (0x%08X) {" % (LMA + r * 0x01000000)]
        for name, _, _ in members:
            pre = IN_PREFIX + name
            lines.append("        .zxvbank.%d.%s {" % (r, name))
            for sec in (".text .text.*", ".rodata .rodata.* .srodata .srodata.*", ".data .data.* .sdata .sdata.*",
                        ".bss .bss.* .sbss .sbss.*"):
                lines.append("            *(%s)" % " ".join(pre + x for x in sec.split()))
            lines.append("            *(%s.*)" % pre)        # lo demás del banco, también aquí
            lines.append("            . = ALIGN(4);")
            lines.append("        }")
        lines.append("    } > URAM")
        # Dónde ha quedado de verdad (lo de la tabla de los puentes): la de
        # su primer banco; y hasta donde llega el más grande.
        lines.append("    __zxvbank_region%d = ABSOLUTE(ADDR(.zxvbank.%d.%s));" % (r, r, members[0][0]))
        lines.append("    __zxvbank_region%d_end = ABSOLUTE(.);" % r)
        # Lo de la tabla de GDB (_ovly_table): cada banco, dónde va, lo que
        # mide y dónde está en el ELF.
        for name, _, _ in members:
            i = [b[0] for b in banks].index(name)
            sec = ".zxvbank.%d.%s" % (r, name)
            lines.append("    __zxvbank_vma%d = ABSOLUTE(ADDR(%s)); __zxvbank_size%d = SIZEOF(%s); __zxvbank_lma%d = LOADADDR(%s);"
                         % (i, sec, i, sec, i, sec))
        parts.append("\n".join(lines))
    text = base.replace("/* ZXVBANKS */", "/* ZXVBANKS (zxvbanks.py) */\n" + "\n".join(parts))
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def debug_frame():
    """La CFI de __zxvbank_call, para que GDB siga la pila a través de él:
    el sp no cambia (el CFA es sp), y ra, mientras está guardado, está en el
    hueco de la pila de los puentes que tiene ese mismo sp (el último que lo
    tenga: así vale también con un banco que llama a otro). Va escrita a
    mano: la expresión necesita la dirección de __zxvbank_sp, y .cfi_escape
    no admite reubicaciones."""
    loop = [
        "    .byte 0x08, %d                      /* 1: DW_OP_const1u SLOT */" % SLOT,
        "    .byte 0x1c                          /* DW_OP_minus: el hueco de abajo */",
        "    .byte 0x12                          /* DW_OP_dup */",
        "    .byte 0x03                          /* DW_OP_addr __zxvbank_stack */",
        "    .4byte __zxvbank_stack",
        "    .byte 0x2d                          /* DW_OP_lt: por debajo de la pila: no está */",
        "    .byte 0x28                          /* DW_OP_bra 2f */",
        "    .2byte 15",
        "    .byte 0x12                          /* DW_OP_dup */",
        "    .byte 0x23, 44                      /* DW_OP_plus_uconst 44: su sp */",
        "    .byte 0x06                          /* DW_OP_deref */",
        "    .byte 0x15, 2                       /* DW_OP_pick 2: el CFA */",
        "    .byte 0x2e                          /* DW_OP_ne */",
        "    .byte 0x28                          /* DW_OP_bra 1b */",
        "    .2byte -23",
        "    .byte 0x16, 0x13                    /* DW_OP_swap, DW_OP_drop: el hueco (su ra) */",
        "    .byte 0x2f                          /* DW_OP_skip 3f */",
        "    .2byte 3",
        "    .byte 0x13, 0x13, 0x30              /* 2: DW_OP_drop, DW_OP_drop, DW_OP_lit0 */",
    ]
    return [
        "/* La CFI de __zxvbank_call (ver debug_frame() en zxvbanks.py). */",
        "    .section .debug_frame, \"\", @progbits",
        "    .balign 4",
        ".Lzxvbank_cie:",
        "    .4byte .Lzxvbank_cie_end - .Lzxvbank_cie_start",
        ".Lzxvbank_cie_start:",
        "    .4byte 0xffffffff                   /* un CIE */",
        "    .byte 3                             /* versión */",
        "    .byte 0                             /* sin aumento */",
        "    .uleb128 1                          /* alineación del código */",
        "    .sleb128 -4                         /* alineación de los datos */",
        "    .uleb128 1                          /* la dirección de vuelta: ra */",
        "    .byte 0x0c, 2, 0                    /* DW_CFA_def_cfa sp, 0 */",
        "    .balign 4",
        ".Lzxvbank_cie_end:",
        "    .4byte .Lzxvbank_fde_end - .Lzxvbank_fde_start",
        ".Lzxvbank_fde_start:",
        "    .4byte .Lzxvbank_cie",
        "    .4byte __zxvbank_call",
        "    .4byte .Lzxvbank_call_end - __zxvbank_call",
        "    .byte 0x04                          /* DW_CFA_advance_loc4: ra, guardado */",
        "    .4byte .Lzxvbank_saved - __zxvbank_call",
        "    .byte 0x10, 1, 37                   /* DW_CFA_expression ra, 37 bytes: */",
        "    .byte 0x03                          /* DW_OP_addr __zxvbank_sp */",
        "    .4byte __zxvbank_sp",
        "    .byte 0x06                          /* DW_OP_deref: encima del último hueco */",
    ] + loop + [
        "    .byte 0x04                          /* DW_CFA_advance_loc4: ra, recuperado */",
        "    .4byte .Lzxvbank_restored - .Lzxvbank_saved",
        "    .byte 0xc1                          /* DW_CFA_restore ra */",
        "    .balign 4",
        ".Lzxvbank_fde_end:",
        "",
    ]


def bridges(banks, funcs, out):
    """Los puentes, la llamada común y las tablas (zxvbankcode.c)."""
    regions = sorted({b[1] for b in banks})
    L = ["/* Generado por zxvbanks.py: los puentes de los bancos de código (D109). */",
         "    .section .text.zxvbank, \"ax\"",
         "    .balign 4",
         "    .option push",
         "    .option norelax                     /* las distancias de .debug_frame, fijas */",
         "",
         "/* t5: el banco; t6: la función. La pila del programa no se toca (los",
         " * argumentos que van en ella siguen en su sitio): lo del puente va en",
         " * __zxvbank_stack, con las interrupciones quitadas solo al apilar y al",
         " * desapilar (un gancho del frame que llame a un puente no lo pisa). */",
         "    .globl __zxvbank_call",
         "__zxvbank_call:",
         "    csrrci t3, mstatus, 8",
         "    la t0, __zxvbank_sp",
         "    lw t1, 0(t0)",
         "    addi t2, t1, %d" % SLOT,
         "    la t4, __zxvbank_stack_top",
         "    bleu t2, t4, 1f",
         "    andi t3, t3, 8",
         "    csrs mstatus, t3",
         "    tail __zxvbank_overflow",
         "1:  sw t2, 0(t0)",
         "    andi t3, t3, 8",
         "    csrs mstatus, t3",
         "    sw ra, 0(t1)",
         "    sw sp, 44(t1)                       /* para que GDB encuentre su hueco */",
         ".Lzxvbank_saved:",
         "    sw t5, 4(t1)",
         "    sw t6, 8(t1)"]
    for i in range(8):
        L.append("    sw a%d, %d(t1)" % (i, 12 + 4 * i))
    L += ["    mv a0, t5",
          "    call __zxvbank_enter",
          "    la t0, __zxvbank_sp",
          "    lw t1, 0(t0)",
          "    addi t1, t1, -%d" % SLOT,
          "    lw t6, 8(t1)"]
    for i in range(8):
        L.append("    lw a%d, %d(t1)" % (i, 12 + 4 * i))
    L += ["    jalr t6",
          "    la t0, __zxvbank_sp",
          "    lw t1, 0(t0)",
          "    addi t1, t1, -%d" % SLOT,
          "    sw a0, 12(t1)",
          "    sw a1, 16(t1)",
          "    lw a0, 4(t1)",
          "    call __zxvbank_leave",
          "    csrrci t3, mstatus, 8",
          "    la t0, __zxvbank_sp",
          "    lw t1, 0(t0)",
          "    addi t1, t1, -%d" % SLOT,
          "    lw ra, 0(t1)",
          ".Lzxvbank_restored:",
          "    lw a0, 12(t1)",
          "    lw a1, 16(t1)",
          "    sw t1, 0(t0)",
          "    andi t3, t3, 8",
          "    csrs mstatus, t3",
          "    ret",
          ".Lzxvbank_call_end:",
          ""]
    for fname, bank in funcs:
        L += ["    .globl %s" % fname,
              "    .type %s, @function" % fname,
              "%s:" % fname,
              "    li t5, %d" % bank,
              "    la t6, __zxvbank_real_%s" % fname,
              "    tail __zxvbank_call",
              ""]
    L += ["    .option pop", ""]
    L += debug_frame()
    L += ["    .section .rodata.zxvbank, \"a\"",
          "    .balign 4",
          "    .globl __zxvbank_nbanks, __zxvbank_banks, __zxvbank_nregions, __zxvbank_regions",
          "__zxvbank_nbanks: .word %d" % len(banks),
          "__zxvbank_banks:"]
    for i, (name, region, _) in enumerate(banks):
        L.append("    .word __zxvbank_name%d, %d" % (i, region))
    L.append("__zxvbank_nregions: .word %d" % (max(regions) + 1))
    L.append("__zxvbank_regions:")
    for r in range(max(regions) + 1):
        if r in regions:
            L.append("    .word __zxvbank_region%d, __zxvbank_region%d_end" % (r, r))
        else:
            L.append("    .word 0, 0")
    for i, (name, _, _) in enumerate(banks):
        L.append("__zxvbank_name%d: .asciz \"%s\"" % (i, name))
    L += ["",
          "    .section .bss.zxvbank, \"aw\", @nobits",
          "    .balign 4",
          "    .globl __zxvbank_stack, __zxvbank_stack_top",
          "__zxvbank_stack: .space %d" % (NEST_MAX * SLOT),
          "__zxvbank_stack_top:",
          "",
          "    .section .data.zxvbank, \"aw\"",
          "    .balign 4",
          "    .globl __zxvbank_sp",
          "__zxvbank_sp: .word __zxvbank_stack",
          "",
          "/* La tabla de los bancos para GDB (overlay auto) y el emulador: por",
          " * banco, dónde va, lo que mide, dónde está en el ELF y si está cargado. */",
          "    .globl _novlys, _ovly_table",
          "_novlys: .word %d" % len(banks),
          "_ovly_table:"]
    for i in range(len(banks)):
        L.append("    .word __zxvbank_vma%d, __zxvbank_size%d, __zxvbank_lma%d, 0" % (i, i, i))
    L += [""]
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(L))


def main():
    for stream in (sys.stdout, sys.stderr):             # lo que no se pueda escribir en la consola, con ?
        try:
            stream.reconfigure(errors="replace")
        except (AttributeError, ValueError):
            pass
    ap = argparse.ArgumentParser(description=tr("Builds a .ZXV program with code banks (D109).",
                                                "Compila un programa .ZXV con bancos de código (D109)."))
    ap.add_argument("-o", dest="out", required=True, help=tr("the .zxv to write", "el .zxv que se escribe"))
    ap.add_argument("--cc", default="riscv64-unknown-elf-gcc", help=tr("the compiler", "el compilador"))
    ap.add_argument("--cflags", default=CFLAGS, help=tr("compiler options (by default, the SDK's)",
                                                         "opciones del compilador (si no, las del SDK)"))
    ap.add_argument("--sdk", default=SDK, help=tr("the SDK folder", "la carpeta del SDK"))
    ap.add_argument("--code", action="append", default=[], metavar=tr("NAME[@REGION]=A.c,B.c", "NOMBRE[@ZONA]=A.c,B.c"),
                    help=tr("a code bank and its files (repeatable)", "un banco de código y sus ficheros (se puede repetir)"))
    ap.add_argument("--bank", action="append", default=[], metavar=tr("NAME=FILE", "NOMBRE=FICHERO"),
                    help=tr("a data bank (repeatable)", "un banco de datos (se puede repetir)"))
    ap.add_argument("--pack", action="store_true", help=tr("compress the banks", "comprime los bancos"))
    ap.add_argument("--name")
    ap.add_argument("--mode")
    ap.add_argument("--screen")
    ap.add_argument("sources", nargs="+", help=tr("the resident files (.c, .S)", "los ficheros residentes (.c, .S)"))
    a = ap.parse_args()

    banks = [parse_code(s) for s in a.code]
    if not banks:
        die(tr("no --code: for a program without code banks use elf2zxv.py",
               "sin --code: para un programa sin bancos de código, elf2zxv.py"))
    names = [b[0] for b in banks]
    if len(set(names)) != len(names):
        die(tr("a bank name is repeated", "un nombre de banco está repetido"))
    sdk = a.sdk
    base = os.path.splitext(a.out)[0]
    work = base + ".bancos"
    os.makedirs(work, exist_ok=True)
    cflags = split_flags(a.cflags) + ["-I" + os.path.join(sdk, "include")]
    nm, objcopy, objdump, readelf = (tool(a.cc, t) for t in ("nm", "objcopy", "objdump", "readelf"))

    # 1. Los bancos, compilados y con sus funciones públicas renombradas.
    funcs, objs, owner = [], [], {}
    for i, (name, region, files) in enumerate(banks):
        bank_objs = []
        for n, src in enumerate(files):
            obj = os.path.join(work, "zxvbank_%s-%d.o" % (name, n))
            run([a.cc] + cflags + ["-c", src, "-o", obj], tr("compiling {}", "compilar {}").format(src))
            for sec in sections(objdump, obj):
                if sec.startswith((".tdata", ".tbss")):
                    die(tr("bank {}: {} has thread variables", "banco {}: {} tiene variables de hilo").format(name, src))
                if sec.startswith((".bss.zxv_vram0", ".bss.zxv_vram1")):
                    die(tr("bank {}: {} puts variables in the hidden VRAM (ZXV_VRAM0_DATA, ZXV_VRAM1_DATA): declare "
                           "them in the resident part, where they take no user RAM either",
                           "banco {}: {} pone variables en la VRAM oculta (ZXV_VRAM0_DATA, ZXV_VRAM1_DATA): "
                           "decláralas en la parte residente, donde tampoco ocupan RAM de usuario").format(name, src))
            for sym, kind in symbols(nm, obj):
                if kind in "TtW" and kind != "t":
                    if sym in owner:
                        die(tr("{} is in banks {} and {}", "{} está en los bancos {} y {}").format(sym, owner[sym], name))
                    owner[sym] = name
                    funcs.append((sym, i))
                elif kind in "DdBbRrCGgSsVv" and kind.isupper():
                    die(tr("bank {}: {} exports the variable {} (make it static, or put it in the resident part)",
                           "banco {}: {} exporta la variable {} (hazla static, o ponla en la parte residente)")
                        .format(name, src, sym))
            for fn in pointers(objdump, readelf, obj):
                print(tr("zxvbanks: warning: bank {} takes the address of its function {} ({}): use that pointer "
                         "only while {} is loaded; with another bank in its region it would run that bank's code",
                         "zxvbanks: aviso: el banco {} toma la dirección de su función {} ({}): usa ese puntero solo "
                         "mientras {} esté cargado; con otro banco en su zona ejecutaría el código de ese")
                      .format(name, fn.replace("__zxvbank_real_", ""), src, name))
            bank_objs.append(obj)
        objs += bank_objs
    for obj in objs:
        mine = owner_of(obj, banks)
        args = ["--prefix-alloc-sections=" + IN_PREFIX + mine]
        for sym, i in funcs:
            if banks[i][0] == mine:
                args += ["--redefine-sym", "%s=__zxvbank_real_%s" % (sym, sym)]
        run([objcopy] + args + [obj], "objcopy")

    # 2. Lo residente, los puentes (sin líneas: «step» no entra en ellos)
    # y el enlace.
    bridges_s = os.path.join(work, "bridges.S")
    bridges(banks, funcs, bridges_s)
    bridges_o = os.path.join(work, "bridges.o")
    run([a.cc] + [f for f in cflags if not f.startswith("-g")] + ["-c", bridges_s, "-o", bridges_o],
        tr("assembling the bridges", "ensamblar los puentes"))
    ld = os.path.join(work, "zxv.ld")
    linker_script(banks, ld)
    resident = [os.path.join(sdk, "crt0.S")] + [s for s in a.sources if s != os.path.join(sdk, "crt0.S")] + \
        [os.path.join(sdk, f) for f in ("zxvlib.c", "zxvbank.c", "zxvbankcode.c")]
    res_objs = []
    for n, src in enumerate(resident):
        obj = os.path.join(work, "res%d-%s.o" % (n, os.path.splitext(os.path.basename(src))[0]))
        run([a.cc] + cflags + ["-c", src, "-o", obj], tr("compiling {}", "compilar {}").format(src))
        res_objs.append(obj)

    def link(inputs, out):
        run([a.cc] + cflags + ["-nostdlib", "-nostartfiles", "-static", "-T", ld, "-Wl,--gc-sections",
                               "-Wl,--no-warn-rwx-segments"] + inputs + ["-lgcc", "-o", out], tr("linking", "enlazar"))

    elf = base + ".elf"
    link(res_objs + [bridges_o] + objs, elf)

    # 3. Para GDB, que solo puede tener la información de depuración de un
    # banco por dirección: lo residente sin los bancos y cada banco aparte
    # (tools/zxvbanks-gdb.py los añade y los quita según se cargan).
    gdb_files(a, banks, objs, res_objs, bridges_o, elf, work, link, objcopy, readelf)

    # 4. El .zxv, con los bancos de código (del ELF) y los de datos.
    cmd = [sys.executable, os.path.join(HERE, "elf2zxv.py"), elf, a.out]
    for b in a.bank:
        cmd += ["--bank", b]
    if a.pack:
        cmd.append("--pack")
    for opt in ("name", "mode", "screen"):
        if getattr(a, opt):
            cmd += ["--" + opt, getattr(a, opt)]
    sys.stdout.write(run(cmd, "elf2zxv", env=dict(os.environ, PYTHONIOENCODING="utf-8")))
    return 0


def elf_sections(readelf, elf):
    """{índice: (nombre, dirección, tamaño, ocupa memoria)} de las secciones."""
    out = {}
    for line in run([readelf, "-SW", elf], "readelf").splitlines():
        m = re.match(r"\s*\[\s*(\d+)\]\s+(\S+)\s+\S+\s+([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)\s+\S+\s+(\S*)", line)
        if m and m.group(1) != "0":
            out[int(m.group(1))] = (m.group(2), int(m.group(3), 16), int(m.group(4), 16), "A" in m.group(5))
    return out


def elf_symbols(readelf, elf):
    """[(nombre, índice de su sección)] de los símbolos con nombre."""
    out = []
    for line in run([readelf, "-sW", elf], "readelf").splitlines():
        p = line.split()
        if len(p) == 8 and p[0].endswith(":") and p[6].isdigit():
            out.append((p[7], int(p[6])))
    return out


def gdb_files(a, banks, objs, res_objs, bridges_o, elf, work, link, objcopy, readelf):
    """SALIDA.bancos/gdb/residente.elf (sin nada de los bancos, y con los
    puentes renombrados F.puente: «break F» es la función de verdad) y
    SALIDA.bancos/gdb/BANCO.elf (solo lo del banco). Mismas direcciones que
    SALIDA.elf: se enlaza igual, con la información de depuración quitada
    de lo que no va."""
    gdir = os.path.join(work, "gdb")
    os.makedirs(gdir, exist_ok=True)
    bare = {}
    for obj in res_objs + objs + [bridges_o]:
        b = os.path.join(work, os.path.basename(obj)[:-2] + ".nodebug.o")
        run([objcopy, "--strip-debug", obj, b], "objcopy")
        bare[obj] = b
    want = {s: v[1:] for s, v in elf_sections(readelf, elf).items() if v[3]}

    def same_layout(out):
        got = {s: v[1:] for s, v in elf_sections(readelf, out).items() if v[3]}
        if got != want:
            die(tr("{}: not the same layout as {}", "{}: no queda igual que {}").format(out, elf))

    def strip(out, keep):
        secs = elf_sections(readelf, out)
        drop = [n for n, s in elf_symbols(readelf, out) if s in secs and not keep(secs[s][0], n)]
        lst = out + ".quitar"
        with open(lst, "w", encoding="utf-8") as f:
            f.write("\n".join(sorted(set(drop))) + "\n")
        run([objcopy, "--strip-symbols=" + lst, out], "objcopy")
        os.remove(lst)

    # Lo residente: los bancos, sin depuración ni símbolos.
    out = os.path.join(gdir, "residente.elf")
    link(res_objs + [bridges_o] + [bare[o] for o in objs], out)
    same_layout(out)
    strip(out, lambda sec, name: not sec.startswith(".zxvbank."))
    names = sorted({owner_sym for owner_sym, _ in elf_symbols(readelf, out)})
    bridges_named = [f for f in names if "__zxvbank_real_" + f in
                     {n for n, _ in elf_symbols(readelf, elf)}]
    if bridges_named:
        args = []
        for f in bridges_named:
            args += ["--redefine-sym", "%s=%s.puente" % (f, f)]
        run([objcopy] + args + [out], "objcopy")

    # Cada banco: solo lo suyo.
    for name, region, _ in banks:
        sec_name = ".zxvbank.%d.%s" % (region, name)
        out = os.path.join(gdir, name + ".elf")
        mine = [o for o in objs if owner_of(o, banks) == name]
        link([bare[o] for o in res_objs] + [bare[bridges_o]] +
             [o if o in mine else bare[o] for o in objs], out)
        same_layout(out)
        strip(out, lambda sec, n, s=sec_name: sec == s)


def owner_of(obj, banks):
    m = re.match(r"zxvbank_(.+)-\d+\.o$", os.path.basename(obj))
    return m.group(1) if m else None


if __name__ == "__main__":
    sys.exit(main())
