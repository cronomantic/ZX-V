REM espacio.bas - La fase 2, banco de código «espacio»: un planeta, estrellas
REM que pasan a tres velocidades y una nave que sube y baja.

CONST NSTARS AS UBYTE = 40
DIM sx(NSTARS - 1) AS INTEGER
DIM sy(NSTARS - 1) AS INTEGER
DIM sv(NSTARS - 1) AS UBYTE

SUB espacioEmpieza()
    DIM i AS UBYTE
    FOR i = 0 TO NSTARS - 1
        sx(i) = INT(RND * 256): sy(i) = 18 + INT(RND * 172): sv(i) = 1 + i MOD 3
    NEXT i
END SUB

SUB espacioDibuja(f AS UINTEGER)
    DIM i AS UBYTE
    DIM y, fase, llama AS INTEGER
    BOX 0, 0, 16, 256, 176
    DISC 2, 70, 76, 26                      ' el planeta, con su luz
    DISC 10, 78, 70, 19
    DISC 15, 200, 46, 6                     ' una luna
    FOR i = 0 TO NSTARS - 1
        sx(i) = sx(i) - sv(i)
        IF sx(i) < 0 THEN sx(i) = sx(i) + 256
        IF sv(i) = 3 THEN BOX 15, sx(i), sy(i), 3, 1 ELSE BOX 7, sx(i), sy(i), sv(i), 1
    NEXT i
    fase = f MOD 72                         ' la nave: sube y baja 18 píxeles
    IF fase < 36 THEN y = 140 + fase - 18 ELSE y = 140 + 54 - fase
    POLY 5, 56, y - 15, 56, y - 4, 72, y - 4             ' las aletas
    POLY 5, 56, y + 15, 56, y + 4, 72, y + 4
    POLY 13, 56, y - 8, 56, y + 8, 94, y                 ' el casco
    BOX 15, 70, y - 2, 8, 3                              ' la cabina
    IF f BAND 4 THEN llama = 8 ELSE llama = 4            ' el motor
    IF f BAND 2 THEN BOX 14, 56 - llama, y - 3, llama, 6 ELSE BOX 10, 56 - llama, y - 3, llama, 6
END SUB
