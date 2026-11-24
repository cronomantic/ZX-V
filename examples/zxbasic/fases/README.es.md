[English](README.md) · **Español**

# Fases

Un ejemplo pequeño de los **bancos de código** en ZX Basic: dos
fases, el valle (tres pelotas que botan) y el espacio (estrellas y una nave),
cada una en su fichero y en su banco. Las funciones de una fase no están en la
memoria a la vez que las de la otra: se cargan del propio `.zxv` la primera vez
que se llama a una de ellas, en la misma zona de la memoria. El programa las
llama como siempre. Las fases se turnan cada 8 segundos o al pulsar una tecla.

| Fichero | Contenido |
|---|---|
| `fases.bas` | El programa: el bucle, los carteles y la cabecera |
| `valle.bas` | La fase 1, banco `valle` |
| `espacio.bas` | La fase 2, banco `espacio` |

Es un solo programa, no dos programas encadenados: a los bancos solo va el
código de las funciones. Las variables de cada fase (los `DIM` de fuera de sus
funciones) se quedan en la parte residente, como las de `fases.bas`: todo el
programa las ve y no se pierden al cargar el otro banco.

## Compilarlo

Hace falta el fork de ZX Basic (`cronomantic/zxbasic`), v1.19.0-zxv.7 o
posterior (la que trae `--code-bank`), y el compilador de C de RISC-V, el mismo que usa el SDK:

```
zxbc --arch zxv --code-bank valle=valle.bas --code-bank espacio=espacio.bas fases.bas
```

Sale `fases.zxv`: cópialo a la tarjeta y cárgalo con `LOAD "fases"`. Necesita
ZX-V 0.11.0 o posterior (la ROM con bancos).
