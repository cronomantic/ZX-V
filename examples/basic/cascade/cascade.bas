10 REM CASCADE: junta tres gemas iguales en linea y desaparecen
20 REM Un pozo de 6x12 casillas (un mapa de tiles de 16x16); la pareja que cae son
30 REM dos sprites, y todo se dibuja con doble buffer a 60 imagenes por segundo.
40 REM Graficos, musica y efectos en cascade.gfx, cascade.pt3 y cascade.afb.
50 GO TO @empezar
100 REM --- Una pareja: aparece arriba y cae hasta que se posa ---
110 DEF PROC pareja
120 LET ga=na: LET gb=nb: PROC siguiente
130 LET pc=4: LET o=0: LET yy=-16: LET lk=0: LET vp=1: PROC limite
140 IF demo THEN PROC piensa
150 REPEAT
160 LET fr=fr+1
170 PROC entrada
180 LET vc=sp: IF bj AND vc<6 THEN LET vc=6
190 LET yy=yy+vc: IF yy>=ym THEN LET yy=ym: LET lk=lk+1 ELSE LET lk=0
200 PROC dibujar
210 UNTIL lk>20-18*bj OR quit
220 IF NOT quit THEN PROC posa
230 END PROC
300 REM --- Hasta donde puede bajar la pareja: ym, en pixeles ---
310 REM o: donde esta la segunda gema (0 arriba, 1 derecha, 2 abajo, 3 izquierda)
320 DEF PROC limite
330 LET lr=13-n(pc): IF o=2 THEN LET lr=lr-1
340 IF o=1 OR o=3 THEN LET lc=13-n(pc+u(o+1)): IF lc<lr THEN LET lr=lc
350 LET ym=16*(lr-2)
360 END PROC
400 REM --- Moverse una columna (d: -1 o 1), si cabe ---
410 DEF PROC mueve(d)
420 LET c=pc+d: LET cs=c+u(o+1)
430 IF c<2 OR c>7 OR cs<2 OR cs>7 THEN END PROC
440 IF FN f(c,yy) AND FN f(cs,yy+16*v(o+1)) THEN LET pc=c: PROC limite: SFX 0
450 END PROC
500 REM --- Girar (d: 1 a la derecha, -1 a la izquierda); contra una pared,
510 REM se aparta; si no hay sitio para tumbarse, las dos gemas se cambian ---
520 DEF PROC gira(d)
530 LET oo=o+d: IF oo>3 THEN LET oo=0
540 IF oo<0 THEN LET oo=3
550 LET uu=u(oo+1): LET vv=v(oo+1): LET ok=0
560 IF pc+uu>=2 AND pc+uu<=7 THEN IF FN f(pc,yy) AND FN f(pc+uu,yy+16*vv) THEN LET ok=1
570 IF NOT ok AND uu<>0 THEN
580 LET c=pc-uu
590 IF c>=2 AND c<=7 THEN IF FN f(c,yy) AND FN f(c+uu,yy) THEN LET pc=c: LET ok=1
600 ELSE IF NOT ok AND vv=1 THEN
610 LET c=16*(10-n(pc)): IF c>=-16 AND c<yy THEN LET yy=c: LET ok=1
620 END IF
630 IF ok THEN LET o=oo: PROC limite: SFX 1 ELSE IF o=0 OR o=2 THEN LET c=ga: LET ga=gb: LET gb=c: SFX 1
640 END PROC
700 REM --- La pareja se posa: sus dos gemas pasan al pozo ---
710 DEF PROC posa
720 LET vp=0: LET jp=2+yy/16: LET c2=pc+u(o+1): LET j2=jp+v(o+1)
730 IF j2<1 THEN LET fin=1
740 LET g(pc,jp)=ga: IF jp>1 THEN MAP POKE 0,pc-2,jp-2,ga
750 IF j2>0 THEN LET g(c2,j2)=gb: IF j2>1 THEN MAP POKE 0,c2-2,j2-2,gb
760 SFX 2
770 END PROC
800 REM --- Lo que pasa despues: caen las gemas sueltas y se borran las lineas, una y
810 REM otra vez mientras haya: cada vuelta de mas es una cadena que vale mas ---
820 DEF PROC resolver
830 LET cad=0
840 REPEAT
850 PROC gravedad
860 LET nm=0
870 FOR i=2 TO 7: IF g(i,1) THEN LET fin=1
880 NEXT i
890 IF NOT fin THEN PROC busca
900 IF nm THEN LET cad=cad+1: PROC borra
910 UNTIL nm=0
920 IF cad>mc THEN LET mc=cad
930 END PROC
1000 REM --- La gravedad: cada columna se aprieta hacia abajo; lo que cae, cae
1010 REM como sprite (a, b, e, d: columna, y en pixeles, fila de llegada y color) ---
1020 DEF PROC gravedad
1030 LET nv=0
1040 FOR i=2 TO 7
1050 LET k=13
1060 FOR j=13 TO 1 STEP -1
1070 LET c=g(i,j)
1080 IF c THEN
1090 IF j<>k THEN LET nv=nv+1: LET a(nv)=i: LET b(nv)=16*(j-2): LET e(nv)=k: LET d(nv)=c: LET g(i,j)=0: LET g(i,k)=c: IF j>1 THEN MAP POKE 0,i-2,j-2,0
1100 LET k=k-1
1110 END IF
1120 NEXT j
1130 LET n(i)=13-k
1140 NEXT i
1150 IF nv=0 THEN END PROC
1160 LET an=1: LET vg=1
1170 REPEAT
1180 LET mv=0: LET vg=vg+.5: IF vg>8 THEN LET vg=8
1190 FOR l=1 TO nv
1200 LET k=16*(e(l)-2)
1210 IF b(l)<k THEN LET b(l)=b(l)+vg: IF b(l)>=k THEN LET b(l)=k: IF e(l)>1 THEN MAP POKE 0,a(l)-2,e(l)-2,d(l)
1220 IF b(l)<k THEN LET mv=1
1230 NEXT l
1240 PROC dibujar
1250 UNTIL mv=0
1260 LET an=0: SFX 2
1270 END PROC
1300 REM --- Buscar lineas de tres o mas: se marcan en m; nm dice cuantas gemas ---
1310 REM w, z: las cuatro direcciones (derecha, abajo y las dos diagonales)
1320 DEF PROC busca
1330 LET nm=0: LET pz=0
1340 FOR i=2 TO 7: FOR j=2 TO 13: LET m(i,j)=0: NEXT j: NEXT i
1350 FOR i=2 TO 7: FOR j=2 TO 13
1360 LET c=g(i,j)
1370 IF c=7 THEN
1380 PROC prisma
1390 ELSE IF c THEN
1400 FOR l=1 TO 4
1410 LET dx=w(l): LET dy=z(l)
1420 REM Solo desde la primera gema de cada linea: la de antes es de otro color
1430 IF g(i-dx,j-dy)<>c THEN
1440 LET k=0: REPEAT: LET k=k+1: UNTIL g(i+k*dx,j+k*dy)<>c
1450 IF k>=3 THEN FOR t=0 TO k-1: LET m(i+t*dx,j+t*dy)=1: NEXT t
1460 END IF
1470 NEXT l
1480 END IF
1490 NEXT j: NEXT i
1500 FOR i=2 TO 7: FOR j=2 TO 13: IF m(i,j) THEN LET nm=nm+1
1510 NEXT j: NEXT i
1520 END PROC
1600 REM El prisma borra todas las gemas del color sobre el que cae (y a si mismo)
1610 DEF PROC prisma
1620 LET pz=1: LET m(i,j)=1: LET k=g(i,j+1)
1630 IF k<1 OR k>6 THEN END PROC
1640 FOR t=2 TO 7: FOR h=2 TO 13: IF g(t,h)=k THEN LET m(t,h)=1
1650 NEXT h: NEXT t
1660 END PROC
1700 REM --- Borrar lo marcado: parpadea, brilla y desaparece; los puntos ---
1710 DEF PROC borra
1720 LET k=cad: IF k>4 THEN LET k=4
1730 IF pz THEN SFX 7 ELSE SFX 2+k
1740 FOR q=0 TO 7
1750 FOR i=2 TO 7: FOR j=2 TO 13: IF m(i,j) THEN MAP POKE 0,i-2,j-2,(8 AND q<6 AND q-2*INT (q/2)=0)+(g(i,j) AND q<6 AND q-2*INT (q/2)=1)+(9 AND q>5)
1760 NEXT j: NEXT i
1770 FOR h=1 TO 3: PROC dibujar: NEXT h
1780 NEXT q
1790 FOR i=2 TO 7: FOR j=2 TO 13: IF m(i,j) THEN LET g(i,j)=0: MAP POKE 0,i-2,j-2,0
1800 NEXT j: NEXT i
1810 LET puntos=puntos+nm*10*cad*niv+100*pz: LET gemas=gemas+nm: LET pd=2
1820 IF puntos>hm AND NOT demo THEN LET hm=puntos
1830 IF cad>1 THEN LET m$=FN t$("CADENA","CHAIN"): LET n$="  x"+STR$ cad: LET mt=90
1840 LET k=1+INT (gemas/30): IF k>20 THEN LET k=20
1850 IF k>niv THEN LET niv=k: PROC velocidad: LET m$=FN t$("NIVEL","LEVEL"): LET n$="   "+STR$ niv: LET mt=120: SFX 8
1860 END PROC
1900 REM --- Dibujar: el pozo, la pareja o lo que cae, el marcador y los avisos ---
1910 DEF PROC dibujar
1920 SCREEN s: MAP 0,0,0,88,0,96,192
1930 IF vp THEN LET xx=88+16*(pc-2): SPRITE ga,xx,yy: SPRITE gb,xx+16*u(o+1),yy+16*v(o+1)
1940 IF an THEN
1950 FOR l=1 TO nv
1960 IF b(l)<16*(e(l)-2) THEN SPRITE d(l),88+16*(a(l)-2),b(l)
1970 NEXT l
1980 END IF
1990 IF demo THEN PRINT AT 0,11; INK 7; BRIGHT 1; PAPER 0; d$
2000 IF e$<>"" THEN PRINT AT 10,17-LEN e$/2; INK 6; BRIGHT 1; PAPER 0; e$; AT 12,17-LEN f$/2; f$
2010 IF pd THEN PROC marcador: LET pd=pd-1
2020 IF mt THEN
2030 IF mt>2 THEN PRINT AT 10,24; INK 1+INT (fr/4)-7*INT (fr/28); BRIGHT 1; PAPER 0; m$; AT 11,24; n$ ELSE PRINT AT 10,24; PAPER 0; "        "; AT 11,24; "        "
2040 LET mt=mt-1
2050 END IF
2060 PAGE s: LET s=1-s
2070 WAIT 1
2080 END PROC
2100 DEF PROC marcador
2110 PRINT AT 2,1; INK 7; BRIGHT 1; PAPER 0; puntos; "   "; AT 5,1; INK 6; hm; "   "; AT 8,1; INK 7; niv; "  "; AT 11,1; gemas; "   "
2120 BOX 0,208,20,32,36: SPRITE nb,216,22: SPRITE na,216,38
2130 END PROC
2200 REM --- La entrada: los cursores, OPQA, espacio o un mando (el piloto en la demostracion) ---
2210 DEF PROC entrada
2220 IF demo THEN
2230 IF INKEY$<>"" OR JOY 0 OR fr>60*60 THEN LET quit=1
2240 LET pt=pt+1: LET bj=0
2250 IF o=tn AND pc=tc THEN
2260 LET bj=pt>20
2270 ELSE IF pt>=6 THEN
2280 LET pt=0: IF o<>tn THEN PROC gira(1) ELSE IF pc<tc THEN PROC mueve(1) ELSE PROC mueve(-1)
2290 END IF
2300 ELSE
2310 LET j=JOY 0
2320 LET kl=KEY 8 OR KEY 111 OR INT (j/2)-2*INT (j/4): LET kr=KEY 9 OR KEY 112 OR j-2*INT (j/2)
2330 LET bj=KEY 10 OR KEY 97 OR INT (j/4)-2*INT (j/8)
2340 LET kg=KEY 11 OR KEY 113 OR KEY 32 OR INT (j/8)-2*INT (j/16) OR INT (j/16)-2*INT (j/32): LET ka=KEY 122 OR INT (j/32)-2*INT (j/64)
2350 LET dd=kr-kl
2360 IF dd=0 THEN LET rp=0 ELSE LET rp=rp+1: IF rp=1 OR (rp>12 AND rp-4*INT (rp/4)=0) THEN PROC mueve(dd)
2370 IF kg AND NOT og THEN PROC gira(1)
2380 IF ka AND NOT oa THEN PROC gira(-1)
2390 LET og=kg: LET oa=ka
2400 IF KEY 109 AND NOT om THEN LET mu=1-mu: IF mu THEN MUSIC CONTINUE ELSE MUSIC PAUSE
2410 LET om=KEY 109
2420 END IF
2430 END PROC
2500 REM --- El piloto de la demostracion: prueba cada columna y cada giro ---
2510 DEF PROC piensa
2520 LET bs=-1E9: LET tc=4: LET tn=0: LET pt=0
2530 FOR t=1 TO 7: LET x(t)=0: NEXT t
2540 FOR i=2 TO 7: FOR j=2 TO 13: LET c=g(i,j): IF c THEN LET x(c)=x(c)+1
2550 NEXT j: NEXT i
2560 FOR t=0 TO 3: FOR q=2 TO 7
2570 PROC valora
2580 IF pv>bs THEN LET bs=pv: LET tc=q: LET tn=t
2590 NEXT q: NEXT t
2600 END PROC
2610 REM Cuanto vale dejar la pareja en la columna q con el giro t
2620 DEF PROC valora
2630 LET uu=u(t+1): LET cs=q+uu: LET pv=-1E9
2640 IF cs<2 OR cs>7 THEN END PROC
2650 IF uu=0 THEN
2660 LET j1=13-n(q): LET j2=j1-1: IF t=2 THEN LET j2=j1: LET j1=j1-1
2670 ELSE
2680 LET j1=13-n(q): LET j2=13-n(cs)
2690 END IF
2700 LET pv=-1E6+j1+j2: IF j1<2 OR j2<2 THEN END PROC
2710 LET g(q,j1)=ga: LET g(cs,j2)=gb
2720 LET pv=RND*3-3*(26-j1-j2): IF j1<5 OR j2<5 THEN LET pv=pv-300
2730 LET i=q: LET j=j1: LET c=ga: PROC linea
2740 LET i=cs: LET j=j2: LET c=gb: PROC linea
2750 LET g(q,j1)=0: LET g(cs,j2)=0
2760 END PROC
2770 REM Las lineas que haria la gema c en (i, j): las de tres valen mucho
2780 DEF PROC linea
2790 IF c=7 THEN
2800 LET k=g(i,j+1): IF k>=1 AND k<=6 THEN LET pv=pv+20*x(k) ELSE LET pv=pv-10
2810 ELSE
2820 FOR l=1 TO 4
2830 LET dx=w(l): LET dy=z(l)
2840 LET r1=0: REPEAT: LET r1=r1+1: UNTIL g(i+r1*dx,j+r1*dy)<>c
2850 LET r2=0: REPEAT: LET r2=r2+1: UNTIL g(i-r2*dx,j-r2*dy)<>c
2860 LET k=r1+r2-1: IF k>=3 THEN LET pv=pv+60*k ELSE IF k=2 THEN LET pv=pv+5
2870 NEXT l
2880 END IF
2890 END PROC
3000 REM --- La pareja siguiente: de q$ si lo hay (para las pruebas) o al azar ---
3010 DEF PROC siguiente
3020 LET np=np+1: LET pd=2
3030 IF qi<LEN q$ THEN LET na=VAL q$(qi): LET nb=VAL q$(qi+1): LET qi=qi+2: END PROC
3040 LET na=1+INT (RND*nc): LET nb=1+INT (RND*nc)
3050 IF np>5 AND RND<.06 THEN LET nb=7
3060 END PROC
3100 REM La velocidad (pixeles por imagen) y cuantos colores hay en cada nivel
3110 DEF PROC velocidad
3120 LET sp=.5+.2*(niv-1): IF sp>3.5 THEN LET sp=3.5
3130 LET nc=4+(niv>=3)+(niv>=6)
3140 END PROC
3200 REM --- Una partida: parejas hasta que el pozo se desborda ---
3210 DEF PROC partida
3220 FOR i=1 TO 8: FOR j=1 TO 14: LET g(i,j)=99*(i=1 OR i=8 OR j=14): NEXT j: NEXT i
3230 FOR i=2 TO 7: LET n(i)=0: NEXT i: LET n(1)=99: LET n(8)=99
3240 MAP DIM 0,6,12,16
3250 LET puntos=0: LET niv=1: LET gemas=0: LET fin=0: LET quit=0: LET fr=0: LET mc=0: LET hm=hi
3260 LET np=0: LET qi=1: LET mt=0: LET vp=0: LET an=0: LET e$="": LET f$="": LET rp=0
3270 IF p$<>"" THEN PROC preparar
3280 PROC velocidad: PROC siguiente: PROC fondo
3290 IF mu THEN MUSIC PLAY
3300 LET e$=FN t$("LISTO!","READY!"): SFX 11: FOR k=1 TO 60: PROC dibujar: NEXT k: LET e$=""
3310 REPEAT
3320 PROC pareja
3330 IF NOT quit THEN PROC resolver
3340 UNTIL fin OR quit
3350 IF fin THEN PROC final
3360 MUSIC STOP: SFX STOP
3370 END PROC
3400 REM --- Se ha desbordado: las gemas se vuelven piedra, de abajo arriba ---
3410 DEF PROC final
3420 LET vp=0: MUSIC STOP: SFX 9
3430 FOR j=13 TO 2 STEP -1
3440 FOR i=2 TO 7: IF g(i,j) THEN MAP POKE 0,i-2,j-2,10
3450 NEXT i
3460 PROC dibujar: PROC dibujar: PROC dibujar
3470 NEXT j
3480 LET e$=FN t$("FIN DE","GAME"): LET f$=FN t$("PARTIDA","OVER")
3490 FOR k=1 TO 120: PROC dibujar: NEXT k
3500 IF puntos>hi AND NOT demo THEN
3510 LET hi=puntos: PROC grabar: SFX 10
3520 LET e$=FN t$("NUEVO","NEW"): LET f$=FN t$("RECORD!","RECORD!")
3530 FOR k=1 TO 120: PROC dibujar: NEXT k
3540 END IF
3550 LET e$="": LET f$=""
3560 END PROC
3600 REM Para las pruebas: el pozo empieza como dice p$ (fila a fila, de arriba abajo)
3610 DEF PROC preparar
3620 FOR j=2 TO 13: FOR i=2 TO 7
3630 LET c$=p$((j-2)*6+i-1): IF c$<>"." THEN LET g(i,j)=VAL c$: MAP POKE 0,i-2,j-2,g(i,j)
3640 NEXT i: NEXT j
3650 PROC gravedad
3660 END PROC
3700 REM --- El fondo de la partida, en las dos paginas: paredes y rotulos ---
3710 DEF PROC fondo
3720 FOR k=0 TO 1
3730 SCREEN k: CLS
3740 MAP 1,0,0,80,0,8,192: MAP 1,0,0,184,0,8,192
3750 PRINT AT 1,0; INK 5; BRIGHT 1; PAPER 0; FN t$("PUNTOS","SCORE"); AT 4,0; FN t$("RECORD","HIGH"); AT 7,0; FN t$("NIVEL","LEVEL"); AT 10,0; FN t$("GEMAS","GEMS")
3760 PRINT AT 1,24; INK 5; BRIGHT 1; PAPER 0; FN t$("SIGUE","NEXT")
3770 PRINT AT 20,0; INK 7; BRIGHT 0; PAPER 0; "CASCADE"; AT 21,0; FN t$("M: MUSICA","M: MUSIC")
3780 NEXT k
3790 LET s=1: LET pd=2
3800 END PROC
4000 REM --- El titulo: el nombre, las reglas, las teclas y el record ---
4010 DEF PROC titulo
4020 LET demo=0: LET t=0
4030 FOR s=0 TO 1: SCREEN s: CLS: PROC portada: NEXT s
4040 LET s=1
4050 REPEAT
4060 SCREEN s: BOX 0,0,0,16,192: BOX 0,240,0,16,192: BOX 0,16,168,224,8
4070 IF INT (t/30)-2*INT (t/60) THEN PROC centro(21,15,FN t$("PULSA ESPACIO O EL BOTON","PRESS SPACE OR FIRE"))
4080 REM A los lados caen gemas
4090 FOR l=0 TO 5
4100 LET a=t*1.5+l*36: LET k=a-216*INT (a/216): LET c=1+(l+INT (a/216))-6*INT ((l+INT (a/216))/6)
4110 SPRITE c,0,k-24: SPRITE 7-c,240,192-k
4120 NEXT l
4130 PAGE s: LET s=1-s: WAIT 1: LET t=t+1
4140 UNTIL KEY 32 OR KEY 13 OR JOY 0>=16 OR t>10*60
4150 LET demo=t>10*60
4160 END PROC
4200 DEF PROC portada
4210 SPRITE 20,42,2,0,"",172,36
4220 FOR l=1 TO 6: SPRITE l,72+16*l,40: NEXT l
4230 PROC centro(7,7,FN t$("JUNTA TRES GEMAS IGUALES","LINE UP THREE MATCHING GEMS"))
4240 PROC centro(8,7,FN t$("EN FILA, COLUMNA O DIAGONAL","ACROSS, DOWN OR DIAGONALLY"))
4250 PROC centro(9,7,FN t$("LAS DE ARRIBA CAEN: CADENAS!","THE REST FALL: CHAINS!"))
4260 SPRITE 7,40,86: PRINT AT 11,8; INK 7; BRIGHT 1; PAPER 0; FN t$("EL PRISMA BORRA","THE PRISM CLEARS"); AT 12,8; FN t$("TODO UN COLOR","A WHOLE COLOUR")
4270 PROC centro(14,13,FN t$("CURSORES U O P: MOVER","ARROWS OR O P: MOVE"))
4280 PROC centro(15,13,FN t$("ARRIBA, Q O ESPACIO: GIRAR","UP, Q OR SPACE: ROTATE"))
4290 PROC centro(16,13,FN t$("ABAJO O A: BAJAR DEPRISA","DOWN OR A: DROP FASTER"))
4300 PROC centro(17,13,FN t$("MANDO: CRUCETA Y BOTONES","GAMEPAD: D-PAD AND BUTTONS"))
4310 PROC centro(18,13,FN t$("M: MUSICA   ESC: SALIR","M: MUSIC   ESC: QUIT"))
4320 PROC centro(20,14,FN t$("RECORD ","HIGH SCORE ")+STR$ hi)
4330 END PROC
4400 REM Un texto centrado en la fila r, del color c (de 0 a 15: del 8 en adelante, con BRIGHT)
4410 DEF PROC centro(r,c,a$)
4420 PRINT AT r,INT ((32-LEN a$)/2); INK c-8*(c>7); BRIGHT (c>7); PAPER 0; a$
4430 END PROC
5000 REM --- Empezar: el idioma, las tablas, los graficos, la musica y el record ---
5010 @empezar: BORDER 0: PAPER 0: INK 7: MODE 0: CLS: CLEAR
5020 REM Las variables se buscan en el orden en que se crearon: primero, las de cada frame
5030 DIM g(8,14): DIM m(8,14): DIM n(9): DIM u(4): DIM v(4): DIM w(4): DIM z(4)
5040 DIM a(80): DIM b(80): DIM e(80): DIM d(80): DIM x(7)
5050 LET fr=0: LET s=0: LET yy=0: LET ym=0: LET pc=4: LET o=0: LET ga=1: LET gb=1: LET vp=0: LET an=0: LET nv=0
5060 LET sp=.5: LET vc=0: LET lk=0: LET bj=0: LET demo=0: LET quit=0: LET pd=0: LET mt=0: LET xx=0
5070 LET i=0: LET j=0: LET k=0: LET l=0: LET c=0: LET t=0: LET q=0: LET h=0
5080 LET kl=0: LET kr=0: LET kg=0: LET ka=0: LET dd=0: LET rp=0: LET og=0: LET oa=0: LET om=0: LET mu=1
5090 LET pt=0: LET tc=4: LET tn=0
5100 LET lr=0: LET lc=0: LET cs=0: LET oo=0: LET uu=0: LET vv=0: LET ok=0: LET dx=0: LET dy=0
5110 LET puntos=0: LET hm=0: LET niv=1: LET gemas=0: LET na=1: LET nb=1: LET nc=4: LET np=0: LET qi=1
5120 LET en=PEEK 536872736<>101 OR PEEK 536872737<>115: REM no es "es": en ingles
5130 DEF FN t$(a$,b$)=(a$ AND NOT en)+(b$ AND en)
5140 REM g: el pozo, columnas 2-7 y filas 2-13 (la 1 es la de encima, que no se ve);
5150 REM la columna 1, la 8 y la fila 14 son paredes y suelo (99). n: gemas por columna
5160 REM FN f: si una gema en la columna c, a la altura y (pixeles), cabe sin chocar
5170 DEF FN f(c,y)=2+INT ((y+15)/16)<=13-n(c)
5180 RESTORE @tablas
5190 FOR k=1 TO 4: READ u(k): NEXT k: FOR k=1 TO 4: READ v(k): NEXT k
5200 FOR k=1 TO 4: READ w(k): NEXT k: FOR k=1 TO 4: READ z(k): NEXT k
5210 LET d$=FN t$("DEMOSTRACION","    DEMO    "): LET m$="": LET n$="": LET e$="": LET f$=""
5220 LET p$="": LET q$=""
5230 REM Graficos (sprites 1-7 las gemas y 20 el nombre; tiles de 16: el pozo; mapa 1:
5240 REM la pared), musica y efectos: de la carpeta actual o de la del programa
5250 PROC carga(1): PROC carga(2): PROC carga(3)
5260 LET hi=0: PROC leer
5270 REPEAT
5280 PROC titulo
5290 PROC partida
5300 UNTIL 0
7000 REM --- El record, en la tarjeta (SD:, aunque se juegue desde ROM:) ---
7010 DEF PROC leer
7020 ON ERROR GO TO 7040
7030 OPEN #4,"SD:cascade.txt" IN: INPUT #4;hi
7040 ON ERROR GO TO 0: CLOSE #4
7050 END PROC
7100 DEF PROC grabar
7110 ON ERROR GO TO 7130
7120 OPEN #4,"SD:cascade.txt" OUT: PRINT #4;hi
7130 ON ERROR GO TO 0: CLOSE #4
7140 END PROC
7200 REM --- Cargar cascade.gfx (k=1), .pt3 (2) o .afb (3): LOAD lo busca en la carpeta
7210 REM actual y, si no esta, en la del programa (ROM:, si se cargo de ahi). Si no
7215 REM se puede, lo dice y se para con el error de LOAD ---
7220 DEF PROC carga(k)
7230 ON ERROR GO TO 7300
7240 IF k=1 THEN LOAD "cascade" GRAPHICS ELSE IF k=2 THEN LOAD "cascade" MUSIC ELSE LOAD "cascade" SFX
7250 ON ERROR GO TO 0: END PROC
7300 PRINT AT 10,0; INK 7; PAPER 0; FN t$("No se puede cargar ","Cannot load ")+"cascade."+("gfx" AND k=1)+("pt3" AND k=2)+("afb" AND k=3)
7310 IF k=1 THEN LOAD "cascade" GRAPHICS ELSE IF k=2 THEN LOAD "cascade" MUSIC ELSE LOAD "cascade" SFX
7320 END PROC
9000 REM Donde esta la segunda gema de la pareja (u, v) con cada giro: arriba, derecha,
9010 REM abajo, izquierda; y las direcciones de las lineas (w, z)
9020 @tablas: DATA 0,1,0,-1,-1,0,1,0
9030 DATA 1,0,1,1,0,1,1,-1
