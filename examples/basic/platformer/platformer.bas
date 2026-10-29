10 REM ZX-V: un juego de plataformas en BASIC
20 REM Graficos: platformer.gfx (png2gfx: tiles.png, sprites.png, level.png)
30 BORDER 0: PAPER 0: INK 7: CLS
40 LOAD "platformer" GRAPHICS
50 REM Tiles: 0 cielo, 1 hierba, 2 tierra, 3 ladrillo (1-3 solidos), 4 moneda, 5 pinchos, 6 meta, 7 nube
60 DEF FN s(x,y)=ABS (CELL(0,INT (x/8),INT (y/8))-2)<=1
70 DEF FN t(x,y)=CELL(0,INT (x/8),INT (y/8))
80 DEF FN b(j,n)=INT (j/2^n)-2*INT (j/2^(n+1))
90 DIM e(4): DIM f(4): DIM g(4): DIM a(4)
100 LET hi=0: PROC leer
110 @titulo: PROC titulo
120 PROC partida
130 GO TO @titulo
1000 REM --- Titulo (sin tocar nada, juega solo) ---
1010 DEF PROC titulo
1020 SCREEN 0: PAGE 0: SCROLL: CLS
1030 MAP 0,0,0,0,16,256,160
1040 PRINT AT 3,4; PAPER 1; INK 7; BRIGHT 1; " P L A T A F O R M A S "
1050 PRINT AT 5,10; PAPER 0; INK 6; " Record "; hi; " "
1060 PRINT AT 20,0; PAPER 0; INK 7; " O, P o cursores: andar         "; AT 21,0; " Q, espacio o arriba: saltar    "
1070 SPRITE 0,56,136: SPRITE 2,120,136,1
1080 PLAY "T150O5 3cegC5g3eC7C"
1090 LET demo=1: LET t0=TIME
1100 REPEAT: WAIT 1: IF INKEY$<>"" OR JOY 0>0 THEN LET demo=0
1110 UNTIL demo=0 OR TIME-t0>6*60
1120 END PROC
2000 REM --- Una partida ---
2010 DEF PROC partida
2020 LOAD "platformer" GRAPHICS: REM el nivel con todas sus monedas
2030 LET puntos=0: LET vidas=3: LET gana=0
2040 RESTORE @bichos: FOR k=1 TO 4: READ e(k),f(k): LET g(k)=-1: LET a(k)=1: NEXT k
2050 SCREEN 1: CLS: SCREEN 0: CLS: SOUND 7,62
2060 REPEAT: PROC vida: UNTIL vidas=0 OR gana
2070 SOUND 8,0: SCREEN 0: PAGE 0: SCROLL: CLIP
2080 IF gana THEN PRINT AT 9,7; PAPER 4; INK 0; " NIVEL SUPERADO! ": PLAY "T200O5 3ceg5C3g7C"
2090 IF NOT gana THEN PRINT AT 9,7; PAPER 2; INK 7; " FIN DE LA PARTIDA "
2100 IF puntos>hi AND NOT demo THEN LET hi=puntos: PROC grabar: PRINT AT 11,9; PAPER 0; INK 6; " Nuevo record! "
2110 PAUSE 150
2120 END PROC
3000 REM --- Una vida: el bucle del juego ---
3010 DEF PROC vida
3020 LET px=16: LET py=120: LET vx=0: LET vy=0: LET suelo=0: LET muerto=0: LET fl=0: LET n=0: LET s=1: LET vol=0
3030 REPEAT
3040 LET n=n+1: LET iz=0: LET de=0: LET sa=0
3050 IF demo THEN PROC piloto: GO TO 3100
3060 LET j=JOY 0: LET iz=KEY 8 OR KEY CODE "o" OR FN b(j,1): LET de=KEY 9 OR KEY CODE "p" OR FN b(j,0)
3070 LET sa=KEY 11 OR KEY CODE "q" OR KEY 32 OR FN b(j,3) OR FN b(j,4)
3100 REM Movimiento, en pixeles por vuelta (30 por segundo): 75 por segundo andando; el salto
3101 REM hace la misma parabola que con 25 vueltas (-8 y .8 por vuelta)
3110 IF iz THEN LET vx=-2.5: LET fl=1
3120 IF de THEN LET vx=2.5: LET fl=0
3130 IF NOT iz AND NOT de THEN LET vx=0
3140 IF sa AND suelo THEN LET vy=-119/18: SOUND 0,150;1,0;8,13: LET vol=13
3150 LET vy=vy+5/9: IF vy>20/3 THEN LET vy=20/3
3160 LET nx=px+vx
3170 IF vx>0 THEN IF FN s(nx+13,py+1) OR FN s(nx+13,py+14) THEN LET nx=INT ((nx+13)/8)*8-14
3180 IF vx<0 THEN IF FN s(nx+2,py+1) OR FN s(nx+2,py+14) THEN LET nx=INT ((nx+2)/8)*8+6
3190 IF nx<0 THEN LET nx=0
3200 IF nx>1264 THEN LET nx=1264
3210 LET px=nx: LET ny=py+vy: LET suelo=0
3220 IF vy>0 THEN IF FN s(px+3,ny+16) OR FN s(px+12,ny+16) THEN LET ny=INT ((ny+16)/8)*8-16: LET vy=0: LET suelo=1
3230 IF vy<0 THEN IF FN s(px+3,ny) OR FN s(px+12,ny) THEN LET ny=INT (ny/8)*8+8: LET vy=0
3240 LET py=ny: IF py>150 THEN LET muerto=1
3250 REM Monedas, pinchos y meta
3260 LET t=FN t(px+8,py+8)
3270 IF t=4 THEN MAP POKE 0,INT ((px+8)/8),INT ((py+8)/8),0: LET puntos=puntos+10: SOUND 0,40;1,0;8,15: LET vol=15
3280 IF t=5 OR FN t(px+8,py+15)=5 THEN LET muerto=1
3290 IF t=6 THEN LET gana=1
3300 REM Bichos: andan hasta un muro o un borde; si se les pisa, mueren
3310 FOR k=1 TO 4
3320 IF NOT a(k) THEN GO TO 3360
3330 LET nx=e(k)+g(k)*5/3: LET ex=nx+(g(k)>0)*15
3340 IF FN s(ex,f(k)+8) OR NOT FN s(ex,f(k)+17) THEN LET g(k)=-g(k) ELSE LET e(k)=nx
3350 IF ABS (e(k)-px)<15 AND ABS (f(k)-py)<16 THEN IF HIT(0,px,py,2,e(k),f(k)) THEN IF vy>0 AND py<f(k) THEN LET a(k)=0: LET vy=-89/18: LET puntos=puntos+50: SOUND 0,90;1,0;8,15: LET vol=15 ELSE LET muerto=1
3360 NEXT k
3400 REM Dibujo: en la pagina oculta, que se ensena en el VBLANK; solo en la zona del
3401 REM nivel, que es la que se redibuja (al caer por un foso, el robot queda debajo)
3410 LET cam=INT (px+.5)-120: IF cam<0 THEN LET cam=0
3420 IF cam>1024 THEN LET cam=1024
3430 SCREEN s: CLIP 0,16,256,160: MAP 0,cam,0,0,16,256,160
3440 FOR k=1 TO 4
3450 IF a(k) THEN IF ABS (e(k)-cam-120)<150 THEN SPRITE 2+INT (n/4.8)-2*INT (n/9.6),e(k)-cam,f(k)+16,g(k)>0
3460 NEXT k
3470 SPRITE (vx<>0 AND suelo)*(INT (n/3.6)-2*INT (n/7.2)),px-cam,py+16,fl
3480 PRINT AT 0,1; PAPER 0; INK 6; "PUNTOS "; puntos; AT 0,23; "VIDAS "; vidas
3490 PAGE s: LET s=1-s: WAIT 2
3500 IF vol>0 THEN LET vol=vol-5/6: SOUND 8,vol*(vol>0)
3510 UNTIL muerto OR gana
3520 IF muerto THEN LET vidas=vidas-1: SOUND 8,0: PLAY "T200O4 3gfedc5c"
3530 END PROC
3600 REM --- Modo demostracion: avanza y salta muros, fosos, pinchos y bichos ---
3610 DEF PROC piloto
3620 LET de=1: LET sa=FN s(px+18,py+8) OR NOT FN s(px+20,py+20) OR FN t(px+24,py+8)=5
3630 FOR k=1 TO 4
3640 IF a(k) AND e(k)-px>0 AND e(k)-px<46 AND ABS (f(k)-py)<8 THEN LET sa=1
3650 NEXT k
3660 IF INKEY$<>"" THEN LET vidas=1: LET muerto=1
3670 END PROC
4000 REM --- El record, en la tarjeta (SD:, aunque se juegue desde ROM:) ---
4010 DEF PROC leer
4020 ON ERROR GO TO 4040
4030 OPEN #4,"SD:platformer.txt" IN: INPUT #4;hi
4040 ON ERROR GO TO 0: CLOSE #4
4050 END PROC
4100 DEF PROC grabar
4110 ON ERROR GO TO 4130
4120 OPEN #4,"SD:platformer.txt" OUT: PRINT #4;hi
4130 ON ERROR GO TO 0: CLOSE #4
4140 END PROC
5000 REM Los bichos: x, y
5010 @bichos: DATA 330,120,520,120,820,120,1000,120
