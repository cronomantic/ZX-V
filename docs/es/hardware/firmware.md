[English](../../en/hardware/firmware.md) · **Español**

# El firmware

Cada Pico necesita su propio programa, su firmware. Viene en un fichero `.uf2`
que se copia en la Pico como un fichero en un pendrive: no hace falta ningún
programa especial.

| Pico | Fichero | Qué lleva |
|---|---|---|
| Pico CPU | `zxv_cpu.uf2` | La máquina: el BASIC, los servicios (USB, tarjeta SD, reloj) y la unidad `ROM:` con los ejemplos de serie |
| Pico ULA | `zxv_ula_link.uf2` | La imagen y el sonido, que recibe de la Pico CPU |

Ojo con el de la ULA: es `zxv_ula_link.uf2`, **no** `zxv_ula.uf2`. El segundo
es una prueba que enseña unas demos ella sola, sin la Pico CPU.

## Dónde están los ficheros

1. Ve a la página de **Releases** del proyecto en GitHub
   (<https://github.com/cronomantic/ZX-V/releases>).
2. En la última versión, baja **`zxv-uf2.zip`** y descomprímelo. Lleva todos
   los `.uf2`: los que necesitas son `zxv_cpu.uf2` y `zxv_ula_link.uf2`. Los
   demás son pruebas (ver [Programas de prueba](#programas-de-prueba)). Trae
   también `zxv-update.bin`, para actualizar una máquina desde su tarjeta
   ([Cómo se actualiza](#cómo-se-actualiza)).


## Cómo se carga en una Pico

1. **Mantén pulsado el botón BOOTSEL** de la Pico (el único que tiene) y, sin
   soltarlo, conecta su micro-USB al ordenador. Luego suéltalo.
2. En el ordenador aparece una unidad llamada **`RP2350`**.
3. **Copia el `.uf2` a esa unidad** (arrastrándolo). Al terminar la copia, la
   Pico se reinicia sola con el firmware nuevo y la unidad desaparece.

Una vez para cada Pico, cada una con su fichero. Cada fichero lleva la flash
entera de la Pico: el firmware, sitio para una segunda copia (la que usan las
actualizaciones desde la tarjeta) y, en la Pico CPU, la unidad `ROM:`.

- **Antes de soldarlas.** En la placa de ZX-V lo más cómodo es cargar cada
  Pico antes de soldarla, y marcarla (CPU o ULA): son iguales. Recuerda: la
  **Pico ULA va a la izquierda** y la **Pico CPU a la derecha**, vistas desde
  delante.
- **Ya en la placa.** El micro-USB y el botón BOOTSEL siguen a mano. Conéctala
  al ordenador **con la placa encendida**: los diodos de la placa
  separan las dos alimentaciones. Con la placa apagada, una Pico sola por USB
  alimentaría a la otra por los pines del enlace, y el RESET que comparten
  podría dejarla parada.
- **En una protoboard** en la que tu fuente de 5 V va al pin VBUS de una Pico
  (el 40), **desconecta antes la fuente**, o sus 5 V llegarían a tu ordenador
  por el cable USB ([En una protoboard](protoboard.md#la-alimentación)).

## Cómo se actualiza

**Desde la tarjeta**, sin cable:

1. Baja el `zxv-uf2.zip` nuevo, descomprímelo y copia **`zxv-update.bin`** en
   la carpeta principal de la tarjeta microSD de la máquina.
2. Pon la tarjeta en la máquina y, en el editor del BASIC, pulsa **EDIT**
   (Tab) y elige **Actualizar firmware**. El cuadro dice la versión que hay en
   la tarjeta y la de la máquina. Pulsa **Enter** para actualizar, o Esc para
   dejarlo.
3. La máquina se reinicia en una pantalla de actualización con cuatro pasos:
   comprueba el fichero y actualiza la Pico ULA, la Pico CPU y la unidad
   `ROM:`. Tarda uno o dos minutos; **no la apagues**. En esa pantalla el
   teclado USB no funciona: cuando termina, se reinicia sola a los cinco
   segundos con la versión nueva. Luego puedes borrar `zxv-update.bin` de la
   tarjeta.

Cada Pico guarda **dos copias** de su firmware: la actualización se escribe
en la que no está en uso, y la Pico solo pasa a ella cuando está escrita y
comprobada. Así, si se va la corriente a medias o falla la tarjeta, las dos
Pico siguen arrancando; como mucho, la unidad `ROM:` queda vacía hasta que
repites la actualización. Si algo sale mal, la pantalla dice qué (y se
reinicia sola al medio minuto), y en la Pico CPU no ha cambiado nada si no
se llegó a su paso.

**La primera vez.** Las Pico con la versión 0.9.0 o anterior no tienen aún
las dos copias: grábalas **las dos** una vez por USB como arriba, con los
`zxv_cpu.uf2` y `zxv_ula_link.uf2` nuevos. Desde ahí, basta la tarjeta. El
cuadro del menú EDIT lo dice si una Pico lo necesita. En el emulador no hay
nada que actualizar, y el cuadro también lo dice.

**Por USB** sigue funcionando como antes: repite lo de arriba con **las dos**
Pico. Usa siempre los dos ficheros de la misma versión: si la CPU y la ULA
llevan versiones distintas del enlace, la CPU lo dice en su informe (ver
abajo), y puede que no salga la imagen.

**Sin tocar BOOTSEL.** Si tienes `picotool` (viene con la extensión Raspberry
Pi Pico de VS Code), una Pico que ya lleva un firmware de ZX-V se puede
actualizar por su cable USB, sin tocar BOOTSEL:

```sh
picotool load -f -x zxv_cpu.uf2
```

`-f` reinicia la Pico en modo de carga y `-x` arranca el firmware nuevo al
acabar. Si la Pico está colgada, o nunca ha llevado el firmware de ZX-V, usa
BOOTSEL.

## El informe

**Cada 5 segundos la Pico CPU escribe un informe**, en el idioma de la
máquina (el que elige `LANGUAGE`), sobre el enlace, el sonido, la tarjeta,
los aparatos USB, el reloj y la ULA. Es lo primero que hay que mirar cuando
algo no va. Se ve de dos formas:

- **Por J5**, con `zxv-link log` (en [Cargar programas desde el
  PC](#cargar-programas-desde-el-pc-j5)).
- **Por el micro-USB de la Pico CPU**, que es también un puerto serie: con
  cualquier terminal (el *Serial Monitor* de VS Code, PuTTY en Windows o
  `screen /dev/ttyACM0` en Linux; la velocidad da igual). Se enchufa con la
  placa encendida.

La Pico ULA escribe sus mensajes por su micro-USB.

## Cargar programas desde el PC (J5)

La tira `UART` de la placa (J5) es para **mandar programas desde el PC y
ejecutarlos**, sin sacar la tarjeta. Hace falta soldar sus 3 pines y un
adaptador USB-serie de **3,3 V** (muchos eligen la tensión con un puente o un
selector: ponlo en 3,3 V):

- `GND` con `GND`, el `TX` de la placa con el `RX` del adaptador y el `RX` de
  la placa con el `TX` del adaptador. La alimentación del adaptador (`VCC`),
  sin conectar.
- Mejor con la placa encendida: apagada, el adaptador podría alimentarla un
  poco por `RX`.
- **En Linux**, para abrir el puerto tienes que estar en el grupo `dialout`
  (`sudo usermod -aG dialout $USER` y vuelve a iniciar la sesión). Si no,
  `zxv-link` dice que no hay permiso para abrirlo.

En el PC, **`zxv-link-gui`**, con ventana, que va en los paquetes del
emulador. Se usa en pasos, de arriba abajo:

1. **Placa:** elige el puerto del adaptador; al elegirlo se conecta y dice
   qué firmware tiene la máquina y si el BASIC espera una orden.
2. **Programa:** elígelo, arrástralo a la ventana o pulsa uno de los
   recientes. Eso solo lo pone ahí: nada se manda todavía.
3. **Desde dónde lo carga la placa:** **EXT:**, lo normal (la carpeta
   compartida, sin copiar nada a la tarjeta: ver [La unidad
   EXT:](#la-unidad-ext)), o **La tarjeta** (copia antes el programa y sus
   ficheros).
   - Con **EXT:**, el recuadro **EXT: es** dice qué carpeta es, y
     **Elegir...** cambia de carpeta. Si el programa no está dentro, avisa y
     ofrece **Copiarlo a esa carpeta** (con sus ficheros, y usa esa copia) o
     **Ejecutar desde la tarjeta**.
   - Con **La tarjeta**, los ficheros de su carpeta que puede cargar
     (gráficos, música, sonidos...), marcados, y los que añadas. Con la
     placa conectada dice qué hay en la tarjeta con cada nombre: nada, lo
     mismo (no se manda), lo que puso `zxv-link` la última vez pero ha
     cambiado (se sustituye) u otro distinto que no puso `zxv-link` (antes
     de sustituirlo, pregunta). También dice si la placa no tiene tarjeta.
4. **Ejecutar en la placa** (lo manda todo y lo ejecuta) o **Solo copiar a
   la tarjeta** (sin ejecutar nada; sin programa, copia solo los ficheros
   del paso 3). Son los únicos botones que mandan algo. Si hay otro programa
   en marcha, pregunta antes de pararlo. Mientras manda, **Parar** lo deja.
5. **Resultado:** cómo va cada fichero, la orden que se teclea en la
   máquina y lo que la máquina lee y escribe en `EXT:`.

Debajo, los programas recientes y el informe de la máquina mientras está
conectada. **Desconectar** suelta el puerto, para usarlo con otro programa
(como la extensión de VS Code).

Lo mismo, en la consola, con `zxv-link`:

```
zxv-link ports
zxv-link --port COM5 run juego.zxv
zxv-link --port COM5 send graficos.gfx
zxv-link --port COM5 info
zxv-link --port COM5 log
```

- **`ports`** dice qué puertos serie hay: el del adaptador es el que aparece
  al enchufarlo (`COM5` en Windows, `/dev/ttyUSB0` en Linux).
- **`run`** copia el programa a la raíz de la tarjeta y lo carga (un `.bas`,
  con `RUN`). Detrás del programa pueden ir los ficheros que carga
  (`run juego.zxv juego.gfx musica.zxm`). Lo que ya está igual en la tarjeta
  no se vuelve a mandar. Si hay un programa en marcha, antes reinicia la
  máquina entera, como Ctrl+Alt+Supr, y espera a que vuelva el BASIC.
- **`send`** solo lo copia (con otro nombre detrás, a otro sitio:
  `send x.gfx SD:juego/x.gfx`).
- **`info`**, la versión de la máquina; **`log`**, el informe hasta que
  pulsas Ctrl+C.

**Lo que hay en la tarjeta no se pierde sin querer:**

- Un fichero llega entero o no llega. La placa lo escribe aparte
  (`zxv-link.tmp`) y solo al final, con todo comprobado, le pone su nombre.
  Si algo lo corta (**Parar**, el adaptador desenchufado, la placa apagada),
  el que había con ese nombre sigue como estaba.
- `zxv-link` apunta en el PC lo que pone en la tarjeta. Lo que ha cambiado
  desde entonces (el programa que editas y vuelves a ejecutar) lo sustituye
  sin preguntar. Lo que no puso él (lo que copiaste a mano con otro
  contenido) no lo toca: no manda nada y sale con el código 4, salvo con
  `--overwrite` (`zxv-link --port COM5 --overwrite run juego.zxv`). La
  extensión de VS Code y la ventana lo preguntan.

J5 habla en tramas con `zxv-link`: un terminal no sirve.

**Desde VS Code**, la [extensión de ZX-V](https://github.com/cronomantic/zxv-vscode)
hace lo mismo con **ZX-V: Ejecutar en la placa** (desde `EXT:` o desde la
tarjeta: ver abajo), y depura el BASIC de la ROM en la placa con
`"target": "board"`: para eso usa `zxv-link serve`, que ofrece el depurador
por TCP igual que el emulador con `--basic-debug`.

### La unidad EXT:

Además de `SD:` (la tarjeta) y `ROM:` (el disco de la ROM, de solo
lectura), la máquina tiene una tercera unidad: **`EXT:`, la unidad
externa**, los ficheros de una carpeta que sirve **otro equipo** (el
servidor: un PC, una Raspberry Pi, un móvil, otra ZX-V...). Hoy va por J5,
con el mismo adaptador; más adelante, quizá por Wi-Fi. Es **una sola unidad
sea cual sea el enlace**: un programa que usa `EXT:datos.dat` funciona igual
por cable que por Wi-Fi (si hubiera servidor por los dos, gana el cable).

Los programas y el BASIC la usan como las otras, para leer **y escribir**:
`LOAD "EXT:juego.bas"`, `SAVE "EXT:copia"`, `CAT "EXT:"`, `CD "EXT:"`,
`OPEN #` y las funciones de ficheros de la API de C, con los mismos nombres
que en la tarjeta. **Sin servidor**, todo lo de `EXT:` da el error −18
(`V Error de disco (EXT: sin servidor)`); si el servidor se va, la primera
llamada espera 1,5 segundos y falla, y las siguientes fallan enseguida hasta
que vuelve. Nada se cuelga. Por J5 va a unos **80 KB/s** (un programa de
100 KB carga en poco más de un segundo); la tarjeta es más rápida.

**La carpeta se elige a propósito**, nunca es la del programa: la máquina
puede crear ficheros en ella, y la del programa podría ser tu carpeta
personal, Descargas o una carpeta del sistema. En `zxv-link-gui` es, por
defecto, `Documentos/ZX-V-EXT` (se crea la primera vez; no es
`Documentos/ZX-V`, que es la tarjeta del emulador), y **Elegir...** pone
otra. Se sirve mientras la placa está conectada (*Se sirve como EXT:
mientras la placa está conectada.*), así que la máquina también puede usarla
sola: con `SAVE "EXT:copia"` desde el BASIC guardas en el PC lo que tecleas
en la máquina.

En la consola:

```
zxv-link --port COM5 serve --folder CARPETA
zxv-link --port COM5 serve --folder CARPETA --run CARPETA/juegos/juego.zxv
```

- **`serve --folder`** sirve la carpeta como `EXT:` hasta que pulsas
  Ctrl+C, y dice lo que la máquina lee y escribe (`EXT:/record.txt
  (write)`). Se puede juntar con `--basic-debug`.
- **`--run`** antes ejecuta el programa (que tiene que estar dentro de la
  carpeta) desde `EXT:`: teclea `CD "EXT:/juegos": LOAD "juego.zxv"`, para
  que un `.zxv` encuentre sus ficheros, y sigue sirviendo.
- Hace falta un firmware con el protocolo 4 (`zxv-link info` lo dice).

**Lo que hay en la carpeta no se pierde sin querer** (con `zxv-link`, la
ventana y la extensión de VS Code):

- La máquina no sale de la carpeta: `..` se queda en la raíz.
- Un fichero se escribe entero o no se escribe: va a uno temporal que solo
  al final toma su nombre.
- **No sustituye ni borra los ficheros que no escribió la máquina** (le da
  `ZXV_E_ACCESS`), salvo que lo permitas: con `--overwrite`
  (`zxv-link --port COM5 --overwrite serve --folder CARPETA`) o, en la
  ventana, con la casilla **Dejar que la máquina sustituya o borre ficheros
  que no escribió ella** (sin marcar al principio). Lo que escribe la
  máquina lo apunta en el PC.

**En VS Code**, **ZX-V: Ejecutar en la placa** carga por defecto desde
`EXT:` (el ajuste `zxv.boardFrom`: `ext` o `card`). La carpeta es la del
espacio de trabajo, u otra con el ajuste `zxv.extFolder`; `zxv.extOverwrite`
deja sustituir. `zxv-link` sigue sirviendo mientras el programa está en
marcha: en la barra de estado sale **EXT:**, y un clic (o **ZX-V: Dejar de
servir EXT:**) lo para. Si el programa está fuera de la carpeta, ofrece
ejecutarlo desde la tarjeta. Al **depurar en la placa** (`"target":
"board"`), lo mismo: la carpeta se sirve mientras dura la sesión y el
programa se carga desde `EXT:`.

Para hacer otro servidor (en otro equipo, en otro lenguaje), lo que viaja
por J5 está en [El protocolo](protocol.md).

## Si no hay imagen o no hay sonido

### No hay imagen

Fíjate en el **LED rojo del enlace** (`ENLACE` en la placa) al encender.

- **No parpadea nunca.** La Pico ULA no está ejecutando su firmware: mira que
  lleve `zxv_ula_link.uf2` y que le llegue corriente.
- **Parpadea y luego se queda apagado.** La ULA funciona, pero no le llega
  nada de la Pico CPU. Mira que la CPU lleve `zxv_cpu.uf2` (y que no le hayas
  puesto a la ULA el de la CPU por error). En una protoboard, revisa los cables
  del enlace.
- **Parpadea y luego se queda encendido.** Las imágenes llegan bien: el
  problema está entre la ULA y la pantalla. Prueba el modo seguro (abajo),
  otra entrada HDMI o un cable más corto y sin adaptadores. En la placa, el
  informe del puerto serie dice si se ve el monitor (`monitor: conectado`): si
  se queda en `no se sabe` con la pantalla enchufada y encendida, mira el cable
  y el conector HDMI.
- **No deja de parpadear.** Llegan imágenes, pero algunas con errores. En una
  protoboard, acorta los cables del enlace, pon las resistencias de 33 Ω y más
  cables de masa.

Las veces que parpadea al arrancar son el modo de vídeo más uno (una vez en el
modo 0, dos en el 1).

**El modo seguro.** Mantén apretado **MODO** mientras enciendes: la ULA arranca
en DVI, que lo acepta cualquier pantalla. Dura solo hasta el siguiente
reinicio. Para quedarte con un modo, pulsa MODO brevemente con la máquina en
marcha: cada pulsación corta pasa de HDMI a DVI o al revés, lo guarda y
reinicia la ULA. Si tu pantalla solo se ve en el modo seguro, pulsa MODO hasta
que el LED del enlace parpadee **dos veces** al arrancar: es el modo 1, el
mismo que el seguro, y se queda guardado.

| Modo | Salida | Sonido en la pantalla |
|---|---|---|
| 0 | HDMI 640 × 480, 60 Hz (el de partida) | Sí |
| 1 | DVI 640 × 480, 60 Hz (el modo seguro) | No |

**Se ven unas demos en lugar del BASIC** (un juego que se desplaza, rayas de
colores, círculos): la Pico ULA lleva `zxv_ula.uf2`, la prueba. Cárgale
`zxv_ula_link.uf2`.

**Puntos que bailan sobre una imagen que se reconoce** son errores de
transmisión entre la ULA y la pantalla. En una protoboard, busca un cable
suelto y deja los del HDMI cortos y por parejas. Algunas teles necesitan
además 5 V en el pin 18 del conector HDMI
([En una protoboard](protoboard.md#el-hdmi)).

### No hay sonido

- **En el modo 1 (DVI) la pantalla no tiene sonido.** Pasa al modo 0 con
  MODO, o usa el jack de audio.
- **El jack da nivel de línea**: necesita unos altavoces con amplificador, un
  amplificador o la entrada de audio de una tele. Unos auriculares necesitan
  un amplificador.
- **Prueba un sonido desde el BASIC:** `BEEP 1,0` o `PLAY "cdefgab"`.
- **En una protoboard**, el sonido va por su propio cable, de GP20 de la Pico
  CPU (pin 26) a GP21 de la Pico ULA (pin 27). Sin él hay imagen pero no
  sonido.
- Un poco de soplido en el jack es normal.

### Otras cosas

- **El teclado no hace nada.** Mira el informe del puerto serie: la parte
  `USB:` dice cuántos teclados, ratones y mandos ve. Prueba sin el hub, o con
  un hub con alimentación.
- **La tarjeta no va.** Tiene que estar en FAT, FAT32 o exFAT. El informe dice
  `sin tarjeta` hasta el primer acceso (por ejemplo, `CAT`), o si la tarjeta no
  responde. En la placa, la tarjeta se monta en cuanto entra.
- **El informe dice `sin ULA: temporizador`**: a la Pico CPU no le llega la
  señal de imagen de la ULA. En una protoboard, mira el cable entre los dos
  GP27 (pin 32).
- **El informe dice `sin canal de vuelta`**: falta el cable de GP20 de la ULA
  (pin 26) a GP18 de la CPU (pin 24). Todo funciona sin él, salvo que a los
  programas siempre se les dice que la salida es HDMI.

### La página de diagnóstico

**Una pulsación larga de MODO** (de 1 a 5 segundos) o **Ctrl+Alt+F12** en el
teclado enseñan una franja azul en el centro de la pantalla con cómo van las
dos Pico, al día cada segundo: arriba la ULA (su modo, las imágenes por
segundo, los errores del enlace, el CRC de las paletas que le llegan, el
sonido); abajo la CPU (enlace, sonido,
tarjeta, USB, reloj). Si lo de la CPU no llega en 2,5 segundos, la página lo
dice. Lo mismo otra vez la quita. Sale sin tildes: la fuente de la página solo
tiene ASCII. Va en el idioma de la máquina, que la CPU le manda a la ULA con
cada imagen. Los mensajes de la Pico ULA por su micro-USB también, en cuanto
funciona el enlace.

## Programas de prueba

En `zxv-uf2.zip` vienen también unas pruebas. Dos son útiles al montarla:

| Fichero | En | Qué hace |
|---|---|---|
| `zxv_ula.uf2` | Una Pico 2 sola | La imagen y el sonido de la ULA con unas demos, sin la Pico CPU. Comprueba la salida HDMI y el jack. El pulsador MODO va igual |
| `zxv_hdmi.uf2` | Una Pico 2 sola | Barras de colores y un tono de prueba a cada lado (izquierda, derecha y centro) por el HDMI. `zxv_dvi.uf2` es lo mismo en DVI, sin sonido |

Los demás (`zxv_linktx`, `zxv_bench`, `zxv_bench_ram`, `zxv_isatest`) son para
quien desarrolla el firmware. Acuérdate de volver a cargar el firmware de
verdad después.

