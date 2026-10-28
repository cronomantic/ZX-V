# El BASIC de ZX-V

**Versión 0.1 (borrador).** El intérprete, el editor a pantalla completa y las
extensiones de §9 ya funcionan: ver [§11](#11-estado-de-la-implementación).
Licencia CC BY 4.0, como el resto de la documentación ([docs/LICENSE](../docs/LICENSE)).

El BASIC de ZX-V es **compatible con el Sinclair BASIC del Spectrum 48K**, con
algunas órdenes del 128K y extensiones propias. Se ejecuta en el núcleo 0 como parte
de la ROM ZX-V (ver [spec.md](spec.md)).

---

## 1. Referencia y método

- **Referencia funcional:** *The Complete Spectrum ROM Disassembly* (Ian Logan y
  Frank O'Hara) y su versión anotada de SkoolKit.
- **Implementación:** desde cero, en C (y ensamblador RISC-V donde haga falta). Se
  reproduce el **comportamiento**, no se traduce la ROM rutina a rutina. Los
  algoritmos y formatos documentados se reproducen; el código de la ROM de Sinclair
  no se copia.
- **Fuente de caracteres:** diseño propio, con la misma disposición de 96 caracteres
  más los UDG, para que los programas que leen la fuente sigan funcionando.

## 2. Qué se reproduce exactamente

Los programas dependen de estos detalles, así que deben ser idénticos al original:

| Elemento | Detalle |
|---|---|
| `RND` y `RANDOMIZE` | Misma semilla y misma fórmula, para que las secuencias aleatorias coincidan |
| Formato de los números al imprimir | El de la rutina PRINT-FP: 8 cifras significativas, notación `E` y casos especiales |
| `PLOT`, `DRAW` (con arco) y `CIRCLE` | Los mismos algoritmos, para que en modo 0 se dibujen exactamente los mismos píxeles. El origen está abajo a la izquierda |
| `PRINT` | Separadores `,`, `;` y `'`, `TAB`, `AT`, desplazamiento de pantalla y el mensaje `scroll?` |
| Informes de error | Códigos y textos originales (`2 Variable not found, 10:1`…) |
| Tokenización | Los mismos tokens y el mismo formato de línea y de variables, para poder cargar programas `.TAP` directamente |
| Gráficos de bloques (`CHR$ 128`–`143`) | La misma numeración: bit 0 el cuarto de arriba a la derecha, bit 1 arriba a la izquierda, bit 2 abajo a la derecha y bit 3 abajo a la izquierda (129 ▝, 130 ▘, 132 ▗, 136 ▖, 143 █). Van en la tinta, y el resto de la celda en el papel |

Internamente los números son **double IEEE**, no el formato de 5 bytes. La salida
por pantalla debe coincidir; las diferencias de precisión en casos extremos se
aceptan. Para que las comparaciones den lo mismo que en el Spectrum, `=`, `<`,
`>`… (y el final de `FOR`) comparan los números redondeados a la precisión de 5
bytes: `0.1*3=0.3` es cierto, como en el original.

Los números de las líneas llevan detrás su forma oculta de 5 bytes, como en el
Spectrum. Al ejecutar, manda la forma oculta (hay programas que la retocan),
salvo que coincida con la del propio texto: entonces se usa el double, que es
más preciso.

## 3. Pantalla sin *clash*

- **Color por píxel.** `PRINT` pinta la celda de 8×8 con tinta y papel. `PLOT`,
  `DRAW` y `CIRCLE` pintan solo con tinta. Un píxel nunca cambia el color de sus
  vecinos.
- **`INK 8` y `PAPER 8`** (transparente) conservan el color existente. **`INK 9` y
  `PAPER 9`** (contraste) se mantienen.
- **`INVERSE 1`** intercambia tinta y papel. **`OVER 1`** alterna cada píxel afectado
  entre la tinta y el papel del atributo de su celda.
- **Sombra de atributos.** Se guarda el último atributo escrito en cada celda (32×24
  en modo 0, 64×48 en modo 1, o las de la página con el campo virtual), para que
  `ATTR()` funcione.
- **`FLASH 1`.** El intérprete lleva la cuenta de las celdas con FLASH e intercambia
  su tinta y su papel cada 16 frames, en las dos páginas a la vez (la que se ve
  parpadea aunque se dibuje en la otra). Lo hace entre sentencias y en las
  esperas (`PAUSE`, `WAIT`, `INPUT`), así que se para durante una sentencia
  larga como `BEEP`. Las órdenes del motor gráfico (§9.8) quitan el FLASH de
  las celdas del rectángulo en el que pintan.
- **`SCREEN$`** compara la celda con la fuente actual, como el original.
- **Coordenadas gráficas:** en modo 0, `x` va de 0 a 255 e `y` de 0 a 175 (las dos
  últimas filas son la pantalla inferior de edición e `INPUT`). En modo 1, `x` va de
  0 a 511 e `y` de 0 a 367. Con el campo virtual, de 0 al ancho de la página
  menos 1 y de 0 al alto menos 17 (por ejemplo, 1023 y 175 en 1024×192).

## 4. Mapa de memoria virtual del Spectrum

`PEEK` y `POKE` con direcciones **por debajo de 65536** acceden a un **Spectrum
virtual de 64 KB** que el intérprete traduce:

| Zona | Comportamiento |
|---|---|
| 0–16383 (ROM) | `PEEK` devuelve 0, excepto la fuente en 15616 (`0x3D00`), que contiene la fuente de ZX-V |
| 16384–22527 (pantalla) | `POKE` dibuja en la VRAM con la tinta y el papel de la sombra de atributos. `PEEK` lee una sombra de la pantalla Spectrum (6.144 B) |
| 22528–23295 (atributos) | `POKE` cambia los colores de la celda (repinta sus píxeles). `PEEK` lee la sombra de atributos |
| 23552–23733 (variables del sistema) | Se emulan las que usan los programas: `LAST K` (23560), `CHARS` (23606), `BORDCR` (23624), `PROG` (23635, siempre 23755), `FLAGS2` (23658, bloqueo de mayúsculas), `DF SZ` (23659), `SEED` (23670), `FRAMES` (23672), `UDG` (23675), `COORDS` (23677), `S POSN` (23688), `SCR CT` (23692), `ATTR P`/`MASK P`/`ATTR T`/`MASK T`/`P FLAG` (23693–23697) y `RAMTOP` (23730). El resto se guarda y se lee como memoria normal |
| 65368–65535 (UDG) | Los 21 UDG (A–U). `POKE USR "a"+n, valor` funciona |
| 23296–65535 (resto) | Memoria de respaldo de 42 KB: lo que se escribe con `POKE` se lee con `PEEK` |

- **`CHARS`** puede apuntar a una fuente propia cargada en la memoria virtual, como
  hacían muchos programas.
- Con direcciones de **65536 o más**, `PEEK` y `POKE` acceden a la **memoria real**
  de ZX-V, byte a byte. La zona reservada produce un error.

## 5. Puertos (`IN` y `OUT`)

| Puerto | Orden | Comportamiento |
|---|---|---|
| `xxFE` | `IN` | Semifilas del teclado según el byte alto (se combinan con AND si hay varias), desde `KEYROWS`. Los bits 5–7 se leen a 1 |
| `xxFE` | `OUT` | Bits 0–2: color del borde. Bit 4: beeper |
| 31 | `IN` | Joystick Kempston (desde `JOY0`) |
| 64479 / 65503 | `IN` | Ratón Kempston: X e Y, contadores de 8 bits (X a la derecha, Y hacia arriba) |
| 64223 | `IN` | Ratón Kempston: botones a 0 cuando se pulsan (bit 0 derecho, 1 izquierdo, 2 central) |
| 65533 / 49149 | `OUT` | Selección y escritura de registro del AY (128K), a través de la cola. Como en TurboSound, `OUT 65533,255` elige el primer AY y `OUT 65533,254` el segundo |
| 65533 | `IN` | Lectura del registro seleccionado del AY elegido |
| Otros | — | `IN` devuelve 255 y `OUT` se ignora |

**Emulación de joystick.** `IN 31` lee siempre el mando 0 como un Kempston.
Además, cada mando se puede presentar como teclas, como hacían el Interface 2
y los interfaces Cursor, para los programas que leen el teclado con `IN` o
`INKEY$`. Se elige con `JOY mando, modo`:

| Modo | Izquierda | Derecha | Abajo | Arriba | Fuego |
|---|---|---|---|---|---|
| 0 | (no se presenta como teclas) | | | | |
| 1 Sinclair 1 | 6 | 7 | 8 | 9 | 0 |
| 2 Sinclair 2 | 1 | 2 | 3 | 4 | 5 |
| 3 Cursor | 5 | 8 | 6 | 7 | 0 |

- Por defecto, el mando 0 es Sinclair 1 y el 1, Sinclair 2 (como los dos
  puertos del +2). Cualquiera de los tres botones de fuego cuenta.
- Solo lo ven `IN` e `INKEY$`: `KEY` y la comprobación de BREAK leen el
  teclado real, y `JOY n` (la función) da los botones del mando.

## 6. Código máquina y `USR`

- `USR dirección` con una dirección **≥ 65536** llama a código **RISC-V nativo** con
  la ABI ilp32, y devuelve el valor de `a0`.
- `USR "a"` (UDG) devuelve la dirección virtual, como el original.
- `USR` con otra dirección **< 65536** da el error `Código Z80 no admitido`. Es el
  límite de la compatibilidad: el código máquina Z80 no se ejecuta.

## 7. Ficheros

La tarjeta SD (FAT, FAT32 o exFAT) sustituye a la cinta:

Los programas se guardan como **`.TAP` del Spectrum** (cabecera y bloque de
datos): se pueden abrir en cualquier emulador del Spectrum, y los `.TAP` de
programas BASIC del Spectrum se cargan directamente.

| Orden | Efecto |
|---|---|
| `LOAD "nombre"` | Sin extensión prueba `nombre.tap`, después `nombre.zxv` (lo ejecuta; al acabar, la memoria del BASIC empieza de nuevo y el informe es `0 OK` o `W`, spec §9.2), después `nombre.bas` y por último `nombre`. Lee `.TAP` y `.TZX` (el primer programa BASIC, y la deja puesta: ver abajo). Si el programa se grabó con `LINE`, arranca solo |
| `LOAD "x" CODE [dir[, long]]` | Un bloque `CODE` de un `.TAP`, o un fichero binario en bruto (entonces `dir` es obligatoria) |
| `LOAD "x" SCREEN$` | `CODE 16384,6912`: una pantalla `.SCR` (o de un `.TAP`) que se convierte a color por píxel; se ve igual que en el Spectrum, *clash* incluido |
| `SAVE "x" [LINE n]`, `SAVE "x" CODE dir, long`, `SAVE "x" SCREEN$` | Equivalentes de grabación. Sin extensión, se añade `.tap`; con otra extensión, `CODE` y `SCREEN$` se graban en bruto |
| `LOAD "x.bas"`, `MERGE "x.bas"`, `SAVE "x.bas"` | Un listado en texto (UTF-8, LF o CR LF): cada línea se tokeniza y se comprueba como en el editor (`tokenize` y `syntax_check`, guardando `g.pc`, `g.ppc`, `g.subppc` y `g.tmp` de la orden que se ejecuta); un error para con su informe y el número de la línea del fichero. Tildes fuera (como al teclear), £ y © del Spectrum, gráficos de bloque con los cuadrantes de Unicode, y un byte que no es UTF-8 (los UDG al grabar), tal cual. Los búferes (1 + 2 KB), en la zona temporal: en el espacio de trabajo de la ROM no caben |
| `SAVE "x" DATA a()`, `SAVE "x" DATA a$()` | Una matriz de números o de caracteres, en un `.TAP` como el del Spectrum (siempre en ese formato, aunque el nombre tenga otra extensión) |
| `LOAD "x" DATA b()`, `LOAD "x" DATA b$()` | Carga la matriz de un `.TAP` con el nombre `b` (sustituye a la variable que se llame así, como `DIM`). El tipo debe coincidir |
| `LOAD ""`, `LOAD "" SCREEN$`, `LOAD "" CODE`... | Lo siguiente de ese tipo en la **cinta puesta** (ver abajo) |
| `VERIFY`, `MERGE` | Como en el Spectrum (`MERGE` no admite `DATA`) |
| `SAVE "x" GRAPHICS`, `LOAD "x" GRAPHICS`, `VERIFY "x" GRAPHICS` | El almacén de gráficos (§9.8), en un fichero `.gfx` (la extensión que se añade si no hay otra) |
| `CAT ["carpeta"]` | Lista la carpeta (la actual, sin nombre): primero la carpeta, luego cada fichero con su tamaño y las carpetas con `/` y `<DIR>`. Con comodines (`CAT "*.tap"`, `CAT "juegos/nivel?.dat"`) o el nombre de un fichero, solo lo que encaja |
| `CD "carpeta"` | Cambia de carpeta (`"/"` es la raíz de la unidad, `".."` la de arriba). `CD "ROM:"` entra en el disco ROM y `CD "SD:"` vuelve a la tarjeta |
| `DIR$` | La carpeta actual, como `"/juegos"` en la tarjeta o `"ROM:/"` en el disco ROM (función) |
| `MKDIR "carpeta"`, `RMDIR "carpeta"` | Crea una carpeta; borra una carpeta vacía |
| `ERASE "fichero"` | Borra un fichero. Con comodines (`ERASE "*.bak"`), todos los ficheros que encajan (las carpetas no) |
| `MOVE "a" TO "b"` | Cambia el nombre de un fichero o una carpeta, o lo lleva a otra carpeta: si `b` es una carpeta, va dentro con el mismo nombre. Con comodines (`MOVE "*.tap" TO "juegos"`), `b` tiene que ser una carpeta y se mueven los ficheros que encajan (las carpetas no) |
| `COPY "a" TO "b"` | Copia un fichero (si `b` existe, lo sustituye; si es una carpeta, va dentro). Con comodines (`COPY "*.tap" TO "copias"`), `b` tiene que ser una carpeta. Copiar un fichero sobre sí mismo da `V Disk error (already exists)`. `COPY` solo sigue copiando la pantalla a la impresora |
| `LPRINT`, `LLIST`, `COPY` | Van al fichero `/printer.txt` de la tarjeta, aunque se esté en el disco ROM (`COPY` reconoce el texto de la pantalla) |

**El disco ROM.** La máquina trae, además de la tarjeta, un disco de solo
lectura con los ejemplos: la unidad `ROM:` (spec §7.7). Todas las órdenes de
arriba la admiten: `CAT "ROM:"`, `CAT "ROM:*.zxv"`, `LOAD "ROM:graphics"`,
`COPY "ROM:hello.zxv" TO "hello.zxv"` (a la tarjeta), `CD "ROM:"`… `ROM:` y
`SD:` empiezan siempre en la raíz de su unidad; sin prefijo, la ruta es de la
unidad de la carpeta actual. Lo que escribe en el disco ROM (`SAVE`, `ERASE`,
`MOVE`, `MKDIR`…) da `V Disk error (not allowed)`. Un programa que carga más
ficheros, como el juego de plataformas, los busca en la carpeta actual: mejor
`CD "ROM:"` y luego `LOAD "platformer"`.

**La cinta puesta.** Un `.TAP` o un `.TZX` cargado se queda puesto, como una
cinta en la casetera, para los programas de varias partes del Spectrum (un
cargador que trae una pantalla y después el juego, o niveles que se cargan
más tarde):

- `LOAD ""` (y `LOAD "" SCREEN$`, `LOAD "" CODE`, `LOAD "" DATA a()`, `VERIFY`
  y `MERGE`) lee el siguiente bloque de ese tipo, a partir de donde se quedó
  la cinta. Sin cinta puesta es el error `R`.
- `LOAD "nombre"`, si en la tarjeta no hay un fichero con ese nombre, busca en
  la cinta el bloque que se llama así (sin distinguir mayúsculas); si tampoco
  está en la cinta, es el error `T`. Si hay un fichero, gana el fichero, y ese
  pasa a ser la cinta puesta.
- Al llegar al final de la cinta buscando un bloque, se rebobina una vez y se
  sigue buscando desde el principio; con `LOAD ""`, si no hay ninguno de ese
  tipo en toda la cinta, es el error `R`.
- La cinta sigue puesta tras `NEW` y al cambiar de carpeta; se quita al
  reiniciar la máquina o al cargar otro `.TAP` o `.TZX`.

Así, `LOAD "juego"` carga el cargador de `juego.tap`, que con `LOAD ""
SCREEN$: LOAD ""` trae su pantalla y el programa.

De los `.TAP` y `.TZX` se aprovechan los bloques de datos de velocidad estándar
(BASIC, `CODE` y pantallas). El código máquina Z80 que contengan no se puede
ejecutar. Las variables de un programa del Spectrum no se cargan (el formato
interno es otro), pero las matrices de `SAVE ... DATA` sí, en los dos
sentidos: los números van en la forma de 5 bytes del Spectrum (con su
precisión), y un 48K con su ROM original carga las que graba ZX-V.

### 7.1 Canales de fichero

Un stream del 4 al 15 puede ir a un fichero de texto de la tarjeta:

| Orden | Efecto |
|---|---|
| `OPEN #n, "nombre" OUT` | Crea el fichero (o lo vacía) para escribir |
| `OPEN #n, "nombre" IN` | Lo abre para leer |
| `PRINT #n; …`, `LIST #n` | Escriben como en la pantalla, sin colores ni `AT`; `TAB` pone espacios. **La coma es un salto de línea**, para que `PRINT #n; a, b$` se lea con `INPUT #n; a, b$` |
| `INPUT #n; a, b$, LINE c$` | Cada variable lee una línea (vale el fin de línea LF, CR o CR LF). Una cadena toma la línea entera; un número la evalúa como el `INPUT` del teclado (si no es una expresión: `C Nonsense in BASIC`). Al final del fichero: `8 End of file` |
| `INKEY$ #n` | El siguiente carácter, o `""` al final |
| `CLOSE #n` | Cierra el fichero |

- Las palabras clave se escriben como en el listado, así que `LIST #n` da el
  programa como texto. El resto de los caracteres, tal cual (un byte cada uno).
- Lo escrito se guarda en un buffer que se vacía en cada salto de línea, al
  cerrar el fichero y al acabar cada orden o el programa.
- `RUN`, `CLEAR` y `NEW` cierran todos los ficheros. Un `OPEN` sobre un stream
  que ya tiene un fichero lo cierra antes.
- Escribir en un fichero abierto con `IN` (o leer de uno con `OUT`) da
  `J Invalid I/O device`; los streams 0 a 3 y más de 8 ficheros abiertos a la
  vez, `O Invalid stream`.

```basic
10 OPEN #4, "records" OUT: PRINT #4; nombre$, puntos: CLOSE #4
20 OPEN #4, "records" IN: INPUT #4; n$, p: CLOSE #4
30 ON ERROR GO TO 60: OPEN #5, "log" IN
40 INPUT #5; LINE l$: PRINT l$: GO TO 40
60 CLOSE #5: REM ERR = 8 al llegar al final
```

**Comodines:** `*` (cualquier cosa, también nada) y `?` (un carácter), sin
distinguir mayúsculas, solo en el último nombre de la ruta (`"juegos/*.tap"` sí;
en la parte de la carpeta, `F Invalid file name`). Si no encaja nada, `T File not
found`.

Errores propios de la tarjeta: `T File not found`, `U No SD card` y
`V Disk error`. Este último dice el porqué cuando se sabe: `V Disk error (already
exists)`, `(folder not empty)`, `(it is a folder)`, `(not a folder)`, `(card full)` o
`(not allowed)` (en español, `Error de disco (ya existe)`...). `ERR` da 31 en todos.

## 8. Editor a pantalla completa

El editor imita al del **Spectrum +2/128K**:

- **Se mantienen los números de línea**, porque los necesitan `GO TO`, `GO SUB`,
  `RESTORE` y la compatibilidad con `.TAP`.
- El listado se edita **en su sitio**, moviendo el cursor por él. Al final hay
  siempre una línea vacía para teclear líneas nuevas y órdenes directas.
- **Enter** guarda la línea y lleva el cursor al principio de la línea
  siguiente del listado (como en el +2: si se añade una línea en medio, el cursor
  queda en la que la sigue). Una línea sin número se ejecuta como orden directa.
- **Al salir con el cursor de una línea cambiada, se guarda.** Si tiene un error
  de sintaxis, suena el *rasp* del 128K, el cursor se queda en el sitio del error
  y la línea de estado (encima de la barra) muestra el informe.
- Para borrar una línea se teclea solo su número; para copiarla, se cambia su
  número.
- **Se teclea con normalidad**: las palabras clave se escriben letra a letra (al
  estilo del 128K), no con el modo K/L/E del 48K. Al guardar, la línea se vuelve
  a mostrar con las palabras clave en mayúsculas.
- **Solo se teclea el juego del Spectrum**: los códigos 32–126, `£` y `©`
  (`key_to_char` en `editor.c`, que usan el editor e `INPUT`). Las letras de
  Latin-1 con tilde, diéresis, cedilla o virgulilla se escriben sin ella
  (`á` → `a`, `Ñ` → `N`, `ç` → `c`), `¿` → `?` y `¡` → `!`; los demás
  caracteres que no son ASCII se ignoran (spec §6.4). Los códigos 128 en
  adelante son los bloques, los UDG y los tokens, así que esas letras no caben
  en las cadenas del BASIC; las de los mensajes en español las dibuja la ROM
  con su propia fuente (spec §7.10).
- **EDIT (Tab)** abre el menú de opciones del 128K: *ZX-V BASIC* (vuelve),
  *Renumerar* (de 10 en 10, corrigiendo `GO TO`, `GO SUB`, `RESTORE`, `RUN`,
  `LIST`, `LLIST` y `LINE` seguidos de un número), *Pantalla* (enseña la del
  programa hasta pulsar una tecla), *Imprimir* (`LLIST`) y *Deshacer* (D67).
- **Bloques de líneas** (D64): Mayúsculas+↑/↓ marcan líneas enteras (la del
  cursor y la siguiente o la anterior; cada pulsación, una más), que se pintan
  en amarillo. Con un bloque marcado, el menú añade *Copiar aquí*, *Mover
  aquí* (detrás de la línea del cursor) y *Borrar líneas*. Las líneas nuevas
  toman números del hueco hasta la siguiente (de 10 en 10 como mucho); si no
  caben, se renumera todo de 10 en 10 y se vuelve a mirar. Al mover, las
  referencias a las líneas movidas se corrigen, como al renumerar; al copiar,
  no. Supr con el cursor en el bloque lo borra (pregunta S/N en la línea de
  estado) y Esc quita la marca. Renumerar quita la marca.
- **Sangría** (D66): el listado (el editor, `LIST` y `SAVE` de un `.bas`)
  sangra dos espacios por nivel lo que va dentro de `FOR`, `WHILE`,
  `REPEAT`, `DEF PROC`, `SELECT CASE` y el IF de bloque; `ELSE`, `ELSE IF` y
  `CASE` van un nivel menos, y lo que sigue a un `THEN` en la misma línea no
  cuenta. No se guarda: al teclear, los espacios tras el número se pierden.
- **Deshacer** (D67): la opción del menú deshace el último cambio del
  programa (líneas, bloques, `REPLACE`, `RENUM`, `DELETE`, `NEW`, `LOAD`,
  `MERGE`) y, otra vez, lo rehace. La copia está en medio de la memoria
  libre, sin reservarla, con una suma de comprobación.
- **Buscar y sustituir** (D64): `FIND "texto"` y `REPLACE "viejo", "nuevo"`
  (§9.6) buscan en el texto de las líneas tal como se listan, tras el número y
  sin distinguir mayúsculas; F3 busca otra vez lo último.
- **`LIST` lleva al editor:** `LIST 200`, tecleado como orden, pone el cursor del
  editor en la línea 200 (o en la siguiente que haya) para editarla. En un
  programa, o con un canal (`LIST #2` en la pantalla del programa, `LIST #4` en
  un fichero), saca el listado como siempre.
- **Pantallas separadas:** el editor usa la página 1 en modo 1 (64×48
  caracteres), con la barra *ZX-V BASIC* y las franjas de colores del 128K
  abajo, en la última fila (con los bytes libres), y su
  propia tabla de remapeo (la 3); el programa usa la página 0 en su propio modo
  (o las dos, ver §9.4). Las órdenes se ejecutan en la
  pantalla del programa; el informe se queda ahí hasta pulsar una tecla, que
  vuelve al editor (y cuenta como tecleada).
- **La cola de teclas y `INKEY$`:** `INKEY$`, `IN` y `KEY` leen el teclado
  directamente y no gastan la cola, así que, si el programa los ha usado,
  la cola se vacía al pararse (antes del informe) y al empezar un `INPUT`:
  lo pulsado para el programa no sale en el editor ni en el `INPUT`. Sin
  ellos, lo tecleado por adelantado se conserva (`hal_key_flush`).

| Tecla | Efecto |
|---|---|
| Cursores | Mueven el cursor por el listado |
| Inicio, Fin | Principio y fin de la línea |
| Ctrl+Inicio, Ctrl+Fin | Principio del programa y línea vacía del final |
| RePág, AvPág | Diez líneas arriba o abajo |
| Ctrl+←, Ctrl+→ | Palabra anterior y siguiente |
| Retroceso, Supr | Borran a la izquierda y bajo el cursor (Supr, en un bloque marcado, el bloque) |
| Esc | Deshace los cambios de la línea; sin cambios, quita la marca del bloque (durante un programa, BREAK) |
| Tab | EDIT: menú de opciones |
| Mayúsculas+↑, ↓ | Marcan un bloque de líneas |
| F3 | Busca otra vez (`FIND`) |

Son las teclas del PC, sin CAPS SHIFT ni SYMBOL SHIFT: los signos salen como en
la distribución del teclado. El BREAK es solo **Esc** (CAPS SHIFT + SPACE de la
matriz del Spectrum, que ven `IN` e `INKEY$`, no para el programa).

**El idioma** (spec §7.10): los informes de error, la barra y el menú del
editor salen en el idioma de la máquina, inglés o español (`LANGUAGE`).

## 9. Extensiones

Las palabras clave propias usan **tokens con el byte de prefijo `0x01`** seguido
de su número (desde `0x80`), para que el formato de las líneas siga siendo el
del Spectrum. Las sentencias solo se reconocen al principio de una sentencia,
así que no estropean programas que usen esos nombres como variables; las
funciones (`TIME`, `JOY`, `KEY`, `ERR`, `ERL`, `HIT`, `CELL`, `PIXEL`, `MOUSE`,
`CLOCK`, `DIR$`) y
`ELSE` se reconocen en cualquier sitio. `PLAY` lleva el token del 128K (`0xA4`), así que
los programas del 128K que lo usan se cargan tal cual.

Quedan libres los números `0xBC`–`0xFF` (y otros bytes de prefijo, si hiciera
falta). Los números ya usados no cambian nunca, porque están en los
programas grabados. Como algunos coinciden con tokens del Spectrum, lo que
recorre las líneas byte a byte (como la renumeración) salta siempre el par
completo.

### 9.1 Estructuras

| Sentencia | Efecto |
|---|---|
| `WHILE c` … `WEND` | Repite mientras `c` no sea 0. Si es 0 al empezar, sigue tras el `WEND` que le corresponde |
| `REPEAT` … `UNTIL c` | Repite hasta que `c` no sea 0 (al menos una vez) |
| `IF c THEN …  ELSE …` | `ELSE` separa sentencias, como `THEN`: si `c` es 0 se ejecuta lo que hay tras el `ELSE` que le corresponde (en la misma línea), y si no, se salta. Con varios `IF` en una línea, cada `ELSE` va con el `IF` más cercano |
| `IF c THEN` … `ELSE IF c THEN` … `ELSE` … `END IF` | El IF de bloque (D65): `THEN` al final de la línea, en un programa. `ELSE IF` (también con `THEN` al final) y `ELSE` empiezan su línea; tras `ELSE` puede seguir una sentencia en la misma línea. Se ejecuta la primera parte que se cumple y se sigue tras `END IF` |
| `SELECT CASE v` … `CASE …` … `CASE ELSE` … `END SELECT` | Se ejecuta la parte del primer `CASE` que encaja con `v` (número o cadena) o la de `CASE ELSE`, y se sigue tras `END SELECT`. Un `CASE` lleva una lista de valores: `expr` (igual), `a TO b` (entre los dos) u `op expr` con `op` uno de `=`, `<`, `>`, `<=`, `>=`, `<>`. Los `CASE` pueden ir tras `:` y todo en una línea, también como orden directa |
| `DEF PROC nombre[(a, b$…)]` … `END PROC` | Define un procedimiento. Al llegar a él siguiendo el programa, se salta hasta su `END PROC` |
| `PROC nombre[(expr, …)]` | Llama al procedimiento. Los parámetros son locales: se guardan al entrar y se restauran en `END PROC`. Hay recursión |
| `LOCAL v, w$…` | Dentro de un procedimiento: guarda esas variables (que empiezan valiendo 0 o `""`) y las restaura en `END PROC` |

- Los nombres de procedimiento son como los de las variables numéricas (letras
  y cifras, sin distinguir mayúsculas ni espacios). Un parámetro de cadena es
  una sola letra con `$`. Hasta 16 parámetros.
- `WHILE`, `REPEAT` y `PROC` usan su propia pila (64 niveles), distinta de la
  de `GO SUB`. El IF de bloque y `SELECT CASE` no usan pila: al llegar a la
  parte siguiente (`ELSE`, `ELSE IF`, `CASE`) se salta hasta su final, y
  pueden anidarse sin límite. Salir de un bucle con `GO TO` deja su entrada en la pila; volver
  a entrar en el mismo bucle no la repite. `RUN` y `CLEAR` vacían la pila.
- Errores propios: `Y Structure error` (`WEND` sin `WHILE`, `UNTIL` sin
  `REPEAT`, `END PROC` o `LOCAL` fuera de un procedimiento, o un `WHILE` falso,
  un `DEF PROC`, un IF de bloque o un `SELECT CASE` sin su cierre) y `Z PROC not found`. Los parámetros que no
  encajan dan `Q Parameter error`.

### 9.2 Etiquetas

Una sentencia `@nombre` marca un sitio del programa. `GO TO @nombre`,
`GO SUB @nombre`, `RUN @nombre`, `RESTORE @nombre` y `ON ERROR GO TO @nombre`
saltan a ella (a la sentencia exacta, aunque no sea la primera de la línea).
Si no existe: `X Label not found`. Las etiquetas no cambian al renumerar.

### 9.3 Errores: `ON ERROR`

- `ON ERROR GO TO línea` (o `@etiqueta`): si un error detiene el programa, se
  salta ahí. `ON ERROR GO TO 0` lo desactiva. `RUN` y `CLEAR` también.
- **El manejador se desactiva al usarse**, para que un error dentro de él no se
  repita sin fin: hay que volver a poner `ON ERROR` si se quiere seguir
  atrapando errores.
- `ERR` es el código del error (como en el informe: 2 para `Variable not
  found`, 10 para `A`…) y `ERL`, la línea.
- `CONTINUE` repite la sentencia que falló (tras `BREAK`, sigue con la
  siguiente).
- Se atrapan todos los errores de los programas, `BREAK` incluido, salvo `STOP`
  y los de las órdenes directas.

### 9.4 Vídeo

| Sentencia | Efecto |
|---|---|
| `MODE n[, ancho, alto]` | Modo 0 (256×192) o 1 (512×384). Borra las dos páginas y vuelve a la página 0, sin scroll ni cambios de color. En modo 0, la página puede medir 512×384, 1024×192 o 256×768 (el campo virtual, [spec.md §4.9](spec.md)) y la pantalla es una ventana sobre ella que se mueve con `SCROLL` |
| `PAGE n` | Página que se ve (0 o 1) |
| `SCREEN n` | Página en la que dibujan `PRINT`, `CLS`, `PLOT`, `DRAW`, `CIRCLE`… Cada página tiene sus propias sombras, así que `SCREEN$`, `ATTR` y `POINT` leen la de dibujo |
| `REMAP v, c[; v, c…]` | El color `v` se ve como `c` (0–15) en toda la pantalla del programa, sin tocar la VRAM (tabla de remapeo 0) |
| `REMAP #t; v, c[; v, c…]` | Lo mismo en la tabla `t` (0–2), que no se ve hasta que se elige con `REMAP #t` o se pone en unas líneas con `RASTER REMAP` |
| `REMAP #t` | Toda la pantalla usa la tabla `t`: cambia todos los colores de golpe |
| `REMAP` | Las tres tablas vuelven a la identidad y se usa la 0 |
| `RASTER BORDER y1, y2, c` | Las líneas `y1` a `y2` tienen el borde del color `c` (0–15, también los brillantes) |
| `RASTER REMAP y1, y2, t` | Las líneas `y1` a `y2` usan la tabla de remapeo `t` |
| `RASTER SCROLL y1, y2, x[, y]` | Las líneas `y1` a `y2` tienen su propio scroll horizontal `x` y, si se da `y`, enseñan las líneas de la página desde la `y` (pantallas partidas con scroll independiente) |
| `RASTER` | Quita la tabla por línea |
| `SCROLL x, y` | Desplaza la imagen sin tocar la VRAM, de forma circular: en la esquina superior izquierda se ve el píxel `(x, y)` de la página. `SCROLL` solo lo quita |

- **Tabla por línea:** `RASTER` escribe en la tabla por línea de la máquina
  ([spec.md §4.7](spec.md)), que la pantalla aplica línea a línea sin gastar
  nada de la CPU. Las líneas son las filas de píxeles de la pantalla, con la 0
  arriba como en `SPRITE`; las del borde son negativas (hasta −48 en modo 0 y
  −96 en modo 1) o pasan del alto (hasta 239 y 479). Lo que no se toca queda
  como dicen `BORDER`, `REMAP #t` y `SCROLL`. `MODE` deja la pantalla como al
  principio: sin tabla por línea y con las tablas de remapeo en la identidad.
  El editor no la usa, y al volver a la pantalla del programa sigue ahí.

  ```
  10 REM Rayas en el borde, como al cargar una cinta
  20 FOR y=-48 TO 239 STEP 4: RASTER BORDER y,y+1,2: RASTER BORDER y+2,y+3,5: NEXT y
  30 REM La tinta 1 cambia de color por franjas
  40 REMAP #1; 1,9: REMAP #2; 1,5: REMAP #0; 1,13
  50 RASTER REMAP 0,63,1: RASTER REMAP 64,127,2
  ```
- **Campo virtual:** la pantalla del BASIC es la página entera: `PRINT AT`
  llega a la columna 127 en una página de 1024×192, y `PLOT`, `DRAW`, `CLS`,
  `SCREEN$`, `ATTR`, `POINT` y `COPY` la cubren toda. La pantalla inferior
  (`INPUT` y los informes) queda en las dos últimas filas de la página, así que
  en 512×384 y 256×768 solo se ve con el scroll hacia abajo.
- **Doble buffer:** `SCREEN 1: CLS: …dibujar…: PAGE 1: SCREEN 0: …` y así
  sucesivamente.
- Los informes y `INPUT` salen en la página que se ve.
- **El editor también usa la página 1.** Cuando un programa vuelve a usarla
  (con `PAGE 1` o `SCREEN 1`), se redibuja a partir de sus sombras: el texto
  se recupera tal cual; los gráficos, con el color de tinta de cada celda.

### 9.5 Sonido

- `SOUND r, v[; r, v…]` escribe `v` en el registro `r` del AY (como el
  `SOUND` del Timex 2068). Los registros 16–31 son los del segundo AY.
- `PLAY a$[, b$, c$…]` toca hasta seis cadenas a la vez, con la sintaxis del
  128K: las tres primeras en los canales A, B y C del AY y las tres siguientes
  en los del segundo AY. En cada cadena, la envolvente (`U`, `W`, `X`) y el
  mezclador (`M`) son los de su AY. Las cadenas séptima y octava (MIDI en el
  128K) se comprueban pero no suenan. `PLAY` espera a que acaben todas; BREAK
  las corta.

| En la cadena | Significado |
|---|---|
| `a`…`g`, `A`…`G` | Nota; las mayúsculas, una octava más arriba |
| `#`, `$` | Sostenido y bemol (delante de la nota) |
| `&` | Silencio |
| `1`…`9` | Duración: semicorchea, semicorchea con puntillo, corchea, corchea con puntillo, negra (la de partida), negra con puntillo, blanca, blanca con puntillo y redonda |
| `10`…`12` | Tresillos de semicorchea, corchea y negra: valen para las tres notas siguientes |
| `n_m` | Duraciones ligadas: la nota siguiente dura la suma |
| `O n` | Octava (0–8; por defecto 5, en la que `c` es el do central) |
| `T n` | Tempo en negras por minuto (60–240, por defecto 120). Solo cuenta en la primera cadena |
| `V n` | Volumen (0–15, por defecto 15) |
| `U`, `W n`, `X n` | Usar la envolvente; su forma (0–7); su periodo (0–65535) |
| `M n` | Mezclador: bits 0–2, tono en A, B y C; bits 3–5, ruido |
| `N` | Separador (sin efecto) |
| `( )` | Repite lo de dentro una vez (hasta 4 niveles). Un `)` sin pareja repite la cadena desde el principio sin fin (si no tiene ninguna nota, se acaba) |
| `!…!` | Comentario |
| `H` | Para todo |
| `Y n`, `Z n` | MIDI: se ignoran |

Las cadenas mal formadas dan `A Invalid argument`, y los números fuera de rango,
`B Integer out of range`; se comprueban todas antes de empezar a sonar.

### 9.6 Edición

Solo como órdenes directas (en un programa dan `C Nonsense in BASIC`):

| Orden | Efecto |
|---|---|
| `RENUM [inicio[, paso]]` | Renumera (por defecto de 10 en 10) y corrige las referencias, como la opción *Renumerar* del menú |
| `DELETE a[, b]` | Borra las líneas de `a` a `b` (o solo la `a`) |
| `AUTO [inicio[, paso]]` | Numeración automática: la línea del final del editor empieza con el número siguiente. Por defecto, la última línea más 10, de 10 en 10. Acaba con Enter en una línea vacía, con Esc o al mover el cursor a otra línea |
| `EDIT [línea]` | Lleva el cursor del editor a esa línea |
| `TRACE ON`, `TRACE #s`, `TRACE OFF` | Cada línea que empieza a ejecutarse escribe su número, `[10]`, en la pantalla del programa o en el stream `s`, sin cambiar el canal ni los colores del programa (D68). También en un programa. `NEW` lo apaga; con el stream cerrado, `O Invalid stream` (y si se cierra después, se apaga) |
| `FIND [texto]` | Lleva el cursor del editor a la siguiente aparición del texto (desde la línea siguiente a la del cursor, dando la vuelta), sin distinguir mayúsculas. Sin texto, lo último que se buscó (como F3). Si no está, `No encontrado` en la línea de estado |
| `REPLACE viejo, nuevo` | Sustituye en todo el programa, sin distinguir mayúsculas. Cada línea cambiada se comprueba como si se tecleara y las que quedarían mal no se tocan; la línea de estado da los cambios y las líneas sin cambiar. Con `viejo` vacío, `A Invalid argument` |

### 9.7 Otras

| Palabra clave | Efecto |
|---|---|
| `TIME` | Frames desde el arranque (50 por segundo) |
| `JOY n` | Mando 0 o 1: bits 0–8 (derecha, izquierda, abajo, arriba, fuego 1–3, start y select) o 0 si no está conectado |
| `JOY n, modo` | El mando `n` se presenta también como teclas: 0 no, 1 Sinclair 1, 2 Sinclair 2, 3 Cursor (ver §5). Un `n` fuera de 0–1 o un modo fuera de 0–3 da `B Integer out of range`, como la función `JOY n` |
| `KEY n` | 1 si la tecla está pulsada ahora: `n` es el código de una letra, una cifra, Enter (13), espacio (32), los cursores (8–11) o Borrar (12), o 256 más un código de uso HID para las demás |
| `MOUSE n` | El ratón: 0 y 1, la x y la y del puntero en píxeles de la pantalla, con el origen arriba a la izquierda como `SPRITE` (0–255 y 0–191 en modo 0); 2, los botones (bit 0 izquierdo, 1 derecho, 2 central); 3, la rueda acumulada. El puntero sigue al ratón sin aceleración y no sale de la pantalla; empieza en el centro |
| `MOUSE x, y` | Coloca el puntero. Dibujarlo es cosa del programa (por ejemplo con `SPRITE`) |
| `CLOCK n` | El reloj (spec §7.9): 0 el año, 1 el mes, 2 el día, 3 la hora, 4 los minutos, 5 los segundos y 6 el día de la semana (1 lunes ... 7 domingo). Da 0 si el reloj no está en hora. Por ejemplo, `PRINT CLOCK 3;":";CLOCK 4` |
| `CLOCK año, mes, día, hora, minutos[, segundos]` | Pone el reloj en hora (de 2000 a 2099). Con una fecha que no existe, `A Invalid argument`. Los ficheros que se graban llevan la fecha y la hora del reloj |
| `SPEED 0`, `SPEED 1` | La velocidad del 48K (para los programas que usan bucles como pausa o que serían injugables a toda velocidad) o la máxima (la de partida). Ver abajo |
| `MONITOR` | Abre el monitor de pruebas |
| `CD "carpeta"` | Cambia de carpeta en la tarjeta |
| `LANGUAGE "es"` | El idioma de la máquina (spec §7.10): un código de dos letras. La ROM tiene sus textos en inglés (`"en"`) y español (`"es"`); con otro, en inglés. Se guarda (en la Pico, en el reloj) y vale para los informes desde ese momento. Con algo que no son dos letras, `A Invalid argument`. Para leerlo, `PEEK 536872736` y `PEEK 536872737` (la variable `LANG`) |

Como en el Spectrum, estas palabras no se pueden usar como nombres de
variable: `LET time=1` no es válido.

**`SPEED 0`.** Cada sentencia tarda lo que tardaría en el 48K según lo que
hace, no un tiempo fijo: un `LET a=1` unos 2 ms, un `LET a=SQR 2` unos 106
ms, un `CLS` 60 ms. El intérprete suma el coste de lo que va haciendo: la
base de la sentencia, cada operador y función, cada variable y cada línea
que recorre al buscar (como el 48K, que las busca desde el principio), lo
que imprime y dibuja, los scrolls... y la sentencia siguiente no empieza
hasta que ha pasado ese tiempo. Los costes (`rom/src/basic/speedtab.c`) se
midieron con `rom/tools/zxspeed.py` en el simulador del Z80 de SkoolKit con
la ROM original, con la contención de la memoria y las interrupciones, sobre
unas 160 sentencias de prueba (error medio del 3 %). En programas completos
que no entraron en el ajuste, la diferencia con el 48K queda por debajo del
5 % (el caso de prueba `speed48`).

- `PAUSE`, `BEEP`, `INPUT` y la espera de teclas tardan lo que tienen que
  tardar, como en el 48K.
- Las sentencias propias cuentan como un `POKE` y sus funciones como `PEEK`;
  los gráficos (`SPRITE`, `MAP`…) no tienen equivalente y no se frenan más.
- Lo que más se desvía son los números raros al pasarlos a texto (`1E10`,
  `.001`) y `CIRCLE`, hasta un 35 %.

### 9.8 Gráficos para juegos

Estas órdenes usan el motor gráfico de la ROM ([spec.md §7.8](spec.md)):
sprites con máscara, mapas de tiles, suelos en modo 7 y polígonos rellenos.

- **Coordenadas:** en píxeles de la página, **desde la esquina superior
  izquierda** y hacia abajo, como `PRINT AT` (no como `PLOT`). Pueden ser
  negativas o salirse de la página: todo se recorta.
- **Colores:** los 15 de ZX-V por su número, de 0 a 15 (`BRIGHT` es el
  bit 3: 10 es rojo brillante). El 8 es el transparente.
- **Dibujan en la página de dibujo** (`SCREEN`). Para animar sin parpadeo:
  dibujar en la oculta, enseñarla con `PAGE` y esperar con `WAIT`.
- **No pasan por las sombras de la pantalla:** `POINT`, `ATTR` y `SCREEN$` no
  ven lo que dibujan (`PIXEL` sí), y la página 1 pierde estos gráficos cuando
  la usa el editor. Las celdas con `FLASH` del rectángulo que ocupa una orden
  (el sprite, el mapa, el rectángulo que contiene el polígono…) pierden el
  `FLASH`: si no, el parpadeo las repintaría encima.

#### Gráficos

Los sprites, los tiles, los mapas y las texturas se guardan por número en un
**almacén de gráficos**, debajo de RAMTOP. Se conservan con `RUN`, `CLEAR` y
`LOAD` (un cargador puede preparar los gráficos y cargar el juego), y `NEW`
los borra. Definir otra vez un número lo sustituye. `CLEAR dirección` (real)
desplaza el almacén para que quede justo debajo.

`SAVE "x" GRAPHICS` graba todo el almacén en `x.gfx`. `LOAD "x" GRAPHICS`
carga los objetos del fichero, que sustituyen a los del mismo número (los
demás se quedan: se pueden cargar los sprites de un fichero y el nivel de
otro), y `VERIFY "x" GRAPHICS` comprueba que los del fichero son iguales a
los que hay. Un fichero que no es de gráficos o está dañado da
`R Tape loading error`. `GRAPHICS` solo es palabra clave dentro de estas
sentencias.

| Orden | Efecto |
|---|---|
| `GRAB n, x, y, an, al[, t]` | El sprite `n` (0–255) es el rectángulo de la página de dibujo que empieza en `(x, y)`, de `an` × `al` píxeles (hasta 1024). El color `t` pasa a ser transparente |
| `GRAB n, an, a$` | El sprite `n` sale de una cadena con un carácter por píxel, fila a fila, de `an` de ancho: las cifras hexadecimales (`0`–`9`, `a`–`f`) son colores y cualquier otro carácter (`.`, espacio…) es transparente |
| `TILE s, n, x, y[, an, al[, t]]` | El tile `n` (0–4095) del juego de tiles de `s` × `s` (8 o 16) sale de la página en `(x, y)`. Con `an` y `al`, el rectángulo se corta en tiles que se numeran desde `n`, fila a fila. El color `t` pasa a ser transparente |
| `TILE s, n, a$` | Tiles desde una cadena, como en `GRAB` (`s`·`s` caracteres por tile; una cadena más larga define los siguientes) |
| `MAP DIM m, an, al[, s[, r]]` | El mapa `m` (0–15) tiene `an` × `al` celdas a 0, con tiles de `s` × `s` (8 por defecto). Con `r` = 1, se repite en las dos direcciones |
| `MAP POKE m, x, y, v` | La celda `(x, y)` del mapa vale `v`: el número de tile más 4096 (espejo horizontal), 8192 (espejo vertical), 16384 (el color 8 deja ver lo de debajo) y 32768 (prioridad) |
| `MAP POKE m, x, y, a$` | Las celdas desde `(x, y)` hacia la derecha toman el código de cada carácter: `"  ##.."` usa los tiles 32, 35 y 46 |
| `TEXTURE n, x, y, s[, c]` | La textura `n` (0–3) para el modo 7 es el cuadrado de `s` × `s` (potencia de 2, de 2 a 256) de la página en `(x, y)`. Se repite sin fin; con `c`, fuera de ella todo es del color `c` |

Los juegos de tiles crecen al definir un tile más alto (los nuevos empiezan
negros). Un mapa solo se puede dibujar si existen todos los tiles que usa.

#### Dibujo

| Orden | Efecto |
|---|---|
| `SPRITE n, x, y[, f[, c$[, w, h]]]` | Dibuja el sprite `n` con su esquina en `(x, y)`. `f`: 1 espejo horizontal, 2 vertical, 3 los dos. `c$` cambia los colores: el color `i` se dibuja como el carácter `i` de la cadena (`"0123456789abcdef"` los deja igual; un carácter que no es una cifra hexadecimal deja ese color como está; `""`, ninguno). Con `w` y `h` (de 1 a 4096) se dibuja estirado o encogido a ese tamaño, como los objetos que se acercan: `SPRITE 0,x,y,0,"",32,32` |
| `MAP m[, mx, my[, x, y, an, al[, f]]]` | Dibuja el mapa a partir de su píxel `(mx, my)`, en toda la página o en el rectángulo `(x, y, an, al)`. `f`: 0 todas las celdas, 1 las que no tienen prioridad y 2 solo las que la tienen |
| `FLOOR n, x, y, h, a, hz[, d]` | Suelo en perspectiva (modo 7) con la textura `n`, visto desde el punto `(x, y)` de la textura a la altura `h`, mirando en la dirección `a` (en radianes; 0 es hacia `x` creciente), con el horizonte en la línea `hz` de la página y la distancia focal `d` (por defecto, media página). Ocupa de la línea `hz + 1` al final de la página |
| `POLY c, x1, y1[, x2, y2…]` | Un punto, una línea o, con 3 o más vértices (hasta 64), un polígono relleno del color `c` (regla par-impar) |
| `BOX c, x, y, an, al` | Rectángulo relleno |
| `DISC c, x, y, r` | Círculo relleno |
| `CLIP x, y, an, al` | Las órdenes de esta sección solo dibujan dentro de ese rectángulo. `CLIP` solo lo quita (también `MODE`, `RUN` y `CLEAR`) |
| `WAIT [n]` | Espera al siguiente VBLANK y hasta que hayan pasado `n` frames (1 por defecto) desde el `WAIT` anterior: el juego va a un ritmo fijo aunque cada vuelta tarde distinto |
| `HIT(a, ax, ay, b, bx, by)` | 1 si algún píxel opaco del sprite `a` en `(ax, ay)` coincide con uno del sprite `b` en `(bx, by)` |
| `CELL(m, x, y)` | La celda `(x, y)` del mapa (con los bits de espejo, transparencia y prioridad). Fuera de un mapa que no se repite, 0 |
| `PIXEL(x, y)` | El color (0–15) del píxel `(x, y)` de la página de dibujo, tal como está en la VRAM: ve lo que dibujan `PRINT`, `PLOT` y estas órdenes, sin el cambio de `REMAP`. Fuera de la página, -1 (útil para colisiones con lo dibujado) |

Un sprite, tile, mapa o textura que no existe da `A Invalid argument`, un
color mayor que 15 `K Invalid colour`, y los números fuera de su rango
`B Integer out of range`. Si los gráficos no caben: `4 Out of memory`.

```basic
10 BORDER 0: PAPER 0: CLS
20 DISC 2,7,7,7: DISC 10,5,5,2: GRAB 0,0,0,16,16,0: REM una pelota
30 BOX 5,0,0,8,8: TILE 8,32,0,0: REM el cielo (el espacio)
40 BOX 2,0,0,8,8: POLY 7,0,3,7,3: POLY 7,3,0,3,3: TILE 8,35,0,0: REM un ladrillo (#)
50 MAP DIM 0,40,1,8,1: MAP POKE 0,0,0,"####  ####   ######  ###########   #####"
60 LET x=0: LET s=1
70 SCREEN s: BOX 5,0,0,256,168: MAP 0,x,0,0,168,256,8
80 SPRITE 0,120,120+30*SIN (x/20)
90 PAGE s: LET s=1-s: WAIT: LET x=x+2: GO TO 70
```

Los ficheros también se pueden crear en el PC a partir de imágenes PNG con
`sdk/tools/png2gfx.py` (ver [sdk/README.md](../sdk/README.md)): hojas de
sprites, juegos de tiles, niveles dibujados enteros (los tiles repetidos se
aprovechan) y texturas.

#### Formato de los ficheros `.gfx`

Para que otras herramientas puedan crearlos. Los números son *little-endian*
y los píxeles, como en la VRAM (4 bits, el nibble bajo es el píxel par, el 8
es el transparente):

| Parte | Contenido |
|---|---|
| Cabecera | `"ZXVG"`, versión (1 byte, 1), 0 y el número de objetos (2 bytes) |
| Cada objeto | Tipo (1 byte), 0, número (2 bytes), longitud de los datos (4 bytes) y los datos |
| Sprite (tipo 1) | Ancho y alto (2 bytes cada uno) y las filas, de (ancho + 1) / 2 bytes |
| Tiles (tipo 2) | El número es el lado (8 o 16). Cuántos (2 bytes), 0 (2 bytes) y los tiles seguidos (lado × lado / 2 bytes cada uno) |
| Mapa (tipo 3) | Ancho y alto (2 bytes), lado del tile y repetición (1 byte cada uno), 0 (2 bytes) y las celdas (2 bytes), fila a fila |
| Textura (tipo 4) | log₂ del lado, repetición, color de fuera y 0 (1 byte cada uno) y los texels, uno por byte |

`examples/basic/platformer` es un juego completo hecho así: gráficos
convertidos con `png2gfx`, mapa con scroll, sprites con doble buffer,
colisiones con `CELL` y `HIT`, sonido y récord en la tarjeta.

Un bucle del BASIC dibuja unos 380 sprites de 16 × 16 por frame (medido en el
emulador); el coste está más en interpretar cada sentencia que en el dibujo.

## 10. Pendiente

Nada por ahora.

## 11. Estado de la implementación

El intérprete (`rom/src/basic/`, en C) implementa todas las sentencias y
funciones del 48K. Los detalles observables se han comprobado ejecutando los
mismos programas en un **48K con su ROM original** (el simulador de Z80 de
SkoolKit) y comparando la pantalla final:

- Formato de los números (incluida la tabla de `9^n` del manual), funciones
  matemáticas, `RND` y `RANDOMIZE`.
- Cadenas, trozos, matrices, `DEF FN`, `READ`/`DATA`, `FOR`/`NEXT`, `GO SUB`.
- `PRINT` con `,`, `;`, `'`, `TAB`, `AT`, el desplazamiento de la pantalla y la
  pantalla inferior (`#0`, `#1`).
- Informes de error (código, texto, línea y sentencia).
- `LIST` (espacios alrededor de los tokens, cursor `>`).
- Colores: `INK`/`PAPER` 8 y 9, `BRIGHT`, `FLASH`, `INVERSE`, `OVER`, colores
  temporales y los de `PLOT`, que solo cambian la tinta de la celda.
- `PLOT`, `DRAW` (líneas y arcos) y `CIRCLE`: los mismos píxeles.
- `PEEK`/`POKE` de la pantalla, los atributos y las variables del sistema.

Los programas grabados con `SAVE` se cargan y se ejecutan en el 48K original.

Las extensiones (§9) tienen sus propios guiones de prueba en
`rom/tests/basic/cases/`: estructuras, procedimientos, etiquetas y `ON ERROR`,
vídeo, `PLAY` (comparando las escrituras en el AY), órdenes de edición,
gráficos, ficheros y carpetas, comodines, el reloj, el idioma, el ratón y
`SPEED`. En el emulador (las pruebas `basic-*`, `sd-*`, `cinta` e `idioma`) se comprueban además el sonido,
las capturas de las órdenes de vídeo y gráficos, los ficheros en una carpeta y
en una imagen de tarjeta, y la cinta puesta.

Diferencias conocidas:

- La fuente es propia, así que los píxeles de las letras no coinciden.
- Los números internos son double: los cálculos son más precisos y algún
  resultado puede diferir en la última cifra.
- La pantalla inferior no hace subir la superior al crecer: la tapa.
- `SCREEN$` y `ATTR` funcionan en los dos modos, pero las direcciones de
  pantalla de `PEEK`/`POKE` solo ven los primeros 256×192 píxeles.
- `USR` con una dirección menor que 65536 da el error `S Z80 code not
  supported`; con una dirección real ejecuta código RISC-V.
- Las excepciones del código nativo vuelven al BASIC con `W Native code error`.
- Una expresión puede anidar unos 150 paréntesis, funciones o signos, y unas
  25 llamadas a `FN` dentro de otras (también una `FN` que se llama a sí
  misma); más, `4 Out of memory`. La pila de la ROM es de 32 KB y no crece con
  la memoria libre, como en el 48K.

Rendimiento: unas 33.000 vueltas por segundo de
`FOR i=1 TO 10000: LET a=i*2+1: NEXT i` (unas 200 veces el 48K).

Como en el Spectrum, cada salto (`GO TO`, `NEXT`, `UNTIL`, `RETURN`,
`END PROC`, un `PROC` que no esté en su caché) busca su línea desde el
principio del programa, y cada variable se busca en el orden en que se
creó; pero el intérprete recuerda dónde encontró lo último que buscó
(`rom/src/basic/prog.c`), así que en un programa largo no se recorre cada
vez (D76). `SPEED 0` sigue contando lo que recorrería el 48K. Medido en el
emulador, 30.000 `GO SUB` a una subrutina con 300 líneas en medio tardaban
252 imágenes y ahora 56, lo mismo que sin esas líneas; 30.000 `LET a=a+b`
con 60 variables creadas antes, de 111 a 35.
