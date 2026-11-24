REM Nave 3D: un ejemplo de zxv3d.bas (el 3D de caras planas del SDK de ZX-V,
REM D111) en ZX Basic.
REM
REM La nave es nave.obj, pasada a BASIC con el conversor del SDK:
REM
REM   python sdk/tools/obj2c.py nave.obj nave.bas nave --size 4 --bas
REM
REM Gira sola; los cursores (o un mando) la giran más deprisa o la inclinan,
REM y espacio (o fuego) cambia entre caras y alambre. Las estrellas son
REM puntos del mundo 3D que vienen hacia la cámara: PROJECT3D dice en qué
REM píxel caen. Todo se dibuja en la página oculta y luego se enseña (doble
REM buffer), a 60 imágenes por segundo.
REM
REM   zxbc --arch zxv nave3d.bas

#include <zxv.bas>
#include <zxv3d.bas>
#include "nave.bas"

CONST NST AS UBYTE = 48                     ' estrellas
DIM stx(NST - 1) AS FIXED                   ' dónde está cada una
DIM sty(NST - 1) AS FIXED
DIM stz(NST - 1) AS FIXED
DIM yaw, pitch AS LONG
DIM spin AS INTEGER
DIM pg, i, wire, held AS UBYTE
DIM faces AS INTEGER

nave_model(0)
CAMERA3D 0, 1.5, -5.5, 0, -2700            ' detrás y algo arriba, mirando un poco abajo
LIGHT3D -1, 2, -2, 1                        ' de arriba, a la izquierda y delante

FOR i = 0 TO NST - 1
    newStar(i)
    stz(i) = RND * 36 - 6
NEXT i

PAPER 0: INK 7: BORDER 0
pg = 1                                      ' la página oculta
spin = 300
DO
    REM Los mandos: izquierda y derecha cambian el giro, arriba y abajo inclinan
    IF KEY(8) OR (JOY(0) BAND 2) THEN spin = spin - 20
    IF KEY(9) OR (JOY(0) BAND 1) THEN spin = spin + 20
    IF spin > 2000 THEN spin = 2000
    IF spin < -2000 THEN spin = -2000
    IF KEY(11) OR (JOY(0) BAND 8) THEN pitch = pitch + 400
    IF KEY(10) OR (JOY(0) BAND 4) THEN pitch = pitch - 400
    IF KEY(32) OR (JOY(0) BAND 16) THEN
        IF held = 0 THEN
            wire = 1 - wire
            IF wire THEN WIREFRAME3D 13 ELSE WIREFRAME3D -1
        END IF
        held = 1
    ELSE
        held = 0
    END IF
    yaw = (yaw + spin) BAND 65535
    pitch = pitch BAND 65535

    SCREEN pg
    BOX 0, 0, 0, 256, 192
    stars()
    BEGIN3D
    faces = ADD3D(0, 0, 0, 0, yaw, pitch)
    DRAW3D
    PRINT AT 0, 0; INK 15; "NAVE 3D"; AT 0, 22; INK 5; "CARAS "; faces; " "
    PRINT AT 22, 0; INK 13; "CURSORES: GIRAR"; AT 22, 16; "ESPACIO: ";
    IF wire THEN PRINT "CARAS  "; ELSE PRINT "ALAMBRE";
    PAGE pg
    pg = 1 - pg
    WAIT
LOOP

REM Las estrellas vienen hacia la cámara; las de cerca, más claras
SUB stars()
    DIM s AS UBYTE
    FOR s = 0 TO NST - 1
        stz(s) = stz(s) - 0.25
        IF stz(s) < -6 THEN newStar(s)
        IF PROJECT3D(stx(s), sty(s), stz(s)) THEN
            IF stz(s) < 6 THEN
                BOX 15, PROJ3DX(), PROJ3DY(), 2, 2
            ELSEIF stz(s) < 18 THEN
                BOX 7, PROJ3DX(), PROJ3DY(), 1, 1
            ELSE
                BOX 1, PROJ3DX(), PROJ3DY(), 1, 1
            END IF
        END IF
    NEXT s
END SUB

REM Una estrella nueva, al fondo
SUB newStar(s AS UBYTE)
    stx(s) = RND * 24 - 12
    sty(s) = RND * 18 - 9
    stz(s) = 30
END SUB
