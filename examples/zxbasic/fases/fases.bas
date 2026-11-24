REM Fases: un ejemplo de los bancos de código (ZX-V D109) en ZX Basic.
REM
REM Lo de cada fase va en su fichero (valle.bas, espacio.bas) y cada fichero
REM en un banco de código: sus SUB y FUNCTION no están a la vez en la
REM memoria, sino que se cargan del propio .zxv la primera vez que se llama a
REM una de ellas, en la misma zona de la memoria. El programa las llama como
REM siempre. Una tecla pasa a la otra fase (y solas, a los 8 segundos).
REM
REM   zxbc --arch zxv --code-bank valle=valle.bas --code-bank espacio=espacio.bas fases.bas

#include <zxv.bas>
#include "valle.bas"
#include "espacio.bas"

CONST HZ AS INTEGER = 60                    ' imágenes por segundo
DIM pg, fase AS UBYTE
DIM f AS UINTEGER

PAPER 0: INK 7: BORDER 0
pg = 1                                      ' la página oculta
fase = 1
DO
    cartel()
    REM La primera llamada a una función de la fase carga su banco
    IF fase = 1 THEN valleEmpieza() ELSE espacioEmpieza()
    FOR f = 0 TO 8 * HZ - 1
        SCREEN pg
        IF fase = 1 THEN valleDibuja(f) ELSE espacioDibuja(f)
        BOX 1, 0, 0, 256, 16
        IF fase = 1 THEN
            PRINT AT 0, 1; INK 15; PAPER 1; "FASE 1: EL VALLE"
            PRINT AT 1, 1; INK 5; PAPER 1; "Banco de codigo: valle"
        ELSE
            PRINT AT 0, 1; INK 15; PAPER 1; "FASE 2: EL ESPACIO"
            PRINT AT 1, 1; INK 5; PAPER 1; "Banco de codigo: espacio"
        END IF
        PAGE pg: pg = 1 - pg
        WAIT
        IF INKEY$ <> "" THEN EXIT FOR
    NEXT f
    fase = 3 - fase
LOOP

REM El cartel de la fase, en las dos páginas, mientras se carga su banco
SUB cartel()
    DIM p AS UBYTE
    FOR p = 0 TO 1
        SCREEN p
        CLS
        IF fase = 1 THEN PRINT AT 11, 8; INK 14; "FASE 1: EL VALLE" ELSE PRINT AT 11, 7; INK 14; "FASE 2: EL ESPACIO"
    NEXT p
END SUB
