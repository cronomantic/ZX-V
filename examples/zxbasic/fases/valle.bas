REM valle.bas - La fase 1, banco de código «valle»: tres pelotas que botan en
REM la hierba. Sus variables (los DIM de fuera de las funciones) están en la
REM parte residente, así que no se pierden al cargar otro banco.

DIM bx(2) AS INTEGER                        ' en 1/16 de píxel
DIM by(2) AS INTEGER
DIM bvx(2) AS INTEGER
DIM bvy(2) AS INTEGER
DIM bc(2) AS UBYTE => {10, 14, 11}
DIM br(2) AS UBYTE => {9, 8, 7}

SUB valleEmpieza()
    DIM i AS INTEGER                        ' con UBYTE, (40 + i * 80) * 16 se haría en 8 bits
    FOR i = 0 TO 2
        bx(i) = (40 + i * 80) * 16
        by(i) = (150 + i * 8) * 16
        bvy(i) = 0
        IF i = 1 THEN bvx(i) = -14 ELSE bvx(i) = 12 + i * 4
    NEXT i
END SUB

SUB valleDibuja(f AS UINTEGER)
    DIM i AS INTEGER
    DIM x, y, r, suelo AS INTEGER
    BOX 13, 0, 16, 256, 40                  ' el cielo
    BOX 5, 0, 56, 256, 54
    DISC 14, 200, 42, 12                    ' el sol
    POLY 1, 0, 112, 0, 80, 40, 62, 90, 96, 140, 58, 200, 92, 255, 70, 255, 112   ' las montañas
    BOX 4, 0, 110, 256, 82                  ' la hierba
    FOR y = 116 TO 190 STEP 12
        BOX 12, 0, y, 256, 2
    NEXT y
    FOR i = 0 TO 2
        r = br(i)
        suelo = (186 - r) * 16
        bvy(i) = bvy(i) + 3                 ' la gravedad
        bx(i) = bx(i) + bvx(i)
        by(i) = by(i) + bvy(i)
        IF by(i) > suelo THEN
            by(i) = suelo
            bvy(i) = -bvy(i) * 15 / 16
            IF bvy(i) > -30 THEN bvy(i) = -66 - i * 6       ' que no se paren
        END IF
        IF bx(i) < r * 16 THEN bvx(i) = ABS(bvx(i))
        IF bx(i) > (256 - r) * 16 THEN bvx(i) = -ABS(bvx(i))
        x = bx(i) / 16: y = by(i) / 16
        BOX 0, x - r, 185, 2 * r, 2         ' la sombra
        DISC bc(i), x, y, r
        DISC 15, x - r / 3, y - r / 3, r / 3
    NEXT i
END SUB
