REM Petit Soccer: fútbol 7 para ZX-V, en ZX Basic con zxv.bas.
REM
REM El campo (512 x 384) es más grande que la pantalla y la cámara sigue al
REM balón con SCROLL; todo se dibuja en la página oculta y luego se enseña
REM (doble buffer), a 60 imágenes por segundo. Las posiciones van en 1/256 de
REM píxel (F) para que todo se mueva suave. Los sprites salen de sprites.png
REM (mksprites.py) y la música y los efectos, de mksound.py.
REM
REM   zxbc --arch zxv petitsoccer.bas

#include <zxv.bas>
#include "sprites.bas"

CONST F AS LONG = 256                       ' 1 píxel
CONST HZ AS LONG = 60
CONST FX0 AS LONG = 32                      ' las líneas del campo
CONST FX1 AS LONG = 480
CONST FY0 AS LONG = 32
CONST FY1 AS LONG = 352
CONST MX AS LONG = 256                      ' el centro
CONST MY AS LONG = 192
CONST GH AS LONG = 36                       ' media portería
CONST BAR AS LONG = 22                      ' el alto del larguero
CONST NET AS LONG = 16                      ' el fondo de la red
CONST AREAW AS LONG = 72                    ' el área: fondo y media altura
CONST AREAH AS LONG = 92
CONST GRAV AS LONG = 30
CONST TXT AS LONG = 100                     ' el sprite del carácter 32

REM Estados del partido
CONST GPLAY AS UBYTE = 0
CONST GDEAD AS UBYTE = 1                    ' saque: de centro, banda, córner o puerta
CONST GGOAL AS UBYTE = 2
CONST GHALF AS UBYTE = 3
CONST GEND AS UBYTE = 4
REM Estados de un jugador
CONST SNORMAL AS UBYTE = 0
CONST SSLIDE AS UBYTE = 1
CONST SDOWN AS UBYTE = 2
CONST SDIVE AS UBYTE = 3
CONST SHOLD AS UBYTE = 4                    ' el portero con el balón en las manos
CONST SSET AS UBYTE = 5                     ' va a sacar

REM Los 14 jugadores: 0-6 un equipo y 7-13 el otro; el 0 de cada uno, el portero
DIM px(13) AS LONG
DIM py(13) AS LONG
DIM pd(13) AS UBYTE                         ' hacia dónde mira (0 E, 1 SE, 2 S ... 7 NE)
DIM pa(13) AS UBYTE                         ' el paso de la carrera
DIM pmv(13) AS UBYTE                        ' se ha movido en esta imagen
DIM pst(13) AS UBYTE
DIM ptm(13) AS UBYTE                        ' lo que le queda al estado
DIM pcd(13) AS UBYTE                        ' no puede coger el balón todavía
DIM pth(13) AS UBYTE                        ' imágenes que lleva con el balón
DIM pvy(13) AS LONG                         ' la velocidad de la estirada del portero
DIM pc$(13) AS STRING                       ' sus colores: equipación, pelo y piel
DIM ord(13) AS UBYTE                        ' el orden de dibujo, de arriba abajo

DIM ddx(7) AS LONG => {256, 181, 0, -181, -256, -181, 0, 181}
DIM ddy(7) AS LONG => {0, 181, 256, 181, 0, -181, -256, -181}
DIM sbase(7) AS UBYTE => {6, 9, 0, 9, 6, 12, 3, 12}     ' el sprite de cada dirección
DIM sflip(7) AS UBYTE => {0, 0, 0, 1, 1, 1, 0, 0}
DIM srun(3) AS UBYTE => {1, 0, 2, 0}
REM Dónde espera cada uno, del 0 (su línea de fondo) al 448, y de arriba abajo
DIM homex(6) AS LONG => {10, 90, 90, 190, 170, 190, 290}
DIM homey(6) AS LONG => {160, 92, 228, 52, 160, 268, 160}
DIM ccx(24) AS LONG                         ' el círculo central
DIM ccy(24) AS LONG

REM El balón
DIM bx, by, bz, bvx, bvy, bvz AS LONG
DIM bxp, byp, bzp AS LONG                   ' en píxeles
DIM tbx, tby AS LONG                        ' dónde va a estar enseguida
DIM owner, recv, lastT AS LONG              ' quién lo lleva, a quién va, qué equipo lo tocó
DIM spin AS LONG

REM Los equipos
DIM human(1) AS UBYTE                       ' 0 la máquina, 1 y 2 los jugadores
DIM side(1) AS LONG                         ' 1 ataca hacia la derecha, -1 hacia la izquierda
DIM goals(1) AS LONG
DIM ctl(1) AS LONG                          ' el jugador que se maneja
DIM ctlsw(1) AS UBYTE
DIM chase(1) AS LONG                        ' el que va al balón y el que le cubre
DIM cover(1) AS LONG
DIM fprev(1) AS UBYTE                       ' el botón, en la imagen anterior
DIM fhold(1) AS UBYTE                       ' cuánto lleva pulsado
DIM fon(1) AS UBYTE
DIM team(1) AS UBYTE                        ' qué equipo de la lista es

REM El partido
DIM gs, deadkind, half, lvl AS UBYTE
DIM gst, taker, clock, halfFrames, minutes AS LONG
DIM camx, camy AS LONG
DIM pg AS UBYTE
DIM seed AS ULONG
DIM msg$ AS STRING
DIM msgt AS LONG
DIM ix, iy AS LONG
DIM ifire AS UBYTE
DIM nd AS LONG                              ' la distancia que dio nearOpp
DIM demo AS UBYTE
DIM es AS UBYTE                             ' la máquina está en español
DIM hassfx AS UBYTE                         ' hay efectos de sonido
DIM cw$, cy$, cc$ AS STRING                 ' colores para los textos

REM La lista de equipos: nombre, siglas y colores, de los 16 de siempre
CONST NTEAMS AS LONG = 8
DIM tname$(7) AS STRING
DIM tabr$(7) AS STRING
DIM tsh(7) AS UBYTE                         ' el color de la camiseta
DIM tpa(7) AS UBYTE                         ' y el del pantalón

#ifdef DEBUGLOG
REM Para probarlo: lo que pasa en el partido, a un fichero (pslog.bin) al acabar
DIM elog(1999) AS LONG
DIM nlog AS LONG
SUB ev(e AS LONG, a AS LONG)
    IF nlog < 1996 THEN
        elog(nlog) = TIME: elog(nlog + 1) = e: elog(nlog + 2) = a: elog(nlog + 3) = bxp * 1000 + byp
        nlog = nlog + 4
    END IF
END SUB
#else
#define ev(e, a)
#endif

REM --- Utilidades ------------------------------------------------------------

FUNCTION rn(n AS LONG) AS LONG
    seed = seed * 1103515245 + 12345
    RETURN CAST(LONG, (seed >> 16) BAND 32767) MOD n
END FUNCTION

FUNCTION isqrt(v AS LONG) AS LONG
    DIM r, b AS LONG
    IF v <= 0 THEN RETURN 0
    r = v: b = (v + 1) / 2
    WHILE b < r
        r = b: b = (r + v / r) / 2
    END WHILE
    RETURN r
END FUNCTION

REM La distancia, más o menos (para comparar): el lado largo y 3/8 del corto
FUNCTION dst(dx AS LONG, dy AS LONG) AS LONG
    IF dx < 0 THEN dx = -dx
    IF dy < 0 THEN dy = -dy
    IF dx > dy THEN RETURN dx + dy * 3 / 8
    RETURN dy + dx * 3 / 8
END FUNCTION

REM La dirección (de las ocho) que más se parece a (dx, dy)
FUNCTION dir8(dx AS LONG, dy AS LONG) AS UBYTE
    DIM ax, ay AS LONG
    ax = dx: IF ax < 0 THEN ax = -ax
    ay = dy: IF ay < 0 THEN ay = -ay
    IF ay * 12 < ax * 5 THEN
        IF dx > 0 THEN RETURN 0
        RETURN 4
    END IF
    IF ax * 12 < ay * 5 THEN
        IF dy > 0 THEN RETURN 2
        RETURN 6
    END IF
    IF dx > 0 THEN
        IF dy > 0 THEN RETURN 1
        RETURN 7
    END IF
    IF dy > 0 THEN RETURN 3
    RETURN 5
END FUNCTION

FUNCTION tr$(en$ AS STRING, sp$ AS STRING) AS STRING
    IF es THEN RETURN sp$
    RETURN en$
END FUNCTION

REM Un texto con los sprites de la fuente, en cualquier sitio de la página
SUB txt(x AS LONG, y AS LONG, s$ AS STRING, c$ AS STRING)
    DIM k, ch AS LONG
    FOR k = 0 TO LEN(s$) - 1
        ch = CODE(s$(k))
        IF ch > 32 AND ch < 96 THEN SPRITE TXT + ch - 32, x + k * 8, y, 0, c$
    NEXT k
END SUB

SUB bigtxt(x AS LONG, y AS LONG, s$ AS STRING, c$ AS STRING, z AS LONG)
    DIM k, ch AS LONG
    FOR k = 0 TO LEN(s$) - 1
        ch = CODE(s$(k))
        IF ch > 32 AND ch < 96 THEN SPRITE TXT + ch - 32, x + k * 8 * z, y, 0, c$, 8 * z, 8 * z
    NEXT k
END SUB

FUNCTION two$(n AS LONG) AS STRING
    IF n < 10 THEN RETURN "0" + STR$(n)
    RETURN STR$(n)
END FUNCTION

REM Un efecto del banco: 0 chut, 1 pase, 2 silbato, 3 gol, 4 bote, 5 el final
SUB snd(n AS UBYTE)
    IF hassfx THEN SFX n
END SUB

REM --- El balón --------------------------------------------------------------

REM Lo chuta hacia (tx, ty) con esa velocidad y esa subida
SUB kickto(tx AS LONG, ty AS LONG, spd AS LONG, vz AS LONG)
    DIM dx, dy, d AS LONG
    dx = tx - bx / F: dy = ty - by / F
    d = isqrt(dx * dx + dy * dy)
    IF d < 1 THEN d = 1
    bvx = dx * spd / d: bvy = dy * spd / d: bvz = vz
    IF owner >= 0 THEN
        pcd(owner) = 16: lastT = owner / 7: pth(owner) = 0
        IF pst(owner) = SHOLD OR pst(owner) = SSET THEN pst(owner) = SNORMAL
    END IF
    owner = -1
END SUB

REM Un pase raso: con la fuerza justa para que llegue
SUB passto(tx AS LONG, ty AS LONG)
    DIM d, spd AS LONG
    d = dst(tx - bx / F, ty - by / F)
    spd = d * 8 + 330
    IF spd < 560 THEN spd = 560
    IF spd > 1450 THEN spd = 1450
    kickto(tx, ty, spd, 0)
END SUB

REM Por alto: con la velocidad justa para que caiga allí
SUB lobto(tx AS LONG, ty AS LONG, vz AS LONG)
    DIM dx, dy, d, spd AS LONG
    dx = tx - bx / F: dy = ty - by / F
    d = isqrt(dx * dx + dy * dy)
    spd = d * F * GRAV / (2 * vz)
    IF spd > 1800 THEN spd = 1800
    kickto(tx, ty, spd, vz)
END SUB

REM --- Los jugadores ---------------------------------------------------------

REM El rival más cercano a i; deja la distancia en nd
FUNCTION nearOpp(i AS LONG) AS LONG
    DIM j, k, d, best AS LONG
    best = -1: nd = 99999
    k = 7: IF i >= 7 THEN k = 0
    FOR j = k TO k + 6
        d = dst((px(j) - px(i)) / F, (py(j) - py(i)) / F)
        IF d < nd THEN nd = d: best = j
    NEXT j
    RETURN best
END FUNCTION

REM Dónde le toca estar a i según por dónde va el balón (atk: su equipo ataca)
SUB homepos(i AS LONG, atk AS LONG)
    DIM r, t, u, hx, hy AS LONG
    r = i MOD 7: t = i / 7
    IF side(t) > 0 THEN u = bxp - FX0 ELSE u = FX1 - bxp
    hx = homex(r) + (u - 224) * 5 / 8
    IF r < 3 THEN hx = homex(r) + (u - 224) / 2
    IF atk THEN
        hx = hx + 34
        IF r < 3 THEN hx = hx - 24
    ELSE
        hx = hx - 22
    END IF
    IF hx < 24 THEN hx = 24
    IF hx > 424 THEN hx = 424
    hy = FY0 + homey(r) + (byp - MY) * 3 / 8
    IF hy < FY0 + 10 THEN hy = FY0 + 10
    IF hy > FY1 - 10 THEN hy = FY1 - 10
    IF side(t) > 0 THEN ix = FX0 + hx ELSE ix = FX1 - hx
    iy = hy
END SUB

REM Un paso hacia (tx, ty), a esa velocidad
SUB moveto(i AS LONG, tx AS LONG, ty AS LONG, spd AS LONG)
    DIM dx, dy AS LONG
    DIM d AS UBYTE
    dx = tx - px(i) / F: dy = ty - py(i) / F
    IF dx > -3 AND dx < 3 AND dy > -3 AND dy < 3 THEN
        pmv(i) = 0
        RETURN
    END IF
    d = dir8(dx, dy)
    pd(i) = d
    px(i) = px(i) + ddx(d) * spd / F
    py(i) = py(i) + ddy(d) * spd / F
    pmv(i) = 1
END SUB

FUNCTION runspeed(i AS LONG) AS LONG
    DIM s AS LONG
    s = 330
    IF human(i / 7) = 0 THEN s = 285 + lvl * 22
    IF owner = i THEN s = s * 29 / 32
    RETURN s
END FUNCTION

REM El mejor compañero para pasarle: adelantado, libre y no muy lejos; -1 si no hay
FUNCTION bestMate(i AS LONG) AS LONG
    DIM j, k, d, adv, sc, best, bsc, o, savend AS LONG
    k = (i / 7) * 7: best = -1: bsc = -40
    savend = nd
    FOR j = k + 1 TO k + 6
        IF j <> i AND pst(j) = SNORMAL THEN
            d = dst((px(j) - px(i)) / F, (py(j) - py(i)) / F)
            IF d > 28 AND d < 190 THEN
                adv = (px(j) - px(i)) / F * side(i / 7)
                o = nearOpp(j)
                sc = adv - d / 4
                IF nd < 24 THEN sc = sc - 120
                IF sc > bsc THEN bsc = sc: best = j
            END IF
        END IF
    NEXT j
    nd = savend
    RETURN best
END FUNCTION

SUB passmate(i AS LONG, m AS LONG)
    DIM tx, ty AS LONG
    tx = px(m) / F: ty = py(m) / F
    IF pmv(m) THEN
        tx = tx + ddx(pd(m)) * 12 / F: ty = ty + ddy(pd(m)) * 12 / F
    END IF
    pd(i) = dir8(tx - px(i) / F, ty - py(i) / F)
    passto(tx, ty)
    recv = m
    snd(1)
END SUB

REM El pase del que maneja el jugador: al compañero que tiene delante
SUB humanPass(i AS LONG)
    DIM j, k, d, dx, dy, dot, best, bd AS LONG
    k = (i / 7) * 7: best = -1: bd = 99999
    FOR j = k + 1 TO k + 6
        IF j <> i THEN
            dx = (px(j) - px(i)) / F: dy = (py(j) - py(i)) / F
            d = dst(dx, dy)
            dot = dx * ddx(pd(i)) + dy * ddy(pd(i))
            IF d > 14 AND d < 230 AND dot > d * 170 THEN
                IF d < bd THEN bd = d: best = j
            END IF
        END IF
    NEXT j
    IF best >= 0 THEN
        passmate(i, best)
    ELSE
        passto(px(i) / F + ddx(pd(i)) * 90 / F, py(i) / F + ddy(pd(i)) * 90 / F)
        recv = -1
        snd(1)
    END IF
END SUB

REM El tiro: a puerta si la tiene delante; si no, un balón largo. pw, de 10 a 40
SUB humanShoot(i AS LONG, pw AS LONG)
    DIM t, gx, d, vz, ty AS LONG
    t = i / 7
    IF side(t) > 0 THEN gx = FX1 ELSE gx = FX0
    d = (gx - px(i) / F) * side(t)
    IF ddx(pd(i)) * side(t) > 0 AND d < 230 THEN
        ty = MY + SGN(ddy(pd(i))) * 24 + rn(13) - 6
        vz = 60 + pw * 8
        IF pw >= 38 THEN vz = vz + 310          ' demasiado: por encima del larguero
        kickto(gx + side(t) * 8, ty, 1150 + pw * 20, vz)
    ELSE
        kickto(px(i) / F + ddx(pd(i)) * 200 / F, py(i) / F + ddy(pd(i)) * 200 / F, 1100 + pw * 18, 120 + pw * 10)
    END IF
    recv = -1
    snd(0)
END SUB

SUB aiShoot(i AS LONG)
    DIM t, gx, ty, k, span AS LONG
    t = i / 7
    IF side(t) > 0 THEN gx = FX1 ELSE gx = FX0
    k = 7 - t * 7                               ' el portero rival
    ty = MY + 24
    IF py(k) / F > MY THEN ty = MY - 24
    span = 64 - lvl * 12
    ty = ty + rn(span) - span / 2
    pd(i) = dir8(gx - px(i) / F, ty - py(i) / F)
    kickto(gx + side(t) * 8, ty, 1350 + rn(350), 100 + rn(220))
    ev(5, i)
    recv = -1
    snd(0)
END SUB

REM El que lleva el balón, si es de la máquina
SUB aiOwner(i AS LONG)
    DIM t, s, x, y, gx, dg, o, m, tx, ty AS LONG
    t = i / 7: s = side(t)
    x = px(i) / F: y = py(i) / F
    IF s > 0 THEN gx = FX1 ELSE gx = FX0
    dg = dst(gx - x, MY - y)
    IF pth(i) < 250 THEN pth(i) = pth(i) + 1
    o = nearOpp(i)
    IF pth(i) > 14 - lvl * 4 THEN
        IF dg < 150 AND ABS(y - MY) < 96 THEN
            IF dg < 84 OR nd < 30 OR rn(40) = 0 THEN
                aiShoot(i)
                RETURN
            END IF
        END IF
        IF nd < 28 OR rn(100) = 0 THEN
            m = bestMate(i)
            IF m >= 0 THEN
                passmate(i, m)
                RETURN
            END IF
        END IF
    END IF
    tx = gx: ty = MY + (y - MY) / 2
    IF dg < 150 THEN ty = MY
    IF nd < 40 AND o >= 0 THEN
        IF (px(o) - px(i)) * s > 0 THEN         ' lo tiene delante: lo esquiva
            IF py(o) > py(i) THEN ty = y - 40 ELSE ty = y + 40
            tx = x + s * 40
        END IF
    END IF
    IF ty < FY0 + 8 THEN ty = FY0 + 8
    IF ty > FY1 - 8 THEN ty = FY1 - 8
    moveto(i, tx, ty, runspeed(i))
END SUB

REM El portero
SUB keeper(i AS LONG)
    DIM t, s, gx, x, y, ty, tx, dxl, tt, m, k AS LONG
    t = i / 7: s = side(t)
    IF s > 0 THEN gx = FX0 ELSE gx = FX1         ' su portería
    x = px(i) / F: y = py(i) / F
    IF owner = i THEN
        IF ptm(i) > 0 THEN
            ptm(i) = ptm(i) - 1
            pd(i) = 0: IF s < 0 THEN pd(i) = 4
            RETURN
        END IF
        m = t * 7 + 3 + rn(3)
        lobto(px(m) / F + s * 20, py(m) / F, 420)
        recv = m
        snd(0)
        RETURN
    END IF
    tx = gx + s * 8
    ty = MY + (byp - MY) / 3
    IF ty < MY - 28 THEN ty = MY - 28
    IF ty > MY + 28 THEN ty = MY + 28
    IF owner < 0 THEN
        dxl = (bxp - gx) * s                    ' lo que le falta al balón para la línea
        IF bvx * s < -380 AND dxl < 170 AND dxl > 0 THEN
            tt = dxl * F / (-bvx * s)           ' imágenes que tarda
            ty = byp + bvy * tt / F
            IF ty > MY - GH - 12 AND ty < MY + GH + 12 THEN
                IF tt < 22 AND ABS(ty - y) > 7 AND pst(i) = SNORMAL THEN
                    REM Se estira para llegar a la vez que el balón, si le da tiempo
                    IF tt < 3 THEN tt = 3
                    pvy(i) = (ty - y) * F / tt
                    k = 380 + lvl * 50
                    IF human(t) THEN k = 700
                    IF pvy(i) > k THEN pvy(i) = k
                    IF pvy(i) < -k THEN pvy(i) = -k
                    pst(i) = SDIVE: ptm(i) = tt + 3
                    pd(i) = 6: IF ty > y THEN pd(i) = 2
                    RETURN
                END IF
            ELSE
                ty = MY + (byp - MY) / 3
            END IF
        ELSEIF ABS(bvx) + ABS(bvy) < 500 AND dxl < AREAW AND ABS(byp - MY) < AREAH THEN
            k = nearOpp(i)                      ' un balón suelto en el área: sale a por él
            IF dst(bxp - x, byp - y) < dst(bxp - px(k) / F, byp - py(k) / F) + 10 THEN
                tx = bxp: ty = byp
            END IF
        END IF
    END IF
    IF owner >= 0 AND owner / 7 <> t THEN       ' le entran con el balón: sale a por él
        dxl = (bxp - gx) * s
        IF dxl < AREAW - 14 AND ABS(byp - MY) < AREAH - 20 AND dst(bxp - x, byp - y) < 50 THEN
            tx = bxp: ty = byp
        END IF
    END IF
    moveto(i, tx, ty, 300)
    IF pmv(i) = 0 THEN pd(i) = dir8(bxp - x, byp - y)
END SUB

REM Un jugador de campo que lleva la máquina
SUB aiPlayer(i AS LONG)
    DIM t, tx, ty, gx, d AS LONG
    t = i / 7
    IF gs = GDEAD THEN                          ' un saque: cada uno a su sitio, y los rivales, lejos
        IF t = taker / 7 THEN homepos(i, 1) ELSE homepos(i, 0)
        tx = ix: ty = iy
        IF t <> taker / 7 AND dst(tx - bxp, ty - byp) < 64 THEN
            IF side(t) > 0 THEN tx = bxp - 70 ELSE tx = bxp + 70
            IF tx < FX0 + 6 OR tx > FX1 - 6 THEN
                tx = bxp
                IF ty < MY THEN ty = byp + 70 ELSE ty = byp - 70
            END IF
        END IF
        moveto(i, tx, ty, runspeed(i))
        IF pmv(i) = 0 THEN pd(i) = dir8(bxp - px(i) / F, byp - py(i) / F)
        RETURN
    END IF
    IF owner = i THEN
        aiOwner(i)
        RETURN
    END IF
    IF owner >= 0 AND owner / 7 = t THEN        ' ataca su equipo
        homepos(i, 1)
        tx = ix: ty = iy
    ELSEIF i = recv OR i = chase(t) THEN
        tx = tbx: ty = tby
        IF owner >= 0 THEN
            tx = bxp: ty = byp
            d = dst(tx - px(i) / F, ty - py(i) / F)
            IF d < 18 AND d > 6 AND pcd(i) = 0 AND rn(50 - lvl * 12) = 0 THEN
                pd(i) = dir8(tx - px(i) / F, ty - py(i) / F)
                pst(i) = SSLIDE: ptm(i) = 18
                RETURN
            END IF
        END IF
    ELSEIF owner >= 0 AND i = cover(t) THEN     ' entre el balón y su portería
        IF side(t) > 0 THEN gx = FX0 ELSE gx = FX1
        tx = bxp + (gx - bxp) / 3: ty = byp + (MY - byp) / 3
    ELSE
        homepos(i, 0)
        tx = ix: ty = iy
    END IF
    moveto(i, tx, ty, runspeed(i))
    IF pmv(i) = 0 THEN pd(i) = dir8(bxp - px(i) / F, byp - py(i) / F)
END SUB

REM --- Los mandos ------------------------------------------------------------

REM Deja en ix, iy e ifire lo que pulsa el jugador m (1 o 2)
SUB getin(m AS UBYTE)
    DIM j AS UINTEGER
    ix = 0: iy = 0: ifire = 0
    IF m = 1 THEN
        j = JOY(0)
        IF KEY(8) THEN ix = -1
        IF KEY(9) THEN ix = 1
        IF KEY(11) THEN iy = -1
        IF KEY(10) THEN iy = 1
        IF KEY(32) THEN ifire = 1
    ELSE
        j = JOY(1)
        IF KEY(CODE "a") THEN ix = -1
        IF KEY(CODE "d") THEN ix = 1
        IF KEY(CODE "w") THEN iy = -1
        IF KEY(CODE "s") THEN iy = 1
        IF KEY(CODE "f") THEN ifire = 1
    END IF
    IF j BAND 2 THEN ix = -1
    IF j BAND 1 THEN ix = 1
    IF j BAND 8 THEN iy = -1
    IF j BAND 4 THEN iy = 1
    IF j BAND 16 THEN ifire = 1
END SUB

REM Cualquier botón o tecla de empezar
FUNCTION anyfire() AS UBYTE
    IF KEY(32) OR KEY(13) OR KEY(CODE "f") THEN RETURN 1
    IF (JOY(0) BAND 16) OR (JOY(1) BAND 16) THEN RETURN 1
    RETURN 0
END FUNCTION

REM El jugador que maneja el equipo t
SUB humanPlayer(t AS LONG)
    DIM i, d AS LONG
    i = ctl(t)
    getin(human(t))
    IF pst(i) = SNORMAL THEN
        IF ix <> 0 OR iy <> 0 THEN
            pd(i) = dir8(ix, iy)
            d = runspeed(i)
            px(i) = px(i) + ddx(pd(i)) * d / F
            py(i) = py(i) + ddy(pd(i)) * d / F
            pmv(i) = 1
        ELSE
            pmv(i) = 0
        END IF
        IF owner = i THEN
            IF ifire THEN
                IF fprev(t) = 0 THEN fon(t) = 1: fhold(t) = 0
                IF fon(t) THEN
                    fhold(t) = fhold(t) + 1
                    IF fhold(t) >= 40 THEN
                        humanShoot(i, 40): fon(t) = 0
                    END IF
                END IF
            ELSEIF fon(t) THEN
                fon(t) = 0
                IF fhold(t) < 10 THEN
                    humanPass(i)
                ELSE
                    humanShoot(i, fhold(t))
                END IF
            END IF
        ELSE
            fon(t) = 0
            IF ifire AND fprev(t) = 0 AND pcd(i) = 0 THEN
                pst(i) = SSLIDE: ptm(i) = 18
            END IF
        END IF
    END IF
    fprev(t) = ifire
END SUB

REM Quién maneja cada jugador: el que lleva el balón, el que lo va a recibir
REM o el que está más cerca
SUB pickctl()
    DIM t, c AS LONG
    FOR t = 0 TO 1
        IF human(t) THEN
            IF ctlsw(t) > 0 THEN ctlsw(t) = ctlsw(t) - 1
            c = ctl(t)
            IF owner >= 0 AND owner / 7 = t THEN
                IF owner MOD 7 <> 0 THEN c = owner
            ELSEIF owner < 0 AND recv >= 0 AND recv / 7 = t THEN
                c = recv
            ELSEIF chase(t) >= 0 AND chase(t) <> c AND ctlsw(t) = 0 THEN
                IF dst(px(c) / F - tbx, py(c) / F - tby) > dst(px(chase(t)) / F - tbx, py(chase(t)) / F - tby) + 14 THEN
                    c = chase(t)
                END IF
            END IF
            IF c <> ctl(t) THEN ctl(t) = c: ctlsw(t) = 24: fon(t) = 0
        END IF
    NEXT t
END SUB

REM --- Los saques ------------------------------------------------------------

REM Todos a su sitio para un saque de centro del equipo t
SUB kickoff(t AS LONG)
    DIM i, r, u, k AS LONG
    FOR i = 0 TO 13
        r = i MOD 7: k = i / 7
        u = homex(r)
        IF u > 180 THEN u = 180
        IF side(k) > 0 THEN px(i) = (FX0 + u) * F ELSE px(i) = (FX1 - u) * F
        py(i) = (FY0 + homey(r)) * F
        pd(i) = 0: IF side(k) < 0 THEN pd(i) = 4
        pst(i) = SNORMAL: ptm(i) = 0: pcd(i) = 0: pmv(i) = 0: pth(i) = 0
    NEXT i
    taker = t * 7 + 6
    px(taker) = (MX - side(t) * 5) * F: py(taker) = MY * F
    i = t * 7 + 4
    px(i) = (MX - side(t) * 40) * F: py(i) = (MY + 12) * F
    bx = MX * F: by = MY * F: bz = 0: bvx = 0: bvy = 0: bvz = 0
    owner = taker: recv = -1: lastT = t
    pst(taker) = SSET
    gs = GDEAD: deadkind = 0: gst = 0
    ctl(0) = 6: ctl(1) = 13
    fon(0) = 0: fon(1) = 0
END SUB

REM Un saque del equipo t desde (x, y): 1 de banda, 2 de esquina, 3 de puerta
SUB deadball(kind AS UBYTE, t AS LONG, x AS LONG, y AS LONG)
    DIM i, j, d, bd AS LONG
    IF kind = 3 THEN
        taker = t * 7
    ELSE
        bd = 99999
        FOR j = 1 TO 6
            i = t * 7 + j
            d = dst(px(i) / F - x, py(i) / F - y)
            IF d < bd THEN bd = d: taker = i
        NEXT j
    END IF
    FOR i = 0 TO 13
        IF pst(i) <> SNORMAL THEN pst(i) = SNORMAL: ptm(i) = 0
    NEXT i
    px(taker) = x * F: py(taker) = y * F
    bx = x * F: by = y * F: bz = 0: bvx = 0: bvy = 0: bvz = 0
    owner = taker: recv = -1: lastT = t
    pst(taker) = SSET
    pd(taker) = dir8(MX - x, MY - y)
    gs = GDEAD: deadkind = kind: gst = 0
    ev(1 + kind, t)
    IF human(t) THEN ctl(t) = taker
    fon(0) = 0: fon(1) = 0
    snd(2)
    IF kind = 1 THEN msg$ = tr$("THROW-IN", "SAQUE DE BANDA")
    IF kind = 2 THEN msg$ = tr$("CORNER", "CORNER")
    IF kind = 3 THEN msg$ = tr$("GOAL KICK", "SAQUE DE PUERTA")
    msgt = 70
END SUB

REM El que saca, cuando le toca: la máquina sola; el jugador, con su botón
SUB dotake()
    DIM t, m, tx, ty, gx AS LONG
    t = taker / 7
    gst = gst + 1
    IF human(t) AND taker MOD 7 <> 0 THEN
        getin(human(t))
        IF ix <> 0 OR iy <> 0 THEN pd(taker) = dir8(ix, iy)
        IF ifire = 0 AND fprev(t) = 1 AND gst > 20 THEN
            IF deadkind = 2 THEN
                lobto(px(taker) / F + ddx(pd(taker)) * 110 / F, py(taker) / F + ddy(pd(taker)) * 110 / F, 420)
                snd(0)
            ELSE
                humanPass(taker)
            END IF
            gs = GPLAY
        END IF
        fprev(t) = ifire
        RETURN
    END IF
    IF gst < 55 THEN RETURN
    IF deadkind = 2 THEN                        ' al área, por alto
        IF side(t) > 0 THEN gx = FX1 ELSE gx = FX0
        lobto(gx - side(t) * (30 + rn(30)), MY + rn(50) - 25, 430)
        snd(0)
    ELSEIF deadkind = 3 THEN
        m = t * 7 + 3 + rn(3)
        lobto(px(m) / F, py(m) / F, 430)
        recv = m
        snd(0)
    ELSEIF deadkind = 0 THEN
        passmate(taker, t * 7 + 4)
    ELSE
        m = bestMate(taker)
        IF m < 0 THEN m = t * 7 + 4
        passmate(taker, m)
    END IF
    gs = GPLAY
END SUB

REM --- Una imagen de juego ---------------------------------------------------

SUB goal(t AS LONG)
    goals(t) = goals(t) + 1
    ev(1, t)
    gs = GGOAL: gst = 0
    msg$ = tr$("GOAL!", "GOOOL!"): msgt = 150
    owner = -1: recv = -1
    lastT = 1 - t                               ' saca el que lo ha encajado
    snd(3)
END SUB

REM El balón suelto: rueda, bota y sale
SUB ballmove()
    DIM t AS LONG
    bx = bx + bvx: by = by + bvy: bz = bz + bvz
    IF bz > 0 THEN
        bvz = bvz - GRAV
    ELSE
        bz = 0
        IF bvz < -90 THEN
            bvz = -bvz * 5 / 12
            snd(4)
        ELSE
            bvz = 0
        END IF
        bvx = bvx - bvx / 32: bvy = bvy - bvy / 32
        IF bvx > -14 AND bvx < 14 AND bvy > -14 AND bvy < 14 THEN bvx = 0: bvy = 0
    END IF
    bxp = bx / F: byp = by / F: bzp = bz / F
    spin = spin + (ABS(bvx) + ABS(bvy)) / 64
    IF gs <> GPLAY THEN RETURN
    IF bxp < FX0 OR bxp > FX1 THEN
        t = 0: IF bxp < FX0 THEN t = 1          ' el equipo que ataca esa portería...
        IF side(0) < 0 THEN t = 1 - t
        IF ABS(byp - MY) < GH AND bzp < BAR THEN
            goal(t)
        ELSEIF ABS(byp - MY) < GH + 3 AND bzp < BAR + 2 THEN    ' el palo
            bvx = -bvx / 2: bx = bx + bvx * 2
            snd(4)
        ELSEIF lastT = t THEN                   ' ...la echó fuera: saque de puerta
            IF bxp < FX0 THEN deadball(3, 1 - t, FX0 + 20, MY) ELSE deadball(3, 1 - t, FX1 - 20, MY)
        ELSE
            IF bxp < FX0 THEN bxp = FX0 + 2 ELSE bxp = FX1 - 2
            IF byp < MY THEN deadball(2, t, bxp, FY0 + 2) ELSE deadball(2, t, bxp, FY1 - 2)
        END IF
    ELSEIF byp < FY0 OR byp > FY1 THEN
        IF bxp < FX0 + 6 THEN bxp = FX0 + 6
        IF bxp > FX1 - 6 THEN bxp = FX1 - 6
        IF byp < FY0 THEN deadball(1, 1 - lastT, bxp, FY0 + 1) ELSE deadball(1, 1 - lastT, bxp, FY1 - 1)
    END IF
END SUB

REM Quién se hace con el balón
SUB ballown()
    DIM i, d, best, bd, reach, o, v, t AS LONG
    IF owner >= 0 THEN
        o = owner
        IF pst(o) = SHOLD OR pst(o) = SSET THEN
            bx = px(o): by = py(o) - 5 * F: bz = 0
            IF pst(o) = SSET THEN by = py(o) + ddy(pd(o)) * 5: bx = px(o) + ddx(pd(o)) * 5
        ELSE
            bx = px(o) + ddx(pd(o)) * 6: by = py(o) + ddy(pd(o)) * 4: bz = 0
            IF pmv(o) THEN spin = spin + 5
            REM Un rival se lo quita: seguro con una entrada, y a veces al chocar
            t = 7: IF o >= 7 THEN t = 0
            FOR i = t TO t + 6
                IF pcd(i) = 0 AND (pst(i) = SNORMAL OR pst(i) = SSLIDE) THEN
                    d = dst((px(i) - bx) / F, (py(i) - by) / F)
                    IF i = t THEN                       ' el portero se echa a sus pies
                        IF d < 8 AND rn(4) = 0 THEN
                            owner = i: pcd(o) = 40: lastT = t / 7
                            pst(i) = SHOLD: ptm(i) = 50
                            ev(6, 2)
                            EXIT FOR
                        END IF
                    ELSEIF pst(i) = SSLIDE AND d < 9 THEN
                        owner = -1: bvx = ddx(pd(i)) * 3: bvy = ddy(pd(i)) * 3: bvz = 60
                        pst(o) = SDOWN: ptm(o) = 40: pcd(o) = 50: lastT = i / 7
                        ev(7, 1)
                        snd(1)
                        EXIT FOR
                    ELSEIF pst(i) = SNORMAL AND d < 6 THEN
                        IF rn(16 - lvl * 2 * (1 - human(i / 7) / 2)) = 0 THEN
                            owner = i: pcd(o) = 40: pth(i) = 0: lastT = i / 7
                            ev(7, 0)
                            EXIT FOR
                        END IF
                    END IF
                END IF
            NEXT i
        END IF
        bxp = bx / F: byp = by / F: bzp = bz / F
        RETURN
    END IF
    best = -1: bd = 99999
    FOR i = 0 TO 13
        IF pcd(i) = 0 AND pst(i) <> SDOWN THEN
            reach = 6
            IF pst(i) = SSLIDE THEN reach = 8
            IF i MOD 7 = 0 THEN
                reach = 8
                IF pst(i) = SDIVE THEN reach = 11
                IF bzp > BAR + 2 THEN reach = 0
            ELSEIF bzp > 10 THEN
                reach = 0
            END IF
            d = dst(px(i) / F - bxp, py(i) / F - byp)
            IF d < reach AND d < bd THEN bd = d: best = i
        END IF
    NEXT i
    IF best < 0 THEN RETURN
    i = best
    v = (ABS(bvx) + ABS(bvy)) / F
    IF i MOD 7 = 0 THEN
        IF v >= 4 AND rn(10) < v - 3 + lvl * (1 - human(i / 7) / 2) / 2 THEN
            bvx = -bvx / 3: bvy = bvy / 2 + (rn(400) - 200): bvz = 200   ' la despeja
            pcd(i) = 30: lastT = i / 7
            ev(6, 0)
            snd(4)
            RETURN
        END IF
        pst(i) = SHOLD: ptm(i) = 50
        ev(6, 1)
    END IF
    owner = i: recv = -1: lastT = i / 7: pth(i) = 0
    bvx = 0: bvy = 0: bvz = 0
END SUB

REM Mueve a los 14
SUB players()
    DIM i, t AS LONG
    FOR i = 0 TO 13
        t = i / 7
        IF pcd(i) > 0 THEN pcd(i) = pcd(i) - 1
        IF pst(i) = SSLIDE THEN
            px(i) = px(i) + ddx(pd(i)) * (ptm(i) * 30 + 120) / F
            py(i) = py(i) + ddy(pd(i)) * (ptm(i) * 30 + 120) / F
            ptm(i) = ptm(i) - 1
            IF ptm(i) = 0 THEN pst(i) = SDOWN: ptm(i) = 16: pcd(i) = 16
        ELSEIF pst(i) = SDOWN THEN
            ptm(i) = ptm(i) - 1
            IF ptm(i) = 0 THEN pst(i) = SNORMAL
        ELSEIF pst(i) = SDIVE THEN
            py(i) = py(i) + pvy(i)
            ptm(i) = ptm(i) - 1
            IF ptm(i) = 0 THEN pst(i) = SDOWN: ptm(i) = 20
        ELSEIF pst(i) = SSET THEN
            pmv(i) = 0
        ELSEIF i MOD 7 = 0 THEN
            keeper(i)
        ELSEIF gs = GDEAD AND deadkind = 0 THEN
            pmv(i) = 0                          ' en el saque de centro, quietos
        ELSEIF human(t) > 0 AND ctl(t) = i AND gs = GPLAY THEN
            humanPlayer(t)
        ELSE
            aiPlayer(i)
        END IF
        IF px(i) < 10 * F THEN px(i) = 10 * F
        IF px(i) > 502 * F THEN px(i) = 502 * F
        IF py(i) < 20 * F THEN py(i) = 20 * F
        IF py(i) > 376 * F THEN py(i) = 376 * F
        IF pmv(i) THEN pa(i) = pa(i) + 1 ELSE pa(i) = 0
    NEXT i
END SUB

REM Quién va al balón en cada equipo, y el que le sigue
SUB think()
    DIM t, i, j, d, best, best2 AS LONG
    tbx = bxp + bvx * 12 / F: tby = byp + bvy * 12 / F
    FOR t = 0 TO 1
        chase(t) = -1: cover(t) = -1: best = 99999: best2 = 99999
        FOR j = 1 TO 6
            i = t * 7 + j
            IF pst(i) = SNORMAL THEN
                d = dst(px(i) / F - tbx, py(i) / F - tby)
                IF d < best THEN
                    best2 = best: cover(t) = chase(t): best = d: chase(t) = i
                ELSEIF d < best2 THEN
                    best2 = d: cover(t) = i
                END IF
            END IF
        NEXT j
    NEXT t
END SUB

REM --- El dibujo -------------------------------------------------------------

REM Las porterías, de pie: el sprite 26 es la de la derecha, con sus postes en
REM la línea de fondo y la red detrás, y la de la izquierda es su espejo
SUB drawgoals()
    IF camx < FX0 + 2 THEN SPRITE 26, FX0 - NET - 1, MY - GH - BAR, 1
    IF camx + 256 > FX1 - 2 THEN SPRITE 26, FX1, MY - GH - BAR
END SUB

SUB drawfield()
    DIM k, x AS LONG
    FOR k = camx / 32 TO (camx + 255) / 32
        IF k MOD 2 THEN BOX 4, k * 32, camy, 32, 192 ELSE BOX 12, k * 32, camy, 32, 192
    NEXT k
    BOX 15, FX0, FY0, FX1 - FX0 + 1, 1
    BOX 15, FX0, FY1, FX1 - FX0 + 1, 1
    BOX 15, FX0, FY0, 1, FY1 - FY0 + 1
    BOX 15, FX1, FY0, 1, FY1 - FY0 + 1
    BOX 15, MX, FY0, 1, FY1 - FY0 + 1
    BOX 15, MX - 1, MY - 1, 3, 3
    IF camx < MX + 48 AND camx + 256 > MX - 48 THEN
        FOR k = 0 TO 23
            POLY 15, ccx(k), ccy(k), ccx(k + 1), ccy(k + 1)
        NEXT k
    END IF
    IF camx < FX0 + AREAW + 2 THEN              ' el área y la portería de la izquierda
        BOX 15, FX0, MY - AREAH, AREAW, 1
        BOX 15, FX0, MY + AREAH, AREAW, 1
        BOX 15, FX0 + AREAW, MY - AREAH, 1, AREAH * 2 + 1
        BOX 15, FX0, MY - 52, 26, 1
        BOX 15, FX0, MY + 52, 26, 1
        BOX 15, FX0 + 26, MY - 52, 1, 105
        BOX 15, FX0 + 50, MY - 1, 2, 2
    END IF
    IF camx + 256 > FX1 - AREAW - 2 THEN
        BOX 15, FX1 - AREAW, MY - AREAH, AREAW, 1
        BOX 15, FX1 - AREAW, MY + AREAH, AREAW, 1
        BOX 15, FX1 - AREAW, MY - AREAH, 1, AREAH * 2 + 1
        BOX 15, FX1 - 26, MY - 52, 26, 1
        BOX 15, FX1 - 26, MY + 52, 26, 1
        BOX 15, FX1 - 26, MY - 52, 1, 105
        BOX 15, FX1 - 51, MY - 1, 2, 2
    END IF
    drawgoals()
END SUB

SUB drawball()
    DIM z AS LONG
    SPRITE 23, bxp - 3, byp - 1
    z = 6 + bzp / 5
    IF z > 6 THEN
        SPRITE 20 + (spin / 24) MOD 3, bxp - z / 2, byp - 3 - bzp - (z - 6) / 2, 0, "", z, z
    ELSE
        SPRITE 20 + (spin / 24) MOD 3, bxp - 3, byp - 4 - bzp
    END IF
END SUB

SUB drawplayer(i AS LONG)
    DIM x, y, n, f AS LONG
    x = px(i) / F: y = py(i) / F
    IF x < camx - 12 OR x > camx + 268 OR y < camy - 4 OR y > camy + 212 THEN RETURN
    SPRITE 24, x - 5, y - 2
    f = sflip(pd(i))
    IF pst(i) = SSLIDE THEN
        n = 15
        IF pd(i) = 2 THEN n = 16
        IF pd(i) = 6 THEN n = 17
    ELSEIF pst(i) = SDOWN THEN
        n = 18
    ELSEIF pst(i) = SDIVE THEN
        n = 19: f = 0
        IF pd(i) = 2 THEN f = 2
    ELSE
        n = sbase(pd(i))
        IF pmv(i) THEN
            IF n = 6 THEN
                n = n + 1 + (pa(i) / 6) MOD 2
            ELSE
                n = n + srun((pa(i) / 6) MOD 4)
            END IF
        END IF
    END IF
    SPRITE n, x - 6, y - 17, f, pc$(i)
    IF human(i / 7) > 0 AND ctl(i / 7) = i THEN
        IF human(i / 7) = 1 THEN SPRITE 25, x - 2, y - 22 ELSE SPRITE 25, x - 2, y - 22, 0, cc$
    END IF
END SUB

SUB hud()
    DIM s, m AS LONG
    BOX 0, camx, camy, 256, 10
    txt(camx + 4, camy + 1, tabr$(team(0)), "")
    txt(camx + 36, camy + 1, STR$(goals(0)) + "-" + STR$(goals(1)), cy$)
    txt(camx + 68, camy + 1, tabr$(team(1)), "")
    s = clock * 2700 / halfFrames
    IF s > 2700 THEN s = 2700
    IF half = 2 THEN s = s + 2700
    m = s / 60
    txt(camx + 160, camy + 1, two$(m) + ":" + two$(s - m * 60), cy$)
    txt(camx + 216, camy + 1, STR$(half) + tr$("H", "T"), "")
    IF msgt > 0 THEN
        msgt = msgt - 1
        IF gs = GGOAL THEN
            bigtxt(camx + 128 - LEN(msg$) * 12, camy + 70, msg$, cy$, 3)
        ELSE
            txt(camx + 128 - LEN(msg$) * 4, camy + 180, msg$, cy$)
        END IF
    END IF
END SUB

SUB render()
    DIM i, j, k, tx, ty AS LONG
    REM La cámara, tras el balón
    tx = bxp - 128 + bvx * 10 / F: ty = byp - 100 + bvy * 10 / F
    IF tx < 0 THEN tx = 0
    IF tx > 256 THEN tx = 256
    IF ty < 0 THEN ty = 0
    IF ty > 192 THEN ty = 192
    camx = camx + (tx - camx) / 6: camy = camy + (ty - camy) / 6
    IF camx < tx THEN camx = camx + 1
    IF camx > tx THEN camx = camx - 1
    IF camy < ty THEN camy = camy + 1
    IF camy > ty THEN camy = camy - 1
    REM De arriba abajo, para que el de delante tape al de detrás
    FOR i = 1 TO 13
        k = ord(i): j = i - 1
        WHILE j >= 0 AND py(ord(j)) > py(k)
            ord(j + 1) = ord(j): j = j - 1
        END WHILE
        ord(j + 1) = k
    NEXT i
    SCREEN pg
    CLIP camx, camy, 256, 192
    drawfield()
    j = 0
    FOR i = 0 TO 13
        k = ord(i)
        IF j = 0 AND py(k) / F > byp + 2 THEN
            drawball(): j = 1
        END IF
        drawplayer(k)
    NEXT i
    IF j = 0 THEN drawball()
    IF bxp < FX0 OR bxp > FX1 THEN drawgoals()   ' el balón, dentro: la red por delante
    hud()
    CLIP
    SCROLL camx, camy
    PAGE pg
    pg = 1 - pg
    WAIT
END SUB

REM --- El partido ------------------------------------------------------------

REM Los colores de cada jugador: con ellos se pinta su sprite, que tiene la
REM camiseta y las medias en rojo (2), el pantalón en azul (1) y el pelo en
REM amarillo (6). El pelo cambia de uno a otro; los porteros van de otro color.
SUB colours()
    DIM i, t, k, sh, pa, kc AS LONG
    DIM hx$, hair$ AS STRING
    hx$ = "0123456789abcdef"
    hair$ = "026"
    FOR i = 0 TO 13
        t = i / 7
        sh = tsh(team(t)): pa = tpa(team(t))
        IF t = 1 AND tsh(team(1)) = tsh(team(0)) THEN   ' los dos de lo mismo: el segundo, de blanco
            sh = 15: pa = 0
            IF tsh(team(0)) = 15 THEN sh = 0: pa = 15
        END IF
        IF i MOD 7 = 0 THEN                             ' el portero: un color que no lleve nadie
            kc = 11
            IF tsh(team(0)) = 11 OR tsh(team(1)) = 11 OR tsh(team(0)) = 3 OR tsh(team(1)) = 3 THEN kc = 5
            IF t = 1 THEN
                kc = 14
                IF tsh(team(0)) = 14 OR tsh(team(1)) = 14 THEN kc = 5
                IF kc = 5 AND (tsh(team(0)) = 13 OR tsh(team(1)) = 13) THEN kc = 11
            END IF
            sh = kc: pa = 0
        END IF
        k = (i * 5 + i / 7) MOD 3
        pc$(i) = "0" + hx$(pa) + hx$(sh) + "345" + hair$(k)
    NEXT i
END SUB

SUB match()
    DIM i AS LONG
    colours()
    side(0) = 1: side(1) = -1
    goals(0) = 0: goals(1) = 0
    half = 1: clock = 0
    halfFrames = minutes * 60 * HZ
    FOR i = 0 TO 13: ord(i) = i: NEXT i
    camx = 128: camy = 96
    kickoff(0)
    msg$ = "": msgt = 0
    snd(2)
    DO
        IF demo AND anyfire() THEN EXIT DO
        IF gs = GPLAY THEN
            clock = clock + 1
            think()
            pickctl()
            players()
            IF owner < 0 THEN ballmove()
            IF gs = GPLAY THEN ballown()
            IF clock >= halfFrames AND gs = GPLAY THEN
                IF half = 1 THEN snd(2)
                IF half = 1 THEN
                    gs = GHALF: gst = 0: msg$ = tr$("HALF TIME", "DESCANSO"): msgt = 170
                ELSE
                    gs = GEND: gst = 0: msg$ = tr$("FULL TIME", "FINAL"): msgt = 10000
                    snd(5)
                END IF
            END IF
        ELSEIF gs = GDEAD THEN
            think()
            players()
            ballown()
            dotake()
        ELSEIF gs = GGOAL THEN
            gst = gst + 1
            players()
            ballmove()
            REM El balón se queda en la red
            IF bxp < FX0 - NET + 5 THEN bx = (FX0 - NET + 5) * F: bvx = 0
            IF bxp > FX1 + NET - 5 THEN bx = (FX1 + NET - 5) * F: bvx = 0
            IF byp < MY - GH + 4 THEN by = (MY - GH + 4) * F: bvy = 0
            IF byp > MY + GH - 4 THEN by = (MY + GH - 4) * F: bvy = 0
            IF gst > 150 THEN kickoff(lastT)
        ELSEIF gs = GHALF THEN
            gst = gst + 1
            IF gst > 170 THEN
                half = 2: clock = 0
                side(0) = -1: side(1) = 1
                kickoff(1)
            END IF
        ELSE
            gst = gst + 1
            IF gst > 60 AND anyfire() THEN EXIT DO
            IF gst > 600 THEN EXIT DO
        END IF
        render()
    LOOP
    IF hassfx THEN SFXSTOP
#ifdef DEBUGLOG
    ev(9, goals(0) * 100 + goals(1))
    SAVE "pslog.bin" CODE @elog(0), nlog * 4
    nlog = 0
#endif
END SUB

REM --- El título y las opciones ----------------------------------------------

DIM opt, players1 AS LONG

SUB menurow(r AS LONG, a$ AS STRING, b$ AS STRING)
    IF opt = r THEN
        PRINT AT 9 + r * 2, 3; INK 6; BRIGHT 1; ">"; a$; AT 9 + r * 2, 18; b$
    ELSE
        PRINT AT 9 + r * 2, 3; INK 7; BRIGHT 1; " "; a$; AT 9 + r * 2, 18; b$
    END IF
END SUB

SUB menu()
    DIM idle, held, k AS LONG
    DIM lv$(2) AS STRING
    lv$(0) = tr$("EASY", "FACIL"): lv$(1) = "NORMAL": lv$(2) = tr$("HARD", "DIFICIL")
    SCROLL 0, 0
    idle = 0: held = 1: demo = 0
    colours()
    MUSICPLAY
    DO
        SCREEN pg
        BOX 9, 0, 0, 256, 192                   ' azul: las letras blancas y amarillas se leen bien
        BOX 0, 0, 0, 256, 56
        BOX 15, 0, 56, 256, 1
        bigtxt(128 - 12 * 8, 4, "PETIT SOCCER", cw$, 2)
        SPRITE 9, 60, 20, 0, pc$(4), 24, 36            ' uno de cada equipo, con el balón en medio
        SPRITE 9, 172, 20, 1, pc$(11), 24, 36
        SPRITE 20 + (idle / 8) MOD 3, 122, 40, 0, "", 12, 12
        PAPER 1
        menurow(0, tr$("PLAYERS", "JUGADORES"), STR$(players1))
        menurow(1, tr$("TEAM 1", "EQUIPO 1"), tname$(team(0)))
        menurow(2, tr$("TEAM 2", "EQUIPO 2"), tname$(team(1)))
        menurow(3, tr$("MINUTES", "MINUTOS"), STR$(minutes * 2))
        menurow(4, tr$("LEVEL", "NIVEL"), lv$(lvl))
        PRINT AT 20, 3; INK 7; BRIGHT 1; tr$("SPACE OR FIRE: PLAY", "ESPACIO O FUEGO: JUGAR")
        PRINT AT 22, 3; INK 5; BRIGHT 1; tr$("1: CURSORS+SPACE  2: WASD+F", "1: CURSORES+ESPACIO 2: WASD+F")
        PAGE pg
        pg = 1 - pg
        WAIT
        getin(1)
        IF KEY(CODE "d") THEN demo = 1: EXIT DO
        idle = idle + 1
        IF ix = 0 AND iy = 0 AND ifire = 0 THEN
            held = 0
            IF idle > 15 * HZ THEN demo = 1: EXIT DO
        ELSEIF held = 0 THEN
            held = 1: idle = 0
            IF ifire THEN EXIT DO
            opt = opt + iy
            IF opt < 0 THEN opt = 4
            IF opt > 4 THEN opt = 0
            IF ix <> 0 THEN
                IF opt = 0 THEN players1 = 3 - players1
                IF opt = 1 THEN team(0) = (team(0) + NTEAMS + ix) MOD NTEAMS
                IF opt = 2 THEN team(1) = (team(1) + NTEAMS + ix) MOD NTEAMS
                IF opt = 3 THEN
                    minutes = minutes + ix
                    IF minutes < 1 THEN minutes = 5
                    IF minutes > 5 THEN minutes = 1
                END IF
                IF opt = 4 THEN lvl = (lvl + 3 + ix) MOD 3
                colours()
            END IF
        END IF
    LOOP
    MUSICSTOP
    human(0) = 1: human(1) = 0
    IF players1 = 2 THEN human(1) = 2
    IF demo THEN human(0) = 0: human(1) = 0
    DO                                          ' que suelte el botón
        WAIT
    LOOP UNTIL anyfire() = 0
END SUB

SUB defteam(n AS LONG, nm$ AS STRING, ab$ AS STRING, sh AS UBYTE, pa AS UBYTE)
    tname$(n) = nm$: tabr$(n) = ab$
    tsh(n) = sh: tpa(n) = pa
END SUB

SUB init()
    DIM i AS LONG
    es = 0
    IF PEEK(ULONG, 536872736) = 29541 THEN es = 1      ' la variable LANG del sistema: "es"
    MODE 0, 512, 384
    SCREEN 1                                            ' las letras, en la página que no se ve
    PAPER 0: INK 7: BRIGHT 1: BORDER 0: CLS
    FOR i = 32 TO 95
        PRINT AT (i - 32) / 32, (i - 32) MOD 32; CHR$(i);
    NEXT i
    FOR i = 32 TO 95
        GRAB TXT + i - 32, ((i - 32) MOD 32) * 8, ((i - 32) / 32) * 8, 8, 8, 8
    NEXT i
    CLS
    LoadSprites()
    cw$ = "": cy$ = "...............e": cc$ = "..............d."
    SFXLOAD "petitsoccer"
    MUSICLOAD "petitsoccer"
    hassfx = 1
    FOR i = 0 TO 24
        ccx(i) = MX + COS(i * PI / 12) * 44
        ccy(i) = MY + SIN(i * PI / 12) * 44
    NEXT i
    defteam(0, tr$("REDS", "ROJOS"), tr$("RED", "ROJ"), 10, 15)
    defteam(1, tr$("BLUES", "AZULES"), tr$("BLU", "AZU"), 1, 15)
    defteam(2, tr$("WHITES", "BLANCOS"), tr$("WHI", "BLA"), 15, 0)
    defteam(3, tr$("BLACKS", "NEGROS"), tr$("BLK", "NEG"), 0, 15)
    defteam(4, tr$("YELLOWS", "AMARILLOS"), tr$("YEL", "AMA"), 14, 1)
    defteam(5, tr$("PURPLES", "MORADOS"), tr$("PUR", "MOR"), 3, 15)
    defteam(6, tr$("SKY BLUES", "CELESTES"), tr$("SKY", "CEL"), 13, 1)
    defteam(7, tr$("PINKS", "ROSAS"), tr$("PNK", "ROS"), 11, 0)
    team(0) = 0: team(1) = 1
    minutes = 2: lvl = 1: players1 = 1: opt = 0
    seed = TIME + 7
    pg = 1
END SUB

init()
DO
    menu()
    seed = seed + TIME
    match()
LOOP
