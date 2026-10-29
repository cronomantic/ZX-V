[English](../../en/hardware/README.md) · **Español**

# Construir el hardware

ZX-V no es solo un emulador: puedes construir la máquina de verdad y
enchufarla a la tele. Está hecha con dos placas **Raspberry Pi Pico 2**, unos
cuantos conectores y unas decenas de piezas pequeñas. No hace falta programar
nada: el software de las dos Pico viene listo para copiarlo en ellas.

> **Estado.** El firmware de las dos Pico está escrito, y **cada Pico se ha
> probado por separado**: la imagen y el sonido por HDMI con una sola Pico 2,
> y la CPU, que ejecuta los programas exactamente igual que el emulador.
> **Las dos Pico todavía no se han probado juntas**, así que la máquina
> completa está sin probar, tanto en la placa como en una protoboard. La
> **placa de circuito impreso (PCB) está diseñada del todo, pero todavía no
> se ha fabricado ni probado**. Si la montas ahora, serás de los primeros en
> hacerlo: lee con calma [La placa](board.md) y cuenta con que puede haber
> algo que corregir.

## Qué necesitas

- **Dos Raspberry Pi Pico 2.** Tienen que ser Pico **2** (con el chip
  RP2350): la Raspberry Pi Pico original no puede con el firmware.
- **La placa**, pedida a JLCPCB ya montada, y las pocas piezas que sueldas tú
  ([La placa](board.md)). O, en su lugar, una protoboard y las piezas de
  [En una protoboard](protoboard.md).
- **El firmware**: dos ficheros `.uf2`, uno para cada Pico
  ([El firmware](firmware.md)).
- **Una pantalla con entrada HDMI** (una tele o un monitor) y un cable HDMI.
- **Un teclado USB.** Si quieres, también un ratón USB, uno o dos mandos USB y
  un hub USB para enchufarlo todo.
- **Una tarjeta microSD**, para guardar tus programas y cargar otros.
- **Un cargador de móvil USB-C** de 5 V que dé 2 A, con su cable.
- Si quieres, **unos altavoces con amplificador** para la salida de audio, y
  **una pila de botón CR2032** para que el reloj no pierda la fecha y la hora.
- Soldador, estaño, y un ordenador con un cable micro-USB para cargar el
  firmware en las Pico.

## Dos maneras de montarla

| | La placa (lo recomendado) | Una protoboard |
|---|---|---|
| Qué es | Una placa de 100 × 100 mm con todo | Las dos Pico y unos módulos, cableados a mano |
| Qué se suelda | Solo las dos Pico, dos pulsadores, un interruptor, dos LED y la pila: todo de agujero pasante | Muchos cables; el HDMI y el enlace entre las Pico piden cables cortos y ordenados |
| Conectores | HDMI, USB-A, microSD, jack de audio, USB-C para la corriente y reloj | Placas de conexión (*breakouts*) y módulos |
| Estado | Diseñada, **sin fabricar ni probar** | Cada Pico probada por separado; **juntas, todavía no** |
| Guía | [La placa](board.md) | [En una protoboard](protoboard.md) |

Las dos llevan el mismo firmware y funcionan igual.

## Qué hace cada Pico

Las dos Pico se reparten el trabajo como los chips del Spectrum original. La
**Pico CPU** es el ordenador en sí: ejecuta el BASIC y tus programas, y se
ocupa del teclado, el ratón y los mandos (por el puerto USB-A), de la tarjeta
SD y del reloj. La **Pico ULA** hace la imagen y el sonido: recibe de la Pico
CPU cada imagen terminada, la manda a la pantalla por HDMI, y saca el sonido
por el HDMI y por el jack de audio. Además lleva el pulsador del modo de vídeo
y el LED del enlace. Se hablan por un bus de 8 bits y unos pocos cables más.
Para tus programas solo hay una máquina: no notan que haya dos chips.

## Cómo se conecta

### La pantalla (HDMI)

Enchufa cualquier tele o monitor HDMI al conector **HDMI**. La imagen es
siempre de 640 × 480 a 60 Hz, un modo que toda pantalla HDMI tiene que
aceptar, y la Pico ULA tiene dos modos de salida:

| Modo | Salida | Sonido en la pantalla | El LED del enlace parpadea al arrancar |
|---|---|---|---|
| 0 | HDMI (el de partida) | Sí | Una vez |
| 1 | DVI (el **modo seguro**) | No | Dos veces |

- **Una pulsación corta del pulsador MODO** pasa al otro modo. La Pico ULA se
  reinicia con él (el BASIC sigue a lo suyo) y lo recuerda al apagar.
- **Si lo mantienes apretado al encender**, arranca en el modo seguro, solo esa
  vez. Lo acepta cualquier pantalla, también los monitores DVI antiguos que no
  admiten HDMI.
- Los programas no pueden cambiar el modo; solo leerlo.

### El sonido

El sonido sale por el HDMI (en los modos 0 y 1) y, siempre, por el **jack
estéreo de 3,5 mm** (rotulado `AUDIO`). El jack da nivel de línea: conecta unos
altavoces con amplificador, un amplificador o la entrada de audio de la tele.
Unos auriculares necesitan un amplificador.

### Teclado, ratón y mandos

Van al puerto **USB-A** de delante. Para más de uno, usa un hub USB (mejor si
tiene alimentación propia). ZX-V entiende los teclados y ratones USB normales
y la mayoría de los mandos.

- **Ctrl+Alt+F1**, **F2** y **F3** eligen la distribución del teclado:
  española, británica o de EE. UU. La máquina la recuerda.
- **Ctrl+Alt+Supr** es el reinicio de emergencia: vuelve al BASIC aunque un
  programa se haya colgado, y conserva tu programa en BASIC.
- **Ctrl+Alt+F12** enseña o quita la página de diagnóstico (ver
  [El firmware](firmware.md#la-página-de-diagnóstico)).

En [La máquina](../machine.md) se explica cómo ven los programas el teclado,
el ratón y los mandos.

### La tarjeta SD

Mete una tarjeta microSD en la ranura del lateral derecho. Puede estar en FAT,
FAT32 o exFAT (como vienen de fábrica), de cualquier tamaño. Se puede sacar y
volver a meter con la máquina encendida. En la placa la máquina se da cuenta
enseguida: la tarjeta que entra queda lista, y un fichero que estuviera abierto
al sacarla solo se puede cerrar (lo que ya se había escrito en él se queda).
Cierra tus ficheros antes de sacarla.
El LED verde **SD** se enciende mientras la máquina lee o escribe en ella.

Los ejemplos de serie no están en la tarjeta: están en la unidad `ROM:`, que
va dentro del firmware de la Pico CPU.

### La corriente

Enchufa un cargador USB-C al conector **5 V** de detrás, y pasa el interruptor
(delante, a la izquierda) a la derecha, a `ON`; a la izquierda es `OFF`. Con un
cargador de 2 A sobra: la máquina entera gasta menos de 1 A, con los aparatos
USB. El USB-C solo lleva corriente, no datos.

### El reloj y el idioma

La placa tiene un reloj con una pila CR2032. Con la máquina apagada conserva la
fecha y la hora (con ellas se fechan los ficheros que guardas), el idioma de la
máquina y la distribución del teclado. La hora se pone desde el BASIC con
`CLOCK`, por ejemplo `CLOCK 2026,10,20,18,30` (año, mes, día, hora y minutos).

La máquina empieza en **inglés**. Para pasar al español, `LANGUAGE "es"`; con
la pila puesta, se recuerda.

## En esta sección

- **[La placa](board.md)**: cómo pedirla montada a JLCPCB, las piezas que
  sueldas tú, una lista de comprobación, el primer encendido y qué es cada
  conector y cada mando.
- **[El firmware](firmware.md)**: dónde están los `.uf2`, cómo se cargan en
  cada Pico, cómo se actualizan, cómo compilarlos y qué mirar si no hay imagen
  o no hay sonido.
- **[En una protoboard](protoboard.md)**: ZX-V sin la placa, con el cableado
  entre las dos Pico y a cada conector.
