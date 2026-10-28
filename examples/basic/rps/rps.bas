10 REM ZX-V: Rock, Paper, Scissors
20 REM El IF de bloque, SELECT CASE y procedimientos (ver README.md)
30 DIM n$(3,6): FOR i=1 TO 3: READ n$(i): NEXT i
40 DATA "piedra","papel","tijera"
50 RANDOMIZE
60 BORDER 1: PAPER 1: INK 7: CLS
70 LET yo=0: LET tu=0: LET fin=0
80 PRINT AT 1,5; INK 6; BRIGHT 1;"ROCK, PAPER, SCISSORS"
90 PRINT AT 18,0;"P piedra  A papel  T tijera"'"S salir"
100 PROC marcador
110 REPEAT
120 LET k$=INKEY$
130 SELECT CASE k$
140 CASE "p","P": PROC ronda(1)
150 CASE "a","A": PROC ronda(2)
160 CASE "t","T": PROC ronda(3)
170 CASE "s","S": LET fin=1
180 CASE "": WAIT 1
190 CASE ELSE: BEEP .05,-12
200 END SELECT
210 UNTIL fin
230 IF tu>yo THEN
240 PRINT AT 12,2; INK 4; BRIGHT 1;"Has ganado la partida!"
250 PLAY "T200O5 3ceg5C"
260 ELSE IF tu<yo THEN
270 PRINT AT 12,2; INK 2; BRIGHT 1;"Gana la maquina..."
280 ELSE
290 PRINT AT 12,2;"Empate"
300 END IF
310 PRINT AT 21,0;"Fin de la partida              "
320 STOP
1000 REM --- Una ronda: t es lo tuyo; la maquina elige al azar ---
1010 DEF PROC ronda(t)
1020 LOCAL m
1030 LET m=INT (RND*3)+1
1040 PRINT AT 8,2;"Tu: ";n$(t);"   Yo: ";n$(m);"   "
1050 REM 0: empate, 1: gana el primero, 2: gana el segundo
1060 SELECT CASE t-m+3-3*INT ((t-m+3)/3)
1070 CASE 0
1080 PRINT AT 10,2;"Empate          "
1090 CASE 1
1100 PRINT AT 10,2; INK 4;"Ganas tu        "
1110 LET tu=tu+1
1120 BEEP .1,12
1130 CASE 2
1140 PRINT AT 10,2; INK 2;"Gano yo         "
1150 LET yo=yo+1
1160 BEEP .1,-5
1170 END SELECT
1175 PROC marcador
1178 REPEAT: WAIT 1: UNTIL INKEY$="": REM hasta que se suelte la tecla
1180 END PROC
2000 REM --- El marcador ---
2010 DEF PROC marcador
2020 PRINT AT 4,2;"Tu ";tu;"  -  ";yo;" Yo   "
2030 END PROC
