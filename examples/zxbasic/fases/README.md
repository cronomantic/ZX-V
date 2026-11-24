**English** · [Español](README.es.md)

# Fases

A small example of **code banks** in ZX Basic: two stages, the
valley (three bouncing balls) and space (stars and a ship), each one in its
own file and its own bank. The functions of one stage are not in memory at the
same time as those of the other: they are loaded from the `.zxv` itself the
first time one of them is called, into the same region of memory. The program
calls them as usual. The stages take turns every 8 seconds or when a key is
pressed.

| File | Contents |
|---|---|
| `fases.bas` | The program: the loop, the titles and the header |
| `valle.bas` | Stage 1, bank `valle` |
| `espacio.bas` | Stage 2, bank `espacio` |

It is one program, not two chained programs: only the code of the functions
goes to the banks. The variables of each stage (the `DIM`s outside its
functions) stay in the resident part, like those of `fases.bas`: the whole
program sees them and they are not lost when the other bank loads.

## Building it

You need the ZX Basic fork (`cronomantic/zxbasic`), v1.19.0-zxv.7 or later
(the one with `--code-bank`), and the RISC-V C compiler, the same one the SDK uses:

```
zxbc --arch zxv --code-bank valle=valle.bas --code-bank espacio=espacio.bas fases.bas
```

That makes `fases.zxv`: copy it to the card and load it with `LOAD "fases"`. It
needs ZX-V 0.11.0 or later (the ROM with banks).
