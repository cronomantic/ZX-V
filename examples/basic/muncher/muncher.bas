10 REM MUNCHER: come todos los puntos sin que te pillen los perseguidores
20 REM Un mapa de tiles para el laberinto, sprites encima y doble buffer.
30 REM Rufo te persigue, Lila te corta el paso, Niebla vigila su rincon
40 REM y Chispa va a su aire. Lo que se hace en cada frame va al principio.
50 GO TO @empezar
100 REM --- Una vida: el bucle del juego, a 60 imagenes por segundo ---
110 DEF PROC jugar
120 REPEAT
130 LET fr=fr+1
140 IF hielo THEN
150 LET hielo=hielo-1: IF hielo=0 THEN LET po=0
160 ELSE
170 PROC entrada
180 PROC jugador
190 PROC tiempos
200 FOR i=1 TO 4
210 LET sp=vf: IF z(i) THEN LET sp=vm
220 IF e(i)<>2 THEN LET sp=.5+1.5*(e(i)>3)
230 IF y(i)=88 THEN IF x(i)<40 OR x(i)>208 THEN LET sp=vt
240 LET a=f(i)+sp: LET ns=INT a: LET f(i)=a-ns
250 FOR j=1 TO ns: PROC paso: NEXT j
260 IF ABS (x(i)-px)<6 THEN IF ABS (y(i)-py)<6 THEN PROC choque
270 NEXT i
280 END IF
290 PROC dibujar
300 UNTIL muerto OR quedan=0 OR quit
310 END PROC
400 REM --- Un paso (un pixel) del perseguidor i ---
410 REM e(i): 0 en casa, 1 saliendo, 2 por el laberinto, 4 ojos, 5 entrando
420 DEF PROC paso
430 LET ee=e(i)
440 IF ee=2 OR ee=4 THEN
450 IF x(i)=8*INT (x(i)/8) THEN IF y(i)=8*INT (y(i)/8) THEN PROC decide
460 LET dd=d(i): LET x(i)=x(i)+u(dd): LET y(i)=y(i)+v(dd)
470 IF x(i)<-15 THEN LET x(i)=x(i)+272
480 IF x(i)>256 THEN LET x(i)=x(i)-272
490 ELSE IF ee=0 THEN
500 LET y(i)=y(i)+v(d(i)): IF y(i)<85 OR y(i)>91 THEN LET d(i)=o(d(i))
510 ELSE IF ee=1 THEN
520 IF x(i)<>124 THEN LET d(i)=1+(x(i)>124): LET x(i)=x(i)+u(d(i)) ELSE IF y(i)>64 THEN LET d(i)=4: LET y(i)=y(i)-1 ELSE LET e(i)=2: LET d(i)=2
530 ELSE
540 IF x(i)<>124 THEN LET x(i)=x(i)+1: LET d(i)=1 ELSE IF y(i)<88 THEN LET y(i)=y(i)+1: LET d(i)=3 ELSE LET e(i)=1
550 END IF
560 END PROC
600 REM --- En el centro de una casilla: por donde sigue el perseguidor i ---
610 DEF PROC decide
620 LET c=x(i)/8: LET r=y(i)/8
630 IF c<0 OR c>31 THEN END PROC
640 IF ee=4 THEN IF c=15 AND r=8 THEN LET e(i)=5: LET d(i)=1: END PROC
650 LET c=c+3: LET r=r+1: LET dd=o(d(i)): LET n=0
660 IF ee=2 AND (z(i) OR (i=4 AND RND<.35)) THEN
670 REM Asustado (o Chispa, a veces): un camino cualquiera, sin dar la vuelta
680 LET k=INT (RND*4)
690 FOR l=1 TO 4
700 LET ww=1+k-4*INT (k/4): LET k=k+1
710 IF n=0 THEN IF ww<>dd THEN IF m(c+u(ww),r+v(ww)) THEN LET n=ww
720 NEXT l
730 ELSE
740 REM Su blanco: casa, su rincon o el jugador, cada uno a su manera
750 IF ee=4 THEN
760 LET tx=18: LET ty=9
770 ELSE IF fa-2*INT (fa/2) THEN
780 LET tx=g(i): LET ty=h(i)
790 ELSE IF i=2 THEN
800 LET tx=pc+3+4*u(pd): LET ty=pr+1+4*v(pd)
810 ELSE IF i=3 AND (pc>17 OR pr<10) THEN
820 LET tx=g(3): LET ty=h(3)
830 ELSE
840 LET tx=pc+3: LET ty=pr+1
850 END IF
860 REM La casilla de al lado que queda mas cerca del blanco
870 LET b=1E9
880 FOR l=1 TO 4
890 LET ww=q(l)
900 IF ww<>dd THEN IF m(c+u(ww),r+v(ww)) THEN LET a=c+u(ww)-tx: LET bb=r+v(ww)-ty: LET a=a*a+bb*bb: IF a<b THEN LET b=a: LET n=ww
910 NEXT l
920 END IF
930 IF n=0 THEN LET n=dd
940 LET d(i)=n
950 END PROC
1000 REM --- El jugador toca al perseguidor i ---
1010 DEF PROC choque
1020 IF e(i)=0 OR e(i)>2 THEN END PROC
1030 IF z(i) THEN
1040 LET z(i)=0: LET e(i)=4+(e(i)=1): LET ce=ce+1: LET puntos=puntos+100*2^ce
1050 LET hielo=29: LET po=i: LET pn=49+ce: LET pw=24+8*(ce=4): LET fp=200: LET fd=-12: LET fv=15
1060 ELSE
1070 LET muerto=1
1080 END IF
1090 END PROC
1100 REM --- El jugador: avanza de pixel en pixel, y gira en los cruces ---
1110 DEF PROC jugador
1120 LET a=pa+vp: LET nj=INT a: LET pa=a-nj
1130 FOR j=1 TO nj
1140 IF nd=o(pd) THEN LET pd=nd: LET parado=0
1150 IF px=8*INT (px/8) THEN IF py=8*INT (py/8) THEN PROC casilla
1160 IF NOT parado THEN LET px=px+u(pd): LET py=py+v(pd): LET pm=pm+1: IF px<-15 THEN LET px=px+272
1170 IF px>256 THEN LET px=px-272
1180 NEXT j
1190 LET pc=INT ((px+4)/8): LET pr=INT ((py+4)/8)
1200 IF bt THEN IF py=128 THEN IF ABS (px-124)<6 THEN PROC fruta
1210 END PROC
1300 REM En el centro de una casilla: come lo que haya y gira si puede
1310 DEF PROC casilla
1320 LET c=px/8: LET r=py/8+1: LET t=CELL(0,c,r): IF t THEN PROC come
1330 IF demo THEN PROC piloto
1340 LET c=c+3
1350 IF m(c+u(nd),r+v(nd)) THEN LET pd=nd: LET parado=0 ELSE IF NOT m(c+u(pd),r+v(pd)) THEN LET parado=1
1360 END PROC
1400 DEF PROC come
1410 MAP POKE 0,c,r,0: LET quedan=quedan-1: LET lb=lb+1: LET comidos=comidos+1
1420 IF t=2 THEN LET puntos=puntos+50: PROC asusta ELSE LET puntos=puntos+10: LET wk=-wk: LET fp=120+30*wk: LET fd=-12*wk: LET fv=12
1430 IF comidos=70 OR comidos=170 THEN LET bt=480
1440 END PROC
1500 REM Una pildora: los perseguidores se asustan y dan la vuelta
1510 DEF PROC asusta
1520 LET am=tm: LET ce=0: LET fp=250: LET fd=-14: LET fv=15
1530 FOR l=1 TO 4
1540 IF e(l)<4 THEN LET z(l)=1: IF e(l)=2 THEN LET d(l)=o(d(l))
1550 NEXT l
1560 END PROC
1600 DEF PROC fruta
1610 LET puntos=puntos+bv: LET bt=0: LET bp=96: LET fp=40: LET fd=8: LET fv=15
1620 END PROC
1700 REM --- Los relojes: el susto, persecucion o retirada, las salidas de casa ---
1710 DEF PROC tiempos
1720 IF am THEN
1730 LET am=am-1: LET fl=0: IF am<96 THEN LET fl=INT (am/12)-2*INT (am/24)
1740 IF am=0 THEN FOR l=1 TO 4: LET z(l)=0: NEXT l
1750 ELSE
1760 LET ft=ft-1: IF ft=0 THEN LET fa=fa+1: LET ft=t(fa): PROC vuelta
1770 END IF
1780 IF nh<5 THEN IF lb>=n(nh) OR lt>=w(nh) THEN LET e(nh)=1: LET nh=nh+1
1790 LET lt=lt+1: IF bt THEN LET bt=bt-1
1800 IF bp THEN LET bp=bp-1
1810 IF puntos>=extra THEN LET extra=1E9: LET vidas=vidas+1: LET fp=30: LET fd=1: LET fv=15
1820 END PROC
1830 DEF PROC vuelta
1840 FOR l=1 TO 4
1850 IF e(l)=2 AND z(l)=0 THEN LET d(l)=o(d(l))
1860 NEXT l
1870 END PROC
1900 REM --- Dibujar: el laberinto, los perseguidores, el jugador y el marcador ---
1910 DEF PROC dibujar
1920 SCREEN s: MAP 0
1930 LET kk=fr-20*INT (fr/20): IF kk=0 THEN TILE 8,2,p$
1940 IF kk=10 THEN TILE 8,2,e$
1950 IF bt THEN SPRITE 39+fk,123,135
1960 IF bp THEN SPRITE bs,bx,136
1970 LET an=INT (fr/7.2)-2*INT (fr/14.4): LET oj=0
1980 IF sinfan=0 THEN
1990 FOR i=1 TO 4
2000 IF i=po THEN SPRITE pn,x(i)+4-pw/2,y(i)+8 ELSE IF e(i)>3 THEN SPRITE 30+d(i),x(i)-1,y(i)+7: LET oj=1 ELSE IF z(i) THEN SPRITE 36+an+2*fl,x(i)-1,y(i)+7 ELSE SPRITE 18+2*d(i)+an,x(i)-1,y(i)+7,0,k$(i)
2010 NEXT i
2020 END IF
2030 LET kk=INT (pm/2): LET kk=kk-4*INT (kk/4): IF kk=3 THEN LET kk=1
2040 IF mf THEN SPRITE 9+mf,px-2,py+6 ELSE IF po=0 THEN SPRITE 1+(kk>0)*(kk+2*(pd>2)),px-1,py+7,(pd=2)+2*(pd=3)
2050 IF puntos>hm THEN LET hm=puntos
2060 PRINT AT 0,0; INK 7; BRIGHT 1; PAPER 0; puntos; AT 0,10; INK 5+2*demo; h$; (STR$ hm AND NOT demo)
2070 FOR l=2 TO vidas: SPRITE 2,256-10*(l-1),0,0,"",8,8: NEXT l
2080 SPRITE 39+fk,204,0,0,"",8,8
2090 IF m$<>"" THEN PRINT AT 15,16-LEN m$/2; INK 6; BRIGHT 1; PAPER 0; m$
2100 PAGE s: LET s=1-s
2110 REM El sonido: el efecto del canal A y la sirena del canal B (segundo chip), al ritmo de antes (50 por segundo)
2120 IF fv>0 THEN LET fp=fp+fd*5/12: LET fv=fv-5/12: SOUND 16,fp;24,fv
2130 LET kk=fr-28.8*INT (fr/28.8): LET kk=ABS (kk-14.4)/2.4
2140 IF oj THEN LET sv=40+4*kk ELSE IF am THEN LET sv=240+30*INT ((fr-9.6*INT (fr/9.6))/2.4) ELSE LET sv=100+quedan/3+5*kk
2150 SOUND 18,sv-256*INT (sv/256);19,INT (sv/256)
2160 WAIT 1
2170 END PROC
2200 REM --- La entrada: cursores, OPQA o un mando; la ultima direccion se recuerda ---
2210 DEF PROC entrada
2220 IF demo THEN
2230 IF INKEY$<>"" OR JOY 0 OR fr>60*60 THEN LET quit=1
2240 ELSE
2250 LET j=JOY 0: IF j THEN LET j=j-16*INT (j/16): IF j THEN LET nd=1+(j>1)+(j>3)+(j>7)
2260 IF KEY 9 OR KEY 112 THEN LET nd=1
2270 IF KEY 8 OR KEY 111 THEN LET nd=2
2280 IF KEY 10 OR KEY 97 THEN LET nd=3
2290 IF KEY 11 OR KEY 113 THEN LET nd=4
2300 END IF
2310 END PROC
2400 REM --- La demostracion: en cada cruce, el piloto mira por cada camino ---
2410 DEF PROC piloto
2420 IF c<0 OR c>31 THEN END PROC
2430 LET cx=c+3: LET b=-1E9: LET n=pd
2440 REM El punto que queda mas adelante en la lista: hacia alli va si no ve ninguno
2450 REPEAT
2460 LET ww=INT (p(dp)/2): LET tx=ww-32*INT (ww/32)+3: LET ty=INT (ww/32)+1
2470 LET bb=CELL(0,tx-3,ty): IF bb=0 THEN LET dp=dp+1: IF dp>np THEN LET dp=1
2480 UNTIL bb OR quedan<1
2490 FOR l=1 TO 4
2500 IF m(cx+u(l),r+v(l)) THEN PROC mira: IF a>b THEN LET b=a: LET n=l
2510 NEXT l
2520 LET nd=n
2530 END PROC
2540 REM Cuanto vale el camino l: los puntos que se ven, y los perseguidores cerca
2550 DEF PROC mira
2560 LET a=RND*1.5-2*(l=o(pd))-ABS (cx+u(l)-tx)-ABS (r+v(l)-ty): LET xx=cx: LET yy=r: LET gg=8
2570 REPEAT
2580 LET xx=xx+u(l): LET yy=yy+v(l): IF CELL(0,xx-3,yy) THEN LET a=a+4*gg
2590 LET gg=gg-1
2600 UNTIL gg=1 OR xx<3 OR xx>34 OR NOT m(xx+u(l),yy+v(l))
2610 FOR h=1 TO 4
2620 IF e(h)=1 OR e(h)=2 THEN IF (x(h)-px)*u(l)+(y(h)-py)*v(l)>-4 THEN LET bb=ABS (x(h)-px)+ABS (y(h)-py): IF bb<64 THEN LET a=a-(64-bb)*(3-4*(z(h)>0 AND am>30))
2630 NEXT h
2640 END PROC
3000 REM --- Una partida: niveles hasta que no quede ninguna vida ---
3010 DEF PROC partida
3020 LET puntos=0: LET vidas=3: LET nivel=0: LET extra=10000: LET quit=0: LET fr=0: LET hm=hi
3030 LET h$=FN t$("RECORD ","HIGH "): IF demo THEN LET h$=d$
3040 SOUND 23,56;17,0;19,0;24,0;25,0
3050 REPEAT
3060 LET nivel=nivel+1: PROC nivel
3070 UNTIL vidas=0 OR quit
3080 SOUND 24,0;25,0
3090 IF NOT quit THEN
3100 LET m$=g$: LET sinfan=1: LET mf=10: PROC dibujar: LET m$="": LET sinfan=0: LET mf=0
3110 IF puntos>hi AND NOT demo THEN
3120 LET hi=puntos: PROC grabar
3130 SCREEN 1-s: PROC centro(17,14,FN t$(" NUEVO RECORD! "," NEW HIGH SCORE! "))
3140 END IF
3150 IF NOT demo THEN PLAY "T120O4 5g3fe5d3c$b7c","O3 5c3gg5e3ed7c"
3160 PAUSE 100
3170 END IF
3180 END PROC
3200 REM --- Un nivel: los puntos otra vez, y vidas hasta comerlos todos ---
3210 DEF PROC nivel
3220 FOR k=1 TO np: LET ww=INT (p(k)/2): MAP POKE 0,ww-32*INT (ww/32),INT (ww/32)+1,1+p(k)-2*ww: NEXT k
3230 LET quedan=np: LET comidos=0: LET dp=1
3240 LET fk=nivel-4*INT ((nivel-1)/4): LET k=nivel: IF k>6 THEN LET k=6
3250 LET bv=b(k): LET bs=53+k: LET bx=128-4*LEN STR$ bv
3260 REM Las velocidades, en pixeles por imagen: el jugador anda siempre uno (sin paradas ni saltos) y los demas,
3265 REM en proporcion a el (vp era 1+.02 por nivel). El susto, en imagenes: 300 (los pixeles que anda el jugador), 40 menos en cada nivel
3270 LET vp=1+.02*(nivel-1): IF vp>1.1 THEN LET vp=1.1
3280 LET vf=.8+.05*(nivel-1): IF vf>1.05 THEN LET vf=1.05
3290 LET vm=.5+.02*nivel: LET vt=.45: LET vf=vf/vp: LET vm=vm/vp: LET vt=vt/vp: LET vp=1
3300 LET tm=300-40*(nivel-1): IF tm<60 THEN LET tm=60
3310 LET primera=1
3320 REPEAT
3330 PROC salida: PROC jugar
3340 IF muerto THEN PROC muerte
3350 LET primera=0
3360 UNTIL quedan=0 OR vidas=0 OR quit
3370 IF quedan=0 THEN PROC superado
3380 END PROC
3400 REM --- Cada vida empieza con todos en su sitio y un momento para prepararse ---
3410 DEF PROC salida
3420 LET px=124: LET py=128: LET pd=2: LET nd=2: LET parado=0: LET pa=0: LET pm=0: LET mf=0
3430 RESTORE @casa: FOR i=1 TO 4: READ x(i),y(i),d(i),e(i): LET z(i)=0: LET f(i)=0: NEXT i
3440 LET am=0: LET fa=1: LET ft=t(1): LET hielo=0: LET po=0: LET muerto=0: LET lt=0: LET lb=0: LET nh=2
3450 LET bt=0: LET bp=0: LET sinfan=0: LET fl=0: LET fv=0: LET pc=15: LET pr=16
3460 REM Cuando sale cada uno: tras tantos puntos comidos o tantas imagenes
3470 IF primera THEN
3480 LET n(3)=20*(nivel=1)+8*(nivel=2): LET n(4)=50*(nivel=1)+25*(nivel=2): LET w(3)=500: LET w(4)=1000
3490 ELSE
3500 LET n(3)=7: LET n(4)=17: LET w(3)=200: LET w(4)=400
3510 END IF
3520 SOUND 24,0;25,0
3530 IF primera THEN LET m$=FN t$("NIVEL ","LEVEL ")+STR$ nivel: FOR k=1 TO 48: PROC dibujar: NEXT k
3540 LET m$=r$: PROC dibujar
3550 IF primera AND NOT demo THEN PLAY "T190O5 3cegCgece egbEDbgb 5Cg7C","O3 5cgcg ebeb cg7c" ELSE FOR k=1 TO 72: PROC dibujar: NEXT k
3560 LET m$="": SOUND 25,9
3570 END PROC
3600 REM --- Te han pillado: todo se para y el jugador desaparece ---
3610 DEF PROC muerte
3620 SOUND 25,0;24,0: LET fv=0: LET po=0
3630 FOR k=1 TO 48: PROC dibujar: NEXT k
3640 LET sinfan=1: LET bt=0: LET bp=0
3650 FOR k=1 TO 10
3660 LET mf=k: SOUND 16,40+k*18;24,13: PROC dibujar: PROC dibujar: IF k<9 THEN PROC dibujar
3665 SOUND 16,50+k*19;24,10: PROC dibujar: PROC dibujar
3670 NEXT k
3680 SOUND 24,0: FOR k=1 TO 36: PROC dibujar: NEXT k
3690 LET vidas=vidas-1: LET mf=0: LET sinfan=0
3700 END PROC
3800 REM --- Nivel superado: las paredes parpadean (REMAP) ---
3810 DEF PROC superado
3820 SOUND 24,0;25,0: LET fv=0
3830 FOR k=1 TO 48: PROC dibujar: NEXT k
3840 LET sinfan=1: LET bt=0: LET bp=0
3850 FOR k=1 TO 8: REMAP 9,9+6*(k-2*INT (k/2)): FOR j=1 TO 7+(k>6): PROC dibujar: NEXT j: NEXT k
3860 REMAP: LET sinfan=0
3870 END PROC
4000 REM --- El titulo: el nombre, los perseguidores, las teclas y el record ---
4010 DEF PROC titulo
4020 LET demo=0: LET t=0: SOUND 24,0;25,0
4030 FOR s=0 TO 1: SCREEN s: CLS: PROC portada: NEXT s
4040 LET s=1
4050 REPEAT
4060 SCREEN s: BOX 0,0,160,256,32
4070 IF INT (t/30)-2*INT (t/60) THEN PROC centro(20,15,FN t$("PULSA ESPACIO O EL BOTON","PRESS SPACE OR FIRE"))
4080 REM Abajo pasa el jugador perseguido por los cuatro
4090 LET k=272-(t-408*INT (t/408)): LET a=INT (t/3.6): LET a=a-4*INT (a/4): IF a=3 THEN LET a=1
4100 SPRITE 1+(a>0)*a,k,176,1
4110 FOR l=1 TO 4: SPRITE 22+INT (t/7.2)-2*INT (t/14.4),k+12+14*l,176,0,k$(l): NEXT l
4120 PAGE s: LET s=1-s: WAIT 1: LET t=t+1
4130 UNTIL KEY 32 OR KEY 13 OR JOY 0>=16 OR t>10*60
4140 LET demo=t>10*60
4150 END PROC
4200 DEF PROC portada
4210 SPRITE 60,46,10,0,"..............1",168,24: SPRITE 60,44,8,0,"",168,24
4220 PROC centro(5,15,FN t$("COME TODOS LOS PUNTOS","EAT ALL THE DOTS"))
4230 RESTORE @nombres
4240 FOR l=1 TO 4
4250 READ a$,a,b$,c$
4260 SPRITE 20,37,8*(6+l)-1,0,k$(l)
4270 PRINT AT 6+l,7; INK a; BRIGHT 1; PAPER 0; a$; AT 6+l,14; INK 7; BRIGHT 0; FN t$(b$,c$)
4280 NEXT l
4290 SPRITE 61,37,96
4300 PRINT AT 12,7; INK 7; BRIGHT 1; PAPER 0; FN t$("PILDORA: AHORA HUYEN","POWER PILL: THEY FLEE")
4310 PROC centro(14,13,FN t$("CURSORES O OPQA: MOVERTE","ARROWS OR OPQA: MOVE"))
4320 PROC centro(15,13,FN t$("MANDO: LA CRUCETA","GAMEPAD: THE D-PAD"))
4330 PROC centro(16,13,FN t$("ESC: SALIR","ESC: QUIT"))
4340 PROC centro(18,14,FN t$("RECORD ","HIGH SCORE ")+STR$ hi)
4350 END PROC
4400 REM Un texto centrado en la fila r, del color c (de 0 a 15: del 8 en adelante, con BRIGHT)
4410 DEF PROC centro(r,c,a$)
4420 PRINT AT r,INT ((32-LEN a$)/2); INK c-8*(c>7); BRIGHT (c>7); PAPER 0; a$
4430 END PROC
5000 REM --- Empezar: el idioma, las tablas, el laberinto, los graficos y el record ---
5010 @empezar: BORDER 0: PAPER 0: INK 7: MODE 0: CLS: CLEAR
5020 REM Las variables se buscan en el orden en que se crearon: primero, las de cada frame
5030 DIM m(36,23): DIM x(4): DIM y(4): DIM d(4): DIM e(4): DIM f(4): DIM z(4)
5040 DIM u(4): DIM v(4): DIM o(4): DIM q(4): DIM g(4): DIM h(4): DIM n(4): DIM w(4): DIM t(8): DIM b(6): DIM p(300)
5050 LET fr=0: LET i=0: LET j=0: LET k=0: LET l=0: LET a=0: LET b=0: LET c=0: LET r=0: LET s=0
5060 LET px=0: LET py=0: LET pd=2: LET nd=2: LET pa=0: LET pm=0: LET pc=0: LET pr=0: LET parado=0
5070 LET hielo=0: LET po=0: LET sp=0: LET ns=0: LET nj=0: LET ee=0: LET dd=0: LET ww=0: LET bb=0: LET tx=0: LET ty=0
5080 LET am=0: LET fa=1: LET ft=0: LET fl=0: LET lt=0: LET lb=0: LET nh=5: LET bt=0: LET bp=0: LET quedan=1: LET comidos=0
5090 LET fp=0: LET fd=0: LET fv=0: LET wk=1: LET sv=0: LET oj=0: LET an=0: LET mf=0: LET sinfan=0: LET muerto=0: LET quit=0
5100 LET puntos=0: LET vidas=0: LET demo=0: LET hm=0: LET pn=0: LET pw=0: LET ce=0
5110 LET en=PEEK 536872736<>101 OR PEEK 536872737<>115: REM no es "es": en ingles
5120 DEF FN t$(a$,b$)=(a$ AND NOT en)+(b$ AND en)
5130 REM m: el laberinto (1 se puede pasar), de la columna -2 a la 33 y de la fila 0 a la 22
5140 REM x, y, d, e, f, z: cada perseguidor (posicion, direccion, estado, paso, asustado)
5150 REM u, v: las direcciones (1 derecha, 2 izquierda, 3 abajo, 4 arriba); o: la contraria
5160 REM q: el orden en que se prueban; g, h: el rincon de cada uno; n, w: cuando sale
5170 REM t: lo que dura cada fase (retirada, persecucion...); b: lo que vale la fruta
5180 REM p: los puntos del laberinto (fila*32+columna, por dos, mas 1 si es pildora)
5190 RESTORE @tablas
5200 FOR k=1 TO 4: READ u(k): NEXT k: FOR k=1 TO 4: READ v(k): NEXT k
5210 FOR k=1 TO 4: READ o(k): NEXT k: FOR k=1 TO 4: READ q(k): NEXT k
5220 FOR k=1 TO 4: READ g(k): NEXT k: FOR k=1 TO 4: READ h(k): NEXT k
5230 FOR k=1 TO 8: READ t(k): NEXT k: FOR k=1 TO 6: READ b(k): NEXT k
5240 LET n(2)=0: LET w(2)=20
5250 DIM k$(4,3): LET k$(1)="..a": LET k$(2)="..b": LET k$(3)="..d": LET k$(4)="..c"
5260 LET r$=FN t$("LISTO!","READY!"): LET g$=FN t$("FIN DE PARTIDA","GAME  OVER"): LET d$=FN t$("DEMOSTRACION","DEMO"): LET m$=""
5270 REM El laberinto: cada fila es la mitad izquierda; la derecha es su reflejo
5280 DIM l$(25,34): LET a$="##################################"
5290 FOR r=1 TO 25: LET l$(r)=a$: NEXT r
5300 RESTORE @laberinto
5310 FOR r=0 TO 22: READ a$: LET b$=a$: FOR k=16 TO 1 STEP -1: LET b$=b$+a$(k): NEXT k: LET l$(r+2)="#"+b$+"#": NEXT r
5320 LET hi=0: PROC leer: PROC graficos: PROC mapa
5330 REPEAT
5340 PROC titulo
5350 PROC partida
5360 UNTIL 0
5400 REM --- El mapa de tiles: las paredes, la puerta y la lista de los puntos ---
5410 DEF PROC mapa
5420 MAP DIM 0,32,24: LET np=0
5430 FOR r=0 TO 22: FOR c=0 TO 31
5440 LET c$=l$(r+2,c+2)
5450 IF c$="#" OR c$="=" THEN MAP POKE 0,c,r+1,100+r*32+c
5460 IF c$="." OR c$="o" THEN LET np=np+1: LET p(np)=(r*32+c)*2+(c$="o")
5470 IF c$="." OR c$="o" OR c$=" " THEN LET m(c+3,r+1)=1
5480 NEXT c: NEXT r
5490 LET m(1,12)=1: LET m(2,12)=1: LET m(35,12)=1: LET m(36,12)=1: REM el tunel sigue fuera
5500 END PROC
5600 REM --- Los graficos: se dibujan en la pagina 1 y se copian con TILE y GRAB ---
5610 DEF PROC graficos
5620 SCREEN 1: CLS
5630 REM Las paredes: una linea dentro de cada casilla de pared, junto a los pasillos
5640 FOR r=0 TO 22: FOR c=0 TO 31: IF l$(r+2,c+2)="#" THEN PROC pared
5650 NEXT c: NEXT r
5660 BOX 3,118,83,20,2: REM la puerta de casa
5670 TILE 8,100,0,8,256,184: REM un tile por casilla: el 100+fila*32+columna
5680 REM Los tiles 0 (vacio), 1 (punto) y 2 (pildora)
5690 LET e$="": FOR k=1 TO 64: LET e$=e$+"0": NEXT k
5700 LET q$=e$( TO 24)+"0007700000077000"+e$( TO 24)
5710 LET p$="0000000000ffff000ffffff00ffffff00ffffff00ffffff000ffff0000000000"
5720 TILE 8,0,e$: TILE 8,1,q$: TILE 8,2,p$
5730 REM Los sprites de 10x10: el jugador (1-5), los perseguidores y las frutas
5740 RESTORE @sprites
5750 PROC lee: GRAB 1,10,a$
5760 PROC lee: GRAB 2,10,a$: PROC gira: GRAB 4,10,b$
5770 PROC lee: GRAB 3,10,a$: PROC gira: GRAB 5,10,b$
5780 PROC lee: LET i$=a$: PROC lee: LET j$=a$
5790 REM Con los ojos hacia cada lado: 20-27 el cuerpo y 31-34 los ojos solos
5800 FOR k=1 TO 4
5810 PROC lee: GRAB 30+k,10,a$
5820 LET b$=i$: PROC pon: GRAB 18+2*k,10,b$: LET b$=j$: PROC pon: GRAB 19+2*k,10,b$
5830 NEXT k
5840 REM Asustados (36, 37) y parpadeando (38, 39)
5850 PROC lee: GRAB 36,10,a$: LET b$=a$: LET b$(91 TO 100)="1..1111..1": GRAB 37,10,b$
5860 PROC blanco: GRAB 39,10,b$: LET b$=a$: PROC blanco: GRAB 38,10,b$
5870 REM Las frutas (40-43)
5880 FOR k=1 TO 4: PROC lee: GRAB 39+k,10,a$: NEXT k
5890 REM El jugador desaparece (10-18), y el 19 no tiene nada
5900 CLS: DIM j(14): REM la boca se abre: el hueco es un poligono
5910 FOR k=1 TO 8
5920 BOX 0,0,0,12,12: DISC 14,5,5,5
5930 FOR l=0 TO 6: LET a=PI/2-k*PI/9+l*k*PI/27: LET j(2*l+1)=5+9*COS a: LET j(2*l+2)=5-9*SIN a: NEXT l
5940 POLY 0,5,5,j(1),j(2),j(3),j(4),j(5),j(6),j(7),j(8),j(9),j(10),j(11),j(12),j(13),j(14)
5950 GRAB 9+k,0,0,11,11,0
5960 NEXT k
5970 BOX 0,0,0,12,12
5980 POLY 14,5,0,5,2: POLY 14,5,8,5,10: POLY 14,0,5,2,5: POLY 14,8,5,10,5
5990 POLY 14,1,1,2,2: POLY 14,9,1,8,2: POLY 14,1,9,2,8: POLY 14,9,9,8,8
6000 GRAB 18,0,0,11,11,0: GRAB 19,1,"."
6010 REM Los puntos que se ganan (50-53 por los perseguidores, 54-59 por las frutas)
6030 FOR k=1 TO 4: PRINT AT 0,0; INK 5; BRIGHT 1; PAPER 0; 100*2^k: GRAB 49+k,0,0,8*LEN STR$ (100*2^k),8,0: NEXT k
6040 FOR k=1 TO 6: PRINT AT 0,0; INK 3; BRIGHT 1; PAPER 0; b(k): GRAB 53+k,0,0,8*LEN STR$ b(k),8,0: NEXT k
6050 REM El nombre (60) para el titulo, y una pildora (61)
6060 PRINT AT 21,0; INK 6; BRIGHT 1; PAPER 0; "MUNCHER"
6070 GRAB 60,0,168,56,8,0: GRAB 61,8,p$
6090 SCREEN 0: CLS
6100 END PROC
6200 REM Una casilla de pared en la columna c y la fila r: a, b, k, n dicen si
6210 REM hay paso arriba, abajo, a la izquierda y a la derecha
6220 DEF PROC pared
6230 LET xx=c*8: LET yy=r*8+8
6240 LET a=l$(r+1,c+2)<>"#": LET b=l$(r+3,c+2)<>"#": LET k=l$(r+2,c+1)<>"#": LET n=l$(r+2,c+3)<>"#"
6250 IF a THEN BOX 9,xx+3*k,yy+2,8-3*k-3*n,1
6260 IF b THEN BOX 9,xx+3*k,yy+5,8-3*k-3*n,1
6270 IF k THEN BOX 9,xx+2,yy+3*a,1,8-3*a-3*b
6280 IF n THEN BOX 9,xx+5,yy+3*a,1,8-3*a-3*b
6290 REM Las esquinas de dentro: paso solo en la diagonal
6300 IF NOT (a OR k) THEN IF l$(r+1,c+1)<>"#" THEN BOX 9,xx,yy+2,2,1: BOX 9,xx+2,yy,1,2
6310 IF NOT (a OR n) THEN IF l$(r+1,c+3)<>"#" THEN BOX 9,xx+6,yy+2,2,1: BOX 9,xx+5,yy,1,2
6320 IF NOT (b OR k) THEN IF l$(r+3,c+1)<>"#" THEN BOX 9,xx,yy+5,2,1: BOX 9,xx+2,yy+6,1,2
6330 IF NOT (b OR n) THEN IF l$(r+3,c+3)<>"#" THEN BOX 9,xx+6,yy+5,2,1: BOX 9,xx+5,yy+6,1,2
6340 END PROC
6400 REM Un sprite de los DATA: diez filas de diez pixeles
6410 DEF PROC lee
6420 LET a$="": FOR l=1 TO 10: READ b$: LET a$=a$+b$: NEXT l
6430 END PROC
6440 REM b$: a$ girado un cuarto de vuelta (la boca, de la derecha a arriba)
6450 DEF PROC gira
6460 LET b$="": FOR r=0 TO 9: FOR c=0 TO 9: LET b$=b$+a$(c*10+10-r): NEXT c: NEXT r
6470 END PROC
6480 REM Los ojos (a$) encima del cuerpo (b$)
6490 DEF PROC pon
6500 FOR l=1 TO 100: IF a$(l)<>"." THEN LET b$(l)=a$(l)
6510 NEXT l
6520 END PROC
6530 REM El susto se acaba: el cuerpo blanco y la cara roja
6540 DEF PROC blanco
6550 FOR l=1 TO 100: IF b$(l)="1" THEN LET b$(l)="f" ELSE IF b$(l)="e" THEN LET b$(l)="a"
6560 NEXT l
6570 END PROC
7000 REM --- El record, en la tarjeta (SD:, aunque se juegue desde ROM:) ---
7010 DEF PROC leer
7020 ON ERROR GO TO 7040
7030 OPEN #4,"SD:muncher.txt" IN: INPUT #4;hi
7040 ON ERROR GO TO 0: CLOSE #4
7050 END PROC
7100 DEF PROC grabar
7110 ON ERROR GO TO 7130
7120 OPEN #4,"SD:muncher.txt" OUT: PRINT #4;hi
7130 ON ERROR GO TO 0: CLOSE #4
7140 END PROC
9000 REM Las direcciones (u, v), la contraria (o) y el orden en que se prueban (q)
9010 @tablas: DATA 1,-1,0,0,0,0,1,-1,2,1,4,3,4,2,3,1
9020 REM Los rincones (g, h): Rufo arriba a la derecha, Lila arriba a la izquierda...
9030 DATA 32,5,3,34,-1,-1,25,25
9040 REM Retirada y persecucion, en imagenes (unos 6, 17, 6, 17, 4, 17 y 4 segundos), y despues siempre
9050 DATA 350,1000,350,1000,250,1000,250,1E9
9060 REM Lo que vale la fruta en cada nivel
9070 DATA 100,300,500,1000,2000,3000
9080 REM Donde empieza cada perseguidor: x, y, direccion y estado
9090 @casa: DATA 124,64,2,2,124,88,4,0,108,88,3,0,140,88,3,0
9100 REM Sus nombres, su color y lo que hacen
9110 @nombres: DATA "RUFO",2,"TE PERSIGUE","CHASES YOU"
9120 DATA "LILA",3,"TE CORTA EL PASO","CUTS YOU OFF"
9130 DATA "NIEBLA",5,"VIGILA SU RINCON","GUARDS HER CORNER"
9140 DATA "CHISPA",4,"VA A SU AIRE","WANDERS ABOUT"
9200 REM El laberinto: # pared, . punto, o pildora, = puerta, h casa, espacio pasillo
9210 @laberinto: DATA "################","#..............#","#.###.######.#.#","#o###.######.#.#"
9220 DATA "#...............","#.###.##.#######","#.....##......##","#####.###.###.##"
9230 DATA "#####.###       ","#####.### #####=","#####.### ##hhhh","     .    ##hhhh"
9240 DATA "#####.### ##hhhh","#####.### ######","#####.###       ","#####.#####.####"
9250 DATA "#o..............","#.###.#.#####.##","#.............##","#.#########.####"
9260 DATA "#.#########.####","#...............","################"
9300 REM Los sprites, de 10x10 (cifras hexadecimales: colores; punto: transparente)
9310 REM El jugador: cerrado, con la boca medio abierta y abierta del todo
9320 @sprites: DATA "...eeee...",".eeeeeeee.",".eeeeeeee.","eeeeeeeeee","eeeeeeeeee","eeeeeeeeee","eeeeeeeeee",".eeeeeeee.",".eeeeeeee.","...eeee..."
9330 DATA "...eeee...",".eeeeeeee.",".eeeeeee..","eeeeeee...","eeeee.....","eeeee.....","eeeeeee...",".eeeeeee..",".eeeeeeee.","...eeee..."
9340 DATA "...eeee...",".eeeeee...",".eeeee....","eeeee.....","eeee......","eeee......","eeeee.....",".eeeee....",".eeeeee...","...eeee..."
9350 REM El cuerpo de un perseguidor (rojo: SPRITE le pone su color), en dos pasos
9360 DATA "...2222...",".22222222.","2222222222","2222222222","2222222222","2222222222","2222222222","2222222222","2222222222","22..22..22"
9370 DATA "...2222...",".22222222.","2222222222","2222222222","2222222222","2222222222","2222222222","2222222222","2222222222","2..2222..2"
9380 REM Los ojos: mirando a la derecha, a la izquierda, abajo y arriba
9390 DATA "..........","..........","..fff..fff","..f11..f11","..f11..f11","..fff..fff","..........","..........","..........",".........."
9400 DATA "..........","..........","fff..fff..","11f..11f..","11f..11f..","fff..fff..","..........","..........","..........",".........."
9410 DATA "..........","..........","..........",".fff..fff.",".fff..fff.",".f11..11f.",".f11..11f.","..........","..........",".........."
9420 DATA "..........",".f11..11f.",".f11..11f.",".fff..fff.",".fff..fff.","..........","..........","..........","..........",".........."
9430 REM Asustado
9440 DATA "...1111...",".11111111.","1111111111","11ee11ee11","11ee11ee11","1111111111","1e11ee11e1","11ee11ee11","1111111111","11..11..11"
9450 REM Las frutas: cereza, limon, uvas y seta
9460 DATA ".......4..","......44..",".....4.4..","....4...4.","...4....4.",".aa4...aa.","afaaa.afaa","aaaaa.aaaa","aaaaa.aaaa",".aaa...aa."
9470 DATA ".....44...","....44....","..eeeeee..",".eeeeeeee.","eefeeeeeee","efeeeeeee6","eeeeeeeee6",".eeeeeee6.","..e6666e..",".........."
9480 DATA "....44....",".....4....","..bb.bb...",".bfbbfbb..","..bb.bb.b.",".bb.bbbbb.",".bfbbfbb..","..bb.bb...","...bbbb...","....bb...."
9490 DATA "...aaaa...",".aafaaaaa.","aafffaafaa","aaafaaafff","aaaaaaaafa",".aaaaaaaa.","...ffff...","...ffff...","...ffff...","...ffff..."
