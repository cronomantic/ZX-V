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
   demás son pruebas (ver [Programas de prueba](#programas-de-prueba)).


## Cómo se carga en una Pico

1. **Mantén pulsado el botón BOOTSEL** de la Pico (el único que tiene) y, sin
   soltarlo, conecta su micro-USB al ordenador. Luego suéltalo.
2. En el ordenador aparece una unidad llamada **`RP2350`**.
3. **Copia el `.uf2` a esa unidad** (arrastrándolo). Al terminar la copia, la
   Pico se reinicia sola con el firmware nuevo y la unidad desaparece.

Una vez para cada Pico, cada una con su fichero.

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

Baja el `zxv-uf2.zip` nuevo y repite lo de arriba con **las dos** Pico. Usa
siempre los dos ficheros de la misma versión: si la CPU y la ULA llevan
versiones distintas del enlace, la CPU lo dice en su informe (ver abajo), y
puede que no salga la imagen.

**Sin tocar BOOTSEL.** Si tienes `picotool` (viene con la extensión Raspberry
Pi Pico de VS Code), una Pico que ya lleva un firmware de ZX-V se puede
actualizar por su cable USB, sin tocar BOOTSEL:

```sh
picotool load -f -x zxv_cpu.uf2
```

`-f` reinicia la Pico en modo de carga y `-x` arranca el firmware nuevo al
acabar. Si la Pico está colgada, o nunca ha llevado el firmware de ZX-V, usa
BOOTSEL.

## El puerto serie y el informe

El micro-USB de la Pico CPU es también un **puerto serie**. Ábrelo con
cualquier terminal (por ejemplo el *Serial Monitor* de VS Code, PuTTY en
Windows o `screen /dev/ttyACM0` en Linux; la velocidad da igual). Ahí pasan
dos cosas:

- **Lo que escribes en el terminal le llega a ZX-V** como si lo tecleases en
  su teclado. Viene bien para probar la máquina antes de tener un teclado USB.
- **Cada 5 segundos la Pico CPU escribe un informe**, en el idioma de la
  máquina (el que elige `LANGUAGE`), sobre el enlace, el
  sonido, la tarjeta, los aparatos USB, el reloj y la ULA. Es lo primero que
  hay que mirar cuando algo no va.

Los mismos mensajes salen también por la tira `UART` de la placa (J5), a
921600 baudios. La Pico ULA escribe los suyos por su micro-USB.

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
segundo, los errores del enlace, el sonido); abajo la CPU (enlace, sonido,
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

