10 REM SENTRY: defiende las ciudades de los misiles, con el raton
20 REM El IF de bloque, SELECT CASE, procedimientos y los graficos rapidos.
30 REM Lo que se hace en cada frame va al principio: el BASIC busca las lineas
40 REM de los saltos (NEXT, UNTIL, PROC...) desde la primera.
50 GO TO @empezar
100 REM --- Una oleada: el bucle del juego ---
110 DEF PROC jugar
120 REPEAT
130 LET fr=fr+1
140 PROC entrada
150 IF lanzados<nl AND vivos<maxv AND fr>=prox THEN PROC lanzar(RND*255,10): LET lanzados=lanzados+1: LET prox=fr+li*(.5+RND)
160 SCREEN s: SPRITE 20,0,0: PROC escena
170 PROC mover
180 SPRITE 6,mx-4,my-4
190 PRINT AT 0,0; BRIGHT 1; PAPER 0; INK 6; puntos; AT 0,12; INK 7; o$; ola; AT 0,23; INK 5; "^"; hi
200 IF demo THEN PRINT AT 3,10; BRIGHT 1; PAPER 0; INK 5; d$
210 PAGE s: LET s=1-s: WAIT 1
220 UNTIL (lanzados>=nl AND vivos=0 AND expl=0) OR ciudades=0 OR quit
230 END PROC
300 REM --- La entrada: el raton, o los cursores y espacio, o un mando ---
310 DEF PROC entrada
320 IF demo THEN
330 PROC piloto
340 ELSE
350 LET mx=MOUSE 0: LET my=MOUSE 1: LET j=JOY 0
360 LET dx=(KEY 9 OR FN b(j,0))-(KEY 8 OR FN b(j,1)): LET dy=(KEY 10 OR FN b(j,2))-(KEY 11 OR FN b(j,3))
365 REM Con las teclas o la cruceta, la mira acelera: de 2 a 8 pixeles por imagen
370 IF dx OR dy THEN LET ac=ac+1: LET sp=2+INT (ac/3): LET sp=sp-(sp-8)*(sp>8): LET mx=mx+dx*sp: LET my=my+dy*sp: MOUSE mx,my
375 IF NOT (dx OR dy) THEN LET ac=0
380 LET bt=MOUSE 2>0 OR KEY 32 OR j>=16
390 END IF
400 IF mx<0 THEN LET mx=0
410 IF mx>255 THEN LET mx=255
420 IF my<12 THEN LET my=12
430 IF my>156 THEN LET my=156
440 IF bt AND NOT pb THEN PROC disparar
450 LET pb=bt
460 END PROC
500 REM --- Todo se mueve y se dibuja: los misiles, las explosiones y los choques ---
510 DEF PROC mover
520 LET vivos=0: LET expl=0
530 FOR i=1 TO he: REM solo hasta el hueco mas alto que se ha usado
540 IF e(i,1) THEN LET vivos=vivos+1: LET e(i,4)=e(i,4)+e(i,6): LET e(i,5)=e(i,5)+e(i,7): POLY tc,e(i,2),e(i,3),e(i,4),e(i,5): POLY 15,e(i,4),e(i,5): IF e(i,9) THEN IF e(i,5)>60+RND*40 THEN LET e(i,9)=0: PROC lanzar(e(i,4),e(i,5)): PROC lanzar(e(i,4),e(i,5))
550 IF e(i,1) THEN IF e(i,5)>=t(e(i,8),2) THEN PROC impacto(i)
560 NEXT i
570 FOR i=1 TO 6
580 IF p(i,1) THEN LET p(i,4)=p(i,4)+p(i,6): LET p(i,5)=p(i,5)+p(i,7): LET p(i,8)=p(i,8)-1: POLY 9,p(i,2),p(i,3),p(i,4),p(i,5): SPRITE 7,p(i,9)-2,p(i,10)-2: IF p(i,8)<=0 THEN LET p(i,1)=0: PROC explotar(p(i,9),p(i,10))
590 NEXT i
600 LET a=INT (fr/2)
610 FOR k=1 TO 12
620 IF x(k,1) THEN PROC estallido(k)
630 NEXT k
640 IF puntos>=extra THEN PROC premio
650 IF fv THEN LET fv=fv-1: LET fp=fp+25: SOUND 16,fp-256*INT (fp/256); 17,INT (fp/256); 24,fv
660 IF xv THEN LET xv=xv-1: SOUND 25,xv
670 END PROC
700 REM Una explosion: crece, se encoge y derriba los misiles que toca
710 DEF PROC estallido(k)
720 LET x(k,1)=x(k,1)+1: LET expl=expl+1
730 IF x(k,1)>32 THEN LET x(k,1)=0: END PROC
740 LET r=x(k,1)*1.25: IF x(k,1)>16 THEN LET r=(33-x(k,1))*1.25
750 DISC c(1+(a+k)-4*INT ((a+k)/4)),x(k,2),x(k,3),r
760 FOR i=1 TO he
770 IF e(i,1) THEN LET u=e(i,4)-x(k,2): IF ABS u<r THEN LET v=e(i,5)-x(k,3): IF u*u+v*v<r*r THEN PROC derribar(i)
780 NEXT i
790 END PROC
800 DEF PROC explotar(a,b)
810 LOCAL k,f
820 LET f=0: FOR k=1 TO 12: IF x(k,1)=0 AND f=0 THEN LET f=k
830 NEXT k
840 IF f THEN LET x(f,1)=1: LET x(f,2)=a: LET x(f,3)=b
850 SOUND 22,16+INT (RND*12): IF xv<12 THEN LET xv=12
860 END PROC
900 REM Un misil enemigo, derribado: estalla a su vez (y puede derribar a otros)
910 DEF PROC derribar(i)
920 LET e(i,1)=0: LET puntos=puntos+25*mult
930 PROC explotar(e(i,4),e(i,5))
940 END PROC
1000 REM Las ciudades, las bases y su municion
1010 DEF PROC escena
1020 FOR k=1 TO 6: SPRITE 2-t(k,3),t(k,1)-10,164: NEXT k
1030 FOR k=1 TO 3: SPRITE 4-t(6+k,3),t(6+k,1)-14,165: FOR j=1 TO z(k): BOX 15,t(6+k,1)-5+2*(j-1-5*(j>5)),179+3*(j>5),1,2: NEXT j: NEXT k
1040 END PROC
1100 REM --- Disparar: desde la base mas cercana que tenga misiles ---
1110 DEF PROC disparar
1120 LOCAL b,d,k,i,l,a
1130 LET b=0: LET d=999: LET i=0
1140 FOR k=1 TO 3
1150 IF t(6+k,3) AND z(k)>0 THEN IF ABS (t(6+k,1)-mx)<d THEN LET d=ABS (t(6+k,1)-mx): LET b=k
1160 NEXT k
1170 FOR k=1 TO 6: IF p(k,1)=0 AND i=0 THEN LET i=k
1180 NEXT k
1190 IF b AND i THEN
1200 LET z(b)=z(b)-1: LET p(i,1)=1: LET p(i,2)=t(6+b,1): LET p(i,3)=164
1210 LET p(i,4)=p(i,2): LET p(i,5)=164: LET p(i,9)=mx: LET p(i,10)=my
1220 LET a=mx-p(i,2): LET l=my-164: LET l=SQR (a*a+l*l): IF l<1 THEN LET l=1
1230 LET p(i,6)=(mx-p(i,2))/l*5: LET p(i,7)=(my-164)/l*5: LET p(i,8)=INT (l/5)+1
1240 LET fp=40: LET fv=13: SOUND 17,0
1250 ELSE
1260 SOUND 16,200; 17,1; 24,10: LET fv=3: LET fp=600: REM sin misiles: un chasquido
1270 END IF
1280 END PROC
1300 REM --- Un misil enemigo nuevo, desde (x0,y0), hacia un blanco en pie ---
1310 DEF PROC lanzar(x0,y0)
1320 LOCAL i,k,n,l,a
1330 LET i=0: FOR k=1 TO 12: IF e(k,1)=0 AND i=0 THEN LET i=k
1340 NEXT k
1350 IF i THEN
1360 LET n=0
1370 REPEAT: LET k=1+INT (RND*9): LET n=n+1: UNTIL t(k,3) OR n>12
1375 IF i>he THEN LET he=i
1380 LET e(i,1)=1: LET e(i,2)=x0: LET e(i,3)=y0: LET e(i,4)=x0: LET e(i,5)=y0: LET e(i,8)=k
1390 LET a=t(k,1)-x0: LET l=t(k,2)-y0: LET l=SQR (a*a+l*l)
1400 LET e(i,6)=(t(k,1)-x0)/l*vel: LET e(i,7)=(t(k,2)-y0)/l*vel
1410 LET e(i,9)=ola>=3 AND y0<40 AND RND<.3
1420 END IF
1430 END PROC
1500 REM Un misil enemigo llega a su blanco
1510 DEF PROC impacto(i)
1520 LOCAL k
1530 LET k=e(i,8): LET e(i,1)=0
1540 PROC explotar(t(k,1),t(k,2)-4)
1550 IF t(k,3) THEN
1560 LET t(k,3)=0
1570 SELECT CASE k
1580 CASE 7 TO 9: LET z(k-6)=0
1590 CASE ELSE: LET ciudades=ciudades-1
1600 END SELECT
1610 LET xv=15: SOUND 22,31
1620 END IF
1630 END PROC
1700 REM Cada 10000 puntos vuelve una ciudad
1710 DEF PROC premio
1720 LOCAL k
1730 LET extra=extra+10000
1740 FOR k=1 TO 6
1750 IF t(k,3)=0 THEN LET t(k,3)=1: LET ciudades=ciudades+1: SOUND 16,60; 17,0; 24,14: LET fv=14: LET fp=60: LET k=6
1760 NEXT k
1770 END PROC
1800 REM --- La demostracion: apunta al misil mas bajo, un poco por delante ---
1810 DEF PROC piloto
1820 LOCAL i,b,ax,ay
1830 LET b=0: LET ay=0: LET bt=0
1840 FOR i=1 TO he: IF e(i,1) THEN IF e(i,5)>ay THEN LET ay=e(i,5): LET b=i
1850 NEXT i
1860 IF b THEN
1870 LET ax=e(b,4)+e(b,6)*30: LET ay=e(b,5)+e(b,7)*30
1880 LET mx=mx+SGN (ax-mx)*4*(ABS (ax-mx)>4): LET my=my+SGN (ay-my)*4*(ABS (ay-my)>4)
1890 IF ABS (ax-mx)<8 AND ABS (ay-my)<8 AND fr>=cd THEN LET bt=1: LET cd=fr+20
1900 END IF
1910 IF fr>1500 OR INKEY$<>"" OR MOUSE 2 OR JOY 0 THEN LET quit=1
1920 END PROC
2000 REM --- Empezar: el idioma, las tablas, los graficos y el record ---
2010 @empezar: BORDER 0: PAPER 0: INK 7: CLS: CLEAR
2012 REM Las variables se buscan en el orden en que se crearon: primero, las de cada frame
2014 DIM e(12,9): DIM x(12,3): DIM p(6,10): DIM t(9,3): DIM z(3): DIM c(10)
2016 LET i=0: LET k=0: LET he=0: LET u=0: LET v=0: LET r=0: LET a=0: LET tc=0: LET fr=0: LET s=0
2018 LET vivos=0: LET expl=0: LET mx=0: LET my=0: LET mult=1: LET puntos=0: LET fv=0: LET xv=0: LET ac=0: LET sp=2
2020 LET en=PEEK 536872736<>101 OR PEEK 536872737<>115: REM no es "es": en ingles
2030 DEF FN t$(a$,b$)=(a$ AND NOT en)+(b$ AND en)
2040 DEF FN b(j,n)=INT (j/2^n)-2*INT (j/2^(n+1))
2050 REM e: misiles enemigos (vivo, x0, y0, x, y, vx, vy, blanco, se divide)
2060 REM p: los tuyos (vivo, x0, y0, x, y, vx, vy, pasos, x y del blanco)
2070 REM x: explosiones (fase, x, y); t: blancos (x, y, en pie); z: municion
2090 RESTORE @blancos: FOR k=1 TO 9: READ t(k,1),t(k,2): NEXT k
2100 FOR k=1 TO 10: READ c(k): NEXT k
2110 LET o$=FN t$("OLEADA ","WAVE "): LET d$=FN t$("DEMOSTRACION","    DEMO")
2120 LET hi=0: PROC leer: PROC graficos
2130 REPEAT
2140 PROC titulo
2150 PROC partida
2160 UNTIL 0
2200 REM --- Los graficos: se dibujan en la pagina 1 y se copian con GRAB ---
2210 DEF PROC graficos
2220 SCREEN 1: CLS
2230 REM La ciudad (sprite 1) y sus ruinas (2), de 20 x 12
2240 BOX 5,0,4,6,8: BOX 13,7,0,6,12: BOX 9,14,3,6,9
2250 RESTORE @ventanas: FOR k=1 TO 9: READ a,b: POLY 14,a,b: NEXT k
2260 GRAB 1,0,0,20,12,0
2270 POLY 2,24,11,27,7,30,9,34,5,38,9,43,11: POLY 10,29,9,33,7,36,9
2280 GRAB 2,24,0,20,12,0
2290 REM La base (3) y la base destruida (4), de 29 x 11
2300 POLY 6,0,32,6,22,22,22,28,32: BOX 14,11,22,6,3: BOX 0,13,23,2,2
2310 GRAB 3,0,22,29,11,0
2320 POLY 2,32,32,36,28,44,30,50,27,56,32
2330 GRAB 4,32,22,29,11,0
2340 REM La mira (6) y la marca del blanco (7)
2350 GRAB 6,9,"....f...."+"....f...."+"....f...."+"........."+"fff...fff"+"........."+"....f...."+"....f...."+"....f...."
2360 GRAB 7,5,"d...d"+".d.d."+"..d.."+".d.d."+"d...d"
2370 REM El fondo (20): estrellas, la luna, las montanas y el suelo
2380 CLS
2390 FOR k=1 TO 70: POLY 7+8*(RND<.3),RND*255,12+RND*140: NEXT k
2400 DISC 15,224,78,10: DISC 0,229,74,9
2410 POLY 1,0,176,0,152,30,134,60,150,95,128,130,152,165,132,200,148,230,130,255,146,255,176
2420 BOX 6,0,176,256,16: BOX 14,0,176,256,1
2430 GRAB 20,0,0,256,192
2440 REM El nombre (21): se escribe y se copia, para dibujarlo grande
2450 PRINT AT 21,0; INK 6; BRIGHT 1; PAPER 0; "SENTRY"
2460 GRAB 21,0,168,72,8,0
2470 SCREEN 0: CLS
2480 END PROC
2500 REM --- El titulo: el nombre, las teclas y el record; sin tocar nada, la demostracion ---
2510 DEF PROC titulo
2520 LET s=1: LET t=0: LET demo=0
2530 REPEAT
2540 SCREEN s: SPRITE 20,0,0
2550 SPRITE 21,22,30,0,"..............2",216,24: SPRITE 21,20,28,0,"",216,24
2560 PROC centro(8,15,FN t$("DEFIENDE LAS CIUDADES","DEFEND THE CITIES"))
2570 PROC centro(9,15,FN t$("DE LOS MISILES","FROM THE MISSILES"))
2580 PROC centro(12,13,FN t$("RATON: APUNTAR Y DISPARAR","MOUSE: AIM AND FIRE"))
2590 PROC centro(13,13,FN t$("CURSORES Y ESPACIO","ARROWS AND SPACE"))
2595 PROC centro(14,13,FN t$("MANDO: CRUCETA Y BOTON","GAMEPAD: D-PAD AND BUTTON"))
2600 PROC centro(15,13,FN t$("ESC: SALIR","ESC: QUIT"))
2610 PROC centro(17,14,FN t$("RECORD ","HIGH SCORE ")+STR$ hi)
2620 IF INT (t/25)-2*INT (t/50) THEN PROC centro(19,15,FN t$("PULSA EL BOTON","PRESS THE BUTTON"))
2630 PAGE s: LET s=1-s: WAIT 1: LET t=t+1
2640 UNTIL MOUSE 2 OR KEY 32 OR JOY 0>=16 OR t>500
2650 LET demo=t>500
2660 END PROC
2700 REM Un texto centrado en la fila r, del color c (de 0 a 15: del 8 en adelante, con BRIGHT)
2710 DEF PROC centro(r,c,a$)
2720 PRINT AT r,INT ((32-LEN a$)/2); INK c-8*(c>7); BRIGHT (c>7); PAPER 0; a$
2730 END PROC
3000 REM --- Una partida: oleadas hasta que no quede ninguna ciudad ---
3010 DEF PROC partida
3020 LET puntos=0: LET ola=0: LET extra=10000: LET quit=0: LET fr=0
3030 LET mx=128: LET my=90: LET pb=1: LET cd=0: LET fv=0: LET xv=0
3040 FOR k=1 TO 9: LET t(k,3)=1: NEXT k
3050 SOUND 23,46: SOUND 24,0: SOUND 25,0
3060 REPEAT
3070 LET ola=ola+1: PROC oleada
3080 UNTIL ciudades=0 OR quit
3090 SOUND 24,0: SOUND 25,0
3100 IF NOT quit THEN
3110 SCREEN s: SPRITE 20,0,0: PROC escena
3120 PROC centro(9,10,FN t$(" FIN DE LA PARTIDA "," THE END "))
3130 PROC centro(11,14,FN t$("PUNTOS ","SCORE ")+STR$ puntos)
3140 IF puntos>hi AND NOT demo THEN
3150 LET hi=puntos: PROC grabar
3160 PROC centro(13,14,FN t$(" NUEVO RECORD! "," NEW HIGH SCORE! "))
3170 END IF
3180 PAGE s: LET s=1-s
3190 PLAY "T120O4 5c3gfe7d"
3200 PAUSE 150
3210 END IF
3220 END PROC
3300 REM --- Una oleada: cuantos misiles, a que velocidad y cada cuanto ---
3310 DEF PROC oleada
3320 FOR k=1 TO 12: LET e(k,1)=0: LET x(k,1)=0: NEXT k
3330 FOR k=1 TO 6: LET p(k,1)=0: NEXT k
3340 FOR k=1 TO 3: LET z(k)=10: LET t(6+k,3)=1: NEXT k
3350 LET nl=6+ola*2: IF nl>30 THEN LET nl=30
3360 LET vel=.3+ola*.06: IF vel>1.1 THEN LET vel=1.1
3370 LET li=60-ola*4: IF li<18 THEN LET li=18
3380 LET maxv=3+ola: IF maxv>10 THEN LET maxv=10
3390 LET mult=INT ((ola+1)/2): IF mult>6 THEN LET mult=6
3400 LET tc=c(4+ola-6*INT ((ola-1)/6)): REM el color de las estelas
3410 LET he=0: LET lanzados=0: LET vivos=0: LET expl=0: LET prox=fr+60: LET ciudades=0
3420 FOR k=1 TO 6: LET ciudades=ciudades+t(k,3): NEXT k
3430 SCREEN s: SPRITE 20,0,0: PROC escena
3440 PROC centro(10,15,o$+STR$ ola)
3450 PROC centro(12,14,FN t$("PUNTOS X","POINTS X")+STR$ mult)
3460 PAGE s: LET s=1-s
3470 IF NOT demo THEN PLAY "T200O5 3cgcg5C"
3480 IF demo THEN PAUSE 50
3490 PROC jugar
3500 IF ciudades AND NOT quit THEN PROC bonus
3510 END PROC
3600 REM --- El final de la oleada: los misiles y las ciudades que quedan ---
3610 DEF PROC bonus
3620 LOCAL k,m,n
3630 LET m=z(1)+z(2)+z(3): LET n=0
3640 FOR k=1 TO 6: LET n=n+t(k,3): NEXT k
3650 LET puntos=puntos+(m*5+n*100)*mult
3660 SCREEN s: SPRITE 20,0,0: PROC escena
3670 PROC centro(8,15,FN t$("OLEADA SUPERADA","WAVE CLEARED"))
3680 PROC centro(11,14,FN t$("MISILES ","MISSILES ")+STR$ m+" X "+STR$ (5*mult))
3690 PROC centro(13,14,FN t$("CIUDADES ","CITIES ")+STR$ n+" X "+STR$ (100*mult))
3700 PROC centro(16,13,FN t$("PUNTOS ","SCORE ")+STR$ puntos)
3710 PAGE s: LET s=1-s
3720 IF NOT demo THEN PLAY "T180O5 3egC5E"
3730 PAUSE 100
3740 END PROC
4000 REM --- El record, en la tarjeta (SD:, aunque se juegue desde ROM:) ---
4010 DEF PROC leer
4020 ON ERROR GO TO 4040
4030 OPEN #4,"SD:sentry.txt" IN: INPUT #4;hi
4040 ON ERROR GO TO 0: CLOSE #4
4050 END PROC
4100 DEF PROC grabar
4110 ON ERROR GO TO 4130
4120 OPEN #4,"SD:sentry.txt" OUT: PRINT #4;hi
4130 ON ERROR GO TO 0: CLOSE #4
4140 END PROC
9000 REM Los blancos: las seis ciudades y las tres bases (x, y)
9010 @blancos: DATA 50,168,76,168,102,168,154,168,180,168,206,168,20,168,128,168,236,168
9020 REM Los colores de las explosiones y de las estelas de cada oleada
9030 DATA 15,14,10,13,10,11,14,13,12,15
9040 REM Las ventanas de la ciudad
9050 @ventanas: DATA 2,6,2,9,9,2,9,5,9,8,11,4,11,7,16,5,16,8
