/*
 * world.c - Las zonas de Gone Silent:
 *
 *   el exterior, con el módulo de aterrizaje, los habitáculos, control y
 *   comunicaciones con su antena, la planta de terraformación, la entrada
 *   de la mina, una torreta de las defensas, dos criaturas, rocas y
 *   montañas a lo lejos;
 *   los habitáculos, la primera instalación: el vestíbulo, el comedor, el
 *   pasillo, los dormitorios y la enfermería, y detrás, en otra zona, el
 *   invernadero y el taller. Los colonos se atrincheraron: las barricadas
 *   cortan el paso directo al pasillo y al taller. Allí están la tarjeta de
 *   control, el filtro, la primera pieza del módulo y seis registros; el
 *   último da el código de control;
 *   control y comunicaciones, que se abre con la tarjeta y el código:
 *   sellado y a oscuras, con criaturas que entraron por los conductos. En
 *   los generadores se devuelve la energía (la luz, la recarga y las puertas
 *   de la planta y la mina); en seguridad se apagan las torretas; en la sala
 *   de mando está la segunda pieza. Cuatro registros cuentan el sabotaje de
 *   la antena;
 *   la mina, que se abre con la energía: el ascensor baja al nivel 1, con el
 *   cortador; de ahí al 2, con gas, donde se quedó el ascensor; y por el
 *   pozo de ventilación, al 3, el nido, con muchas criaturas, el gas más
 *   fuerte, la incubadora de la que salen todas y la tercera pieza. Cuatro
 *   registros de los mineros;
 *   la planta de terraformación, que se abre con la energía y sigue
 *   funcionando a tope, con gas: la sala de control, los depósitos con una
 *   bomba de inyección y la nave de los procesadores con las otras dos. Con
 *   las tres cerradas, la consola de control para la planta y las criaturas
 *   se calman. Al fondo, la puerta que soldó un operario, que se abre con el
 *   cortador y da al cuarto sellado, con la cuarta pieza. Tres registros.
 *
 * Con las cuatro piezas se repara el módulo con su panel; la última trae la
 * tormenta fuerte.
 *
 * Las piezas: la forma, cómo es, la puerta (la zona y la entrada), la
 * esquina de abajo al suroeste, el tamaño y el material de cada cara (en el
 * orden de las caras de su forma, en render.c), todo en centímetros.
 */
#include "gonesilent.h"

#define ALL(m)      { m, m, m, m, m, m, m, m }
#define BOX(x, y, z, sx, sy, sz, m)     { SH_BOX, OBJ_SOLID, -1, 0, x, y, z, sx, sy, sz, ALL(m), 0, 0, 0 }
#define PYR(x, y, z, sx, sy, sz, m)     { SH_PYRAMID, OBJ_SOLID, -1, 0, x, y, z, sx, sy, sz, ALL(m), 0, 0, 0 }
#define WEDGE(x, y, z, sx, sy, sz, m)   { SH_WEDGE, OBJ_SOLID, -1, 0, x, y, z, sx, sy, sz, ALL(m), 0, 0, 0 }
#define PRISM(x, y, z, sx, sy, sz, m)   { SH_PRISM, OBJ_SOLID, -1, 0, x, y, z, sx, sy, sz, ALL(m), 0, 0, 0 }
#define DOOR(x, y, z, sx, sy, sz, zone, entry) \
    { SH_BOX, OBJ_SOLID | OBJ_DOOR, zone, entry, x, y, z, sx, sy, sz, ALL(MAT_DOOR), 0, 0, 0 }
/* Una puerta que pide las marcas need. */
#define LOCKED(x, y, z, sx, sy, sz, zone, entry, need) \
    { SH_BOX, OBJ_SOLID | OBJ_DOOR, zone, entry, x, y, z, sx, sy, sz, ALL(MAT_DOOR), need, 0, 0 }
/* Un registro, el número n: al leerlo da give. */
#define LOG(x, y, z, sx, sy, sz, n, give) \
    { SH_BOX, OBJ_SOLID | OBJ_LOG, -1, 0, x, y, z, sx, sy, sz, \
      { MAT_METAL, MAT_METAL, MAT_GLOW, MAT_GLOW, MAT_GLOW, MAT_GLOW }, 0, give, n }
/* Un objeto, que flota donde está y se ve como un sprite (main.c elige cuál
 * por give): al cogerlo da give. */
#define ITEM(x, y, z, give) \
    { SH_BOX, OBJ_ITEM, -1, 0, x, y, z, 48, 48, 48, ALL(MAT_DARK), 0, give, 0 }
/* Una criatura, que caza a su manera (CR_...): su caja, en el suelo con el
 * centro en (x, z), que se ve como un sprite (main.c elige cuál). */
#define CREATURE(x, z, kind) \
    { SH_BOX, OBJ_CREATURE | OBJ_SPRITE, -1, 0, x - 60, 0, z - 60, 120, 160, 120, \
      ALL(MAT_DARK), 0, 0, kind }
/* La incubadora, pegada a la roca con el centro en (x, z), y lo que escupe,
 * escondido hasta que lo escupe. */
#define MOTHER(x, z) \
    { SH_BOX, OBJ_SOLID | OBJ_CREATURE | OBJ_SPRITE, -1, 0, x - 260, 0, z - 150, 520, 340, 300, \
      ALL(MAT_DARK), 0, 0, CR_MOTHER }
#define SPIT \
    { SH_OCTA, OBJ_MOVES | OBJ_HIDDEN | OBJ_SPIT, -1, 0, 0, 0, 0, 50, 66, 50, ALL(MAT_EYE), 0, 0, 0 }
/* El punto de recarga: una losa que brilla por arriba. */
#define CHARGER(x, z) \
    { SH_BOX, OBJ_SOLID | OBJ_CHARGE, -1, 0, x, 0, z, 300, 15, 300, \
      { MAT_METAL, MAT_GLOW, MAT_CYAN, MAT_CYAN, MAT_CYAN, MAT_CYAN }, 0, 0, 0 }
/* La torreta: el pie y la cabeza, que gira (con el yaw a 0 mira al norte).
 * Se apaga con la marca off (en need); el color de la cabeza dice si está
 * activa. */
#define TURRET(x, z, off) \
    BOX(x, 0, z, 120, 260, 120, MAT_METAL), \
    { SH_BOX, OBJ_SOLID | OBJ_MOVES | OBJ_TURRET, -1, 0, x - 20, 260, z - 50, 160, 90, 220, ALL(MAT_HAZARD), \
      off, 0, 0 }
/* El mobiliario, que no sale en el mapa: macizo (FURN) o no (DECO: lo que
 * está tirado por el suelo, encima de algo o colgado), y piedras sueltas. */
#define FURN(x, y, z, sx, sy, sz, m)    { SH_BOX, OBJ_SOLID | OBJ_DECO, -1, 0, x, y, z, sx, sy, sz, ALL(m), 0, 0, 0 }
#define DECO(x, y, z, sx, sy, sz, m)    { SH_BOX, OBJ_DECO, -1, 0, x, y, z, sx, sy, sz, ALL(m), 0, 0, 0 }
#define STONES(x, z, s) \
    { SH_PYRAMID, OBJ_DECO, -1, 0, x, 0, z, s, s / 2, s, ALL(MAT_ROCK), 0, 0, 0 }
/* Una silla, con el respaldo al oeste o al este, y un bidón. */
#define CHAIR_W(x, z)   FURN(x, 0, z, 45, 48, 45, MAT_GRATE), DECO(x, 48, z, 8, 50, 45, MAT_GRATE)
#define CHAIR_E(x, z)   FURN(x, 0, z, 45, 48, 45, MAT_GRATE), DECO(x + 37, 48, z, 8, 50, 45, MAT_GRATE)
#define BARREL(x, z, m) FURN(x, 0, z, 60, 90, 60, m)
/* Las luces (main.c, lamp_update): una del techo, que se apaga sin energía, o
 * que falla (kind: L_CEIL o L_FLICKER); una baliza de emergencia, que gira sin
 * energía; y un aviso, que parpadea hasta tener off. */
#define LAMP(x, y, z, sx, sy, sz, kind) \
    { SH_BOX, OBJ_LAMP, -1, 0, x, y, z, sx, sy, sz, ALL(MAT_GLOW), 0, 0, kind }
#define BEACON(x, y, z) \
    { SH_BOX, OBJ_LAMP | OBJ_MOVES, -1, 0, x, y, z, 50, 40, 50, ALL(MAT_GRATE), 0, 0, L_BEACON }
#define WARN(x, y, z, sx, sy, sz, off) \
    { SH_BOX, OBJ_LAMP, -1, 0, x, y, z, sx, sy, sz, ALL(MAT_HAZARD), off, 0, L_WARN }
/* Lo que se usa con Intro: pide need y da give. Una pantalla dice si ya se
 * ha usado (L_SCREEN: naranja hasta tener done, verde después). */
#define SCREEN(x, y, z, sx, sy, sz, done) \
    { SH_BOX, OBJ_LAMP, -1, 0, x, y, z, sx, sy, sz, ALL(MAT_ALARM), done, 0, L_SCREEN }
#define USED(x, y, z, sx, sy, sz, need, give) \
    { SH_BOX, OBJ_SOLID | OBJ_USE, -1, 0, x, y, z, sx, sy, sz, ALL(MAT_GRATE), need, give, 0 }
/* Un terminal: el pupitre, con el teclado y un piloto, y detrás el cuerpo
 * (lo que se usa), con la pantalla. De w de ancho y 100 de fondo desde
 * (x, z), mirando al sur (TERM_S: el jugador, al sur) o al oeste (TERM_W). */
#define TERM_S(x, z, w, need, give) \
    FURN(x, 0, z, w, 110, 50, MAT_METAL), \
    DECO(x + 20, 110, z + 10, w - 40, 6, 25, MAT_DARK), \
    DECO(x + w / 2 - 10, 110, z + 38, 20, 8, 8, MAT_CYAN), \
    USED(x, 0, z + 50, w, 220, 50, need, give), \
    SCREEN(x + 15, 130, z + 45, w - 30, 70, 5, give)
#define TERM_W(x, z, w, need, give) \
    FURN(x, 0, z, 50, 110, w, MAT_METAL), \
    DECO(x + 10, 110, z + 20, 25, 6, w - 40, MAT_DARK), \
    DECO(x + 38, 110, z + w / 2 - 10, 8, 8, 20, MAT_CYAN), \
    USED(x + 50, 0, z, 50, 220, w, need, give), \
    SCREEN(x + 45, 130, z + 15, 5, 70, w - 30, give)
/* Una bomba de inyección, de 250 x 250 desde (x, z): el cuerpo (arriba, lo
 * que se usa), con un piloto que dice si está cerrada (como una pantalla) y
 * la tapa; y al sur, a la altura de los ojos, la rueda, que gira al cerrarla
 * (L_WHEEL): el aro y los dos radios, con su vástago y su eje. */
#define WHEEL(shape, x, y, z, sx, sy, sz, m, give) \
    { shape, OBJ_LAMP | OBJ_MOVES | OBJ_ROLL, -1, 0, x, y, z, sx, sy, sz, ALL(m), give, 0, L_WHEEL }
#define INJECTOR(x, z, give) \
    FURN(x, 0, z, 250, 135, 250, MAT_GRATE), \
    USED(x, 135, z, 250, 85, 250, 0, give), \
    SCREEN(x + 180, 190, z - 5, 40, 20, 5, give), \
    DECO(x - 10, 220, z - 10, 270, 15, 270, MAT_METAL), \
    DECO(x + 115, 150, z - 6, 20, 20, 6, MAT_METAL), \
    WHEEL(SH_OCTA, x + 65, 100, z - 20, 120, 120, 14, MAT_ORANGE, give), \
    WHEEL(SH_BOX, x + 65, 153, z - 28, 120, 14, 6, MAT_METAL, give), \
    WHEEL(SH_BOX, x + 118, 100, z - 28, 14, 120, 6, MAT_METAL, give), \
    DECO(x + 115, 150, z - 36, 20, 20, 8, MAT_DARK)

/* --- El exterior -------------------------------------------------------------- */

static const uint8_t outside_pal[16][3] = {
    { 0, 0, 0 },
    { 48, 24, 22 }, { 92, 46, 34 }, { 140, 76, 50 }, { 190, 118, 76 },     /* roca */
    { 34, 40, 56 }, { 66, 78, 100 }, { 108, 124, 146 }, { 160, 176, 192 },  /* metal */
    { 150, 60, 16 }, { 236, 138, 36 },                                      /* naranja */
    { 16, 90, 100 }, { 90, 230, 140 },                                      /* cian y verde */
    { 58, 30, 72 },                                                         /* el cielo */
    { 170, 96, 90 },                                                        /* la bruma */
    { 250, 244, 220 },                                                      /* la luz */
};

static const band_t outside_sky[] = {
    { 3, 14, 14, PAT_SOLID }, { 8, 14, 13, PAT_25 }, { 16, 14, 13, PAT_50 },
    { 30, 14, 13, PAT_75 }, { 255, 13, 13, PAT_SOLID },
};
/* El suelo, con el tono más oscuro (el 1): ninguna pared es así. */
static const band_t outside_ground[] = {
    { 2, 14, 1, PAT_50 }, { 8, 1, 14, PAT_25 }, { 24, 1, 1, PAT_SOLID },
    { 48, 1, 2, PAT_25 }, { 255, 1, 2, PAT_50 },
};

static const piece_t outside[] = {
    /* el módulo de aterrizaje, detrás de la salida */
    BOX(-700, 150, -1500, 500, 300, 500, MAT_METAL),
    PYR(-700, 450, -1500, 500, 250, 500, MAT_PANEL),
    BOX(-720, 0, -1520, 40, 150, 40, MAT_METAL),
    BOX(-220, 0, -1520, 40, 150, 40, MAT_METAL),
    BOX(-720, 0, -1020, 40, 150, 40, MAT_METAL),
    BOX(-220, 0, -1020, 40, 150, 40, MAT_METAL),
    /* el panel para repararlo, con su pantalla y sus botones */
    USED(-200, 120, -1350, 25, 180, 200, F_PARTS, F_ESCAPED),
    SCREEN(-175, 200, -1330, 5, 80, 160, F_ESCAPED),
    DECO(-175, 145, -1320, 6, 25, 140, MAT_DARK),
    CHARGER(-600, -900),
    /* control y comunicaciones: el edificio, el tejado, la puerta, que pide
       la tarjeta y el código, y su luz */
    { SH_BOX, OBJ_SOLID, -1, 0, -1500, 0, 4000, 3000, 700, 1600,
      { MAT_METAL, MAT_GRATE, MAT_PANEL, MAT_PANEL, MAT_PANEL, MAT_PANEL }, 0, 0, 0 },
    PRISM(-1500, 700, 4000, 1400, 300, 1600, MAT_PANEL),
    LOCKED(-150, 0, 3980, 300, 320, 20, 1, 0, F_CARD | F_CODE),
    BOX(-200, 330, 3970, 400, 30, 30, MAT_GLOW),
    BOX(-260, 0, 3960, 80, 340, 40, MAT_HAZARD),
    BOX(180, 0, 3960, 80, 340, 40, MAT_HAZARD),
    /* la torreta de las defensas, junto a la puerta */
    TURRET(900, 3000, F_DEFENCES_OFF),
    /* el almacén */
    PYR(2200, 0, 4200, 1600, 600, 1600, MAT_METAL),
    /* la antena */
    BOX(1800, 0, 3400, 50, 2600, 50, MAT_METAL),
    BOX(1650, 1800, 3415, 350, 30, 20, MAT_METAL),
    BOX(1780, 2600, 3380, 90, 90, 90, MAT_GLOW),
    /* la planta de terraformación, con su puerta, que pide energía, y la
       tubería hasta control, alta para pasar por debajo */
    BOX(-5200, 0, 6000, 1800, 500, 1800, MAT_PANEL),
    LOCKED(-3400, 0, 6800, 20, 320, 300, 2, 0, F_POWER),
    BOX(-4600, 500, 6600, 600, 2400, 600, MAT_METAL),
    PYR(-4800, 2900, 6400, 1000, 800, 1000, MAT_ORANGE),
    BOX(-5600, 0, 6100, 300, 700, 300, MAT_CYAN),
    BOX(-5600, 0, 6900, 300, 700, 300, MAT_CYAN),
    BOX(-4300, 500, 6050, 300, 150, 300, MAT_HAZARD),
    BOX(-3400, 260, 4700, 1900, 60, 60, MAT_METAL),
    BOX(-2900, 0, 4710, 40, 260, 40, MAT_METAL),
    BOX(-2200, 0, 4710, 40, 260, 40, MAT_METAL),
    /* la plataforma, su rampa y lo que hay encima */
    WEDGE(1500, 0, 1200, 500, 200, 800, MAT_GRATE),
    BOX(1500, 0, 2000, 1200, 200, 900, MAT_METAL),
    BOX(1700, 200, 2200, 150, 150, 150, MAT_HAZARD),
    BOX(2100, 200, 2500, 200, 120, 150, MAT_ORANGE),
    /* la entrada de la mina, con su castillete y la puerta del ascensor, que
       pide energía */
    BOX(-3600, 0, -3200, 1200, 400, 1000, MAT_PANEL),
    BOX(-3200, 400, -2900, 300, 1400, 300, MAT_METAL),
    BOX(-3250, 1800, -2950, 400, 60, 400, MAT_HAZARD),
    LOCKED(-2400, 0, -2850, 20, 320, 300, 5, 0, F_POWER),
    /* cajas junto a control */
    BOX(-1100, 0, 3500, 150, 150, 150, MAT_ORANGE),
    BOX(-950, 0, 3550, 120, 240, 120, MAT_HAZARD),
    BOX(-1115, 150, 3515, 120, 120, 120, MAT_ORANGE),
    /* rocas */
    PYR(3000, 0, 500, 600, 350, 500, MAT_ROCK),
    PYR(-2500, 0, 1500, 900, 500, 800, MAT_ROCK),
    PYR(-3500, 0, -1000, 400, 250, 400, MAT_ROCK),
    PYR(4500, 0, 7000, 1200, 700, 1000, MAT_ROCK),
    PYR(500, 0, -3000, 700, 300, 600, MAT_ROCK),
    PYR(-1000, 0, 9000, 1500, 900, 1300, MAT_ROCK),
    BOX(3600, 0, 2600, 400, 120, 300, MAT_SAND),
    /* las montañas, a lo lejos */
    PYR(-30000, 0, 60000, 40000, 22000, 30000, MAT_ROCK),
    PYR(20000, 0, 70000, 50000, 30000, 35000, MAT_ROCK),
    PYR(60000, 0, 10000, 30000, 16000, 40000, MAT_ROCK),
    PYR(-70000, 0, -20000, 40000, 20000, 40000, MAT_ROCK),
    PYR(10000, 0, -80000, 60000, 26000, 30000, MAT_ROCK),
    /* dos criaturas */
    CREATURE(600, 1500, CR_STALK),
    CREATURE(-1800, 2500, CR_STALK),
    /* los habitáculos: el edificio, el tejado, la puerta y su luz, el
       invernadero y lo que derribaron las criaturas al entrar */
    BOX(4000, 0, -1000, 2600, 450, 2000, MAT_PANEL),
    PRISM(4000, 450, -1000, 2600, 250, 2000, MAT_METAL),
    DOOR(3980, 0, -150, 20, 320, 300, 3, 0),
    BOX(3970, 330, -200, 30, 30, 400, MAT_GLOW),
    BOX(6600, 0, -600, 1800, 350, 1600, MAT_CYAN),
    PYR(8300, 0, 100, 600, 300, 500, MAT_ROCK),
    /* lo que quedó fuera: bidones junto a los habitáculos y a la planta,
       cajas en la entrada de la mina y planchas caídas junto a control */
    BARREL(3720, 520, MAT_ORANGE),
    BARREL(3800, 610, MAT_HAZARD),
    FURN(3690, 0, 640, 90, 60, 60, MAT_ORANGE),
    BARREL(-3250, 7250, MAT_HAZARD),
    BARREL(-3170, 7330, MAT_ORANGE),
    FURN(-2300, 0, -3300, 150, 120, 150, MAT_ORANGE),
    FURN(-2280, 120, -3280, 100, 80, 100, MAT_METAL),
    DECO(-500, 0, 3300, 300, 8, 150, MAT_PANEL),
    DECO(-200, 0, 3350, 120, 12, 220, MAT_PANEL),
};

static const entry_t outside_entries[] = {
    { 0, 0, 0, 0 },                     /* la llegada */
    { 0, 3700, 512, 0 },                /* al salir de control */
    { 3800, 0, 768, 0 },                /* al salir de los habitáculos */
    { -3200, 6950, 256, 0 },            /* al salir de la planta */
    { -2150, -2700, 256, 0 },           /* al salir de la mina por la escalera */
};

/* --- Control y comunicaciones -------------------------------------------------- */

/* El vestíbulo (x -600..600) da a los generadores (al oeste), a seguridad (al
 * este) y a la sala de mando (al norte), y de ella se pasa a comunicaciones. */

static const uint8_t ctrl_pal[16][3] = {
    { 0, 0, 0 },
    { 30, 30, 34 }, { 54, 54, 60 }, { 84, 84, 92 }, { 120, 120, 128 },     /* el suelo */
    { 52, 76, 88 }, { 78, 108, 120 }, { 110, 144, 156 }, { 156, 184, 192 }, /* metal */
    { 130, 40, 20 }, { 220, 90, 40 },                                       /* las alarmas */
    { 20, 80, 60 }, { 90, 230, 140 },                                       /* las pantallas */
    { 14, 14, 18 },                                                         /* el techo */
    { 44, 46, 54 },                                                         /* el fondo */
    { 240, 250, 230 },                                                      /* la luz */
};

static const band_t inside_ceiling[] = {
    { 4, 14, 13, PAT_50 }, { 255, 13, 13, PAT_SOLID },
};
static const band_t inside_floor[] = {
    { 4, 14, 14, PAT_SOLID }, { 12, 14, 1, PAT_50 }, { 255, 1, 1, PAT_SOLID },
};

/* Un punto de recarga que solo funciona con las marcas need. */
#define CHARGER_NEEDS(x, z, need) \
    { SH_BOX, OBJ_SOLID | OBJ_CHARGE, -1, 0, x, 0, z, 300, 15, 300, \
      { MAT_METAL, MAT_GLOW, MAT_CYAN, MAT_CYAN, MAT_CYAN, MAT_CYAN }, need, 0, 0 }

static const piece_t ctrl[] = {
    /* las paredes de fuera, con la puerta al sur */
    BOX(-2460, 0, -60, 2310, 400, 60, MAT_PANEL),
    BOX(150, 0, -60, 2310, 400, 60, MAT_PANEL),
    BOX(-150, 320, -60, 300, 80, 60, MAT_PANEL),
    DOOR(-150, 0, -20, 300, 320, 20, 0, 1),
    BOX(-2460, 0, 0, 60, 400, 2800, MAT_PANEL),
    BOX(2400, 0, 0, 60, 400, 2800, MAT_PANEL),
    BOX(-2460, 0, 2800, 4920, 400, 60, MAT_PANEL),
    /* las del vestíbulo: a los generadores, a seguridad y a la sala de mando */
    BOX(-660, 0, 0, 60, 400, 400, MAT_PANEL),
    BOX(-660, 0, 700, 60, 400, 2100, MAT_PANEL),
    BOX(-660, 320, 400, 60, 80, 300, MAT_PANEL),
    BOX(600, 0, 0, 60, 400, 400, MAT_PANEL),
    BOX(600, 0, 700, 60, 400, 500, MAT_PANEL),
    BOX(600, 320, 400, 60, 80, 300, MAT_PANEL),
    BOX(-600, 0, 1200, 500, 400, 60, MAT_PANEL),
    BOX(100, 0, 1200, 2300, 400, 60, MAT_PANEL),
    BOX(-100, 320, 1200, 200, 80, 60, MAT_PANEL),
    /* la de comunicaciones, con el paso desde la sala de mando */
    BOX(1300, 0, 1260, 60, 400, 940, MAT_PANEL),
    BOX(1300, 0, 2500, 60, 400, 300, MAT_PANEL),
    BOX(1300, 320, 2200, 60, 80, 300, MAT_PANEL),
    /* el vestíbulo: el punto de recarga, sin energía, unas cajas y la luz */
    CHARGER_NEEDS(250, 150, F_POWER),
    BOX(-550, 0, 900, 300, 120, 250, MAT_METAL),
    LAMP(-380, 380, 500, 120, 20, 60, L_CEIL),
    LAMP(-60, 380, 500, 120, 20, 60, L_CEIL),
    LAMP(260, 380, 500, 120, 20, 60, L_CEIL),
    /* los generadores, los conductos por donde entraron las criaturas, su
       consola, un registro, las criaturas y la luz */
    BOX(-2200, 0, 300, 600, 300, 500, MAT_METAL),
    BOX(-2200, 300, 350, 600, 40, 400, MAT_HAZARD),
    BOX(-2200, 0, 1150, 600, 300, 500, MAT_METAL),
    BOX(-2200, 300, 1200, 600, 40, 400, MAT_HAZARD),
    BOX(-2200, 0, 2000, 600, 300, 500, MAT_METAL),
    BOX(-2200, 300, 2050, 600, 40, 400, MAT_HAZARD),
    BOX(-1300, 330, 0, 300, 70, 2800, MAT_GRATE),
    BOX(-1250, 260, 2700, 200, 70, 100, MAT_DARK),
    TERM_S(-1225, 1300, 200, 0, F_POWER),
    LOG(-1500, 0, 2650, 300, 200, 150, 6, 0),
    CREATURE(-1150, 900, CR_AMBUSH),
    CREATURE(-1150, 2300, CR_AMBUSH),
    LAMP(-1700, 380, 1300, 400, 20, 400, L_CEIL),
    /* seguridad: la mesa y sus pantallas, la consola de las defensas, que
       pide energía, la taquilla, un registro y la luz */
    BOX(1800, 0, 100, 500, 90, 200, MAT_METAL),
    BOX(1850, 90, 250, 400, 120, 30, MAT_CYAN),
    TERM_W(2300, 500, 300, F_POWER, F_DEFENCES_OFF),
    BOX(700, 0, 0, 200, 250, 250, MAT_HAZARD),
    LOG(800, 0, 1000, 300, 200, 150, 7, 0),
    LAMP(1300, 380, 500, 600, 20, 200, L_CEIL),
    /* la sala de mando: la mesa con la segunda pieza, las consolas, un
       registro, la torreta que la guarda y la luz */
    BOX(100, 0, 1800, 600, 100, 400, MAT_METAL),
    BOX(200, 100, 1900, 400, 30, 200, MAT_CYAN),
    ITEM(370, 120, 1800, F_PART2),
    BOX(-600, 0, 1400, 150, 150, 1200, MAT_PANEL),
    BOX(-600, 150, 1450, 40, 150, 1100, MAT_CYAN),
    LOG(500, 0, 2650, 300, 200, 150, 9, F_TRUTH),
    TURRET(300, 2450, F_DEFENCES_OFF),
    LAMP(0, 380, 2000, 600, 20, 300, L_CEIL),
    /* comunicaciones: los armarios de la radio, la mesa, un registro, el
       cable de la antena y la luz */
    BOX(2200, 0, 1400, 200, 300, 1000, MAT_PANEL),
    BOX(1500, 0, 2600, 700, 120, 200, MAT_METAL),
    LOG(1600, 0, 1260, 300, 200, 150, 8, F_TRUTH),
    BOX(2250, 0, 2600, 100, 400, 100, MAT_METAL),
    LAMP(1700, 380, 1800, 400, 20, 400, L_CEIL),
    /* las balizas de emergencia, una en cada sala */
    BEACON(400, 330, 1140),
    BEACON(-2390, 330, 800),
    BEACON(2340, 330, 1000),
    BEACON(-590, 330, 2700),
    BEACON(2340, 330, 2480),
    /* los generadores: la rejilla del conducto, arrancada, y los cables que
       cuelgan de él */
    DECO(-1320, 0, 2420, 220, 8, 130, MAT_GRATE),
    DECO(-1180, 170, 1500, 8, 160, 8, MAT_METAL),
    DECO(-1150, 210, 1560, 8, 120, 8, MAT_METAL),
    DECO(-1240, 140, 2100, 8, 190, 8, MAT_METAL),
    /* la sala de mando: sillas junto a la mesa y pantallas en la pared */
    CHAIR_W(30, 1880),
    CHAIR_W(30, 2080),
    DECO(-450, 170, 2790, 350, 120, 10, MAT_CYAN),
    DECO(850, 170, 2790, 350, 120, 10, MAT_CYAN),
    /* seguridad: el armero, abierto y vacío */
    FURN(1500, 0, 1170, 300, 220, 30, MAT_METAL),
    FURN(1500, 0, 1080, 20, 220, 90, MAT_METAL),
    FURN(1780, 0, 1080, 20, 220, 90, MAT_METAL),
    DECO(1500, 200, 1080, 300, 20, 90, MAT_METAL),
    DECO(1520, 120, 1150, 260, 10, 20, MAT_GRATE),
    DECO(1490, 0, 950, 10, 220, 130, MAT_METAL),
};

static const entry_t ctrl_entries[] = {
    { 0, 300, 0, 0 },                   /* al entrar desde fuera */
};

/* --- Los habitáculos ------------------------------------------------------------ */

/* Las dos zonas comparten las coordenadas: el vestíbulo (x -300..300) y el
 * comedor (al oeste) dan al pasillo (z 1200..1600), con los dormitorios y la
 * enfermería al norte; al este del pasillo, el invernadero, y al sur del
 * pasillo, el taller, al que solo se llega por el invernadero. */

static const uint8_t hab_pal[16][3] = {
    { 0, 0, 0 },
    { 40, 34, 30 }, { 70, 60, 52 }, { 104, 90, 78 }, { 144, 128, 112 },     /* el suelo */
    { 60, 70, 80 }, { 90, 102, 114 }, { 126, 140, 152 }, { 172, 184, 194 },  /* metal */
    { 150, 70, 30 }, { 230, 130, 60 },                                       /* mantas y cajas */
    { 20, 90, 90 }, { 90, 230, 140 },                                        /* las pantallas */
    { 22, 20, 20 },                                                          /* el techo */
    { 52, 48, 46 },                                                          /* el fondo */
    { 250, 240, 215 },                                                       /* la luz */
};

static const uint8_t green_pal[16][3] = {
    { 0, 0, 0 },
    { 40, 34, 30 }, { 70, 60, 52 }, { 104, 90, 78 }, { 144, 128, 112 },     /* el suelo */
    { 60, 70, 80 }, { 90, 102, 114 }, { 126, 140, 152 }, { 172, 184, 194 },  /* metal */
    { 150, 70, 30 }, { 230, 130, 60 },                                       /* cajas y bombas */
    { 30, 90, 30 }, { 110, 200, 70 },                                        /* los cultivos */
    { 20, 24, 36 },                                                          /* el techo de cristal */
    { 60, 70, 64 },                                                          /* el fondo */
    { 230, 250, 220 },                                                       /* la luz */
};

static const piece_t hab[] = {
    /* las paredes de fuera, con la puerta al sur y la del invernadero al este */
    BOX(-1560, 0, -60, 1410, 350, 60, MAT_PANEL),
    BOX(150, 0, -60, 210, 350, 60, MAT_PANEL),
    BOX(-150, 320, -60, 300, 30, 60, MAT_PANEL),
    DOOR(-150, 0, -20, 300, 320, 20, 0, 2),
    BOX(-1560, 0, 0, 60, 350, 2800, MAT_PANEL),
    BOX(-1560, 0, 2800, 3120, 350, 60, MAT_PANEL),
    BOX(1500, 0, 1550, 60, 350, 1250, MAT_PANEL),
    BOX(1500, 0, 1140, 60, 350, 110, MAT_PANEL),
    BOX(1500, 320, 1250, 60, 30, 300, MAT_PANEL),
    DOOR(1480, 0, 1250, 20, 320, 300, 4, 0),
    /* el vestíbulo: su pared del este, la del comedor, con el paso, y la del
       pasillo, con la barricada */
    BOX(300, 0, 0, 60, 350, 1140, MAT_PANEL),
    BOX(-360, 0, 0, 60, 350, 300, MAT_PANEL),
    BOX(-360, 0, 600, 60, 350, 540, MAT_PANEL),
    BOX(-360, 300, 300, 60, 50, 300, MAT_PANEL),
    BOX(-300, 0, 1140, 150, 350, 60, MAT_PANEL),
    BOX(150, 0, 1140, 150, 350, 60, MAT_PANEL),
    BOX(-150, 300, 1140, 300, 50, 60, MAT_PANEL),
    BOX(-160, 0, 1080, 160, 140, 180, MAT_ORANGE),
    BOX(0, 0, 1090, 170, 190, 150, MAT_HAZARD),
    BOX(-140, 140, 1100, 260, 50, 90, MAT_METAL),
    BOX(-60, 190, 1110, 120, 110, 70, MAT_ORANGE),
    CHARGER(-150, 500),
    BOX(220, 0, 150, 80, 220, 300, MAT_METAL),
    /* el comedor: la pared del pasillo, las mesas y los bancos, la tarjeta,
       un registro y el dispensador */
    BOX(-1500, 0, 1140, 200, 350, 60, MAT_PANEL),
    BOX(-1000, 0, 1140, 640, 350, 60, MAT_PANEL),
    BOX(-1300, 300, 1140, 300, 50, 60, MAT_PANEL),
    BOX(-1300, 0, 300, 700, 100, 160, MAT_METAL),
    BOX(-1300, 0, 700, 700, 100, 160, MAT_METAL),
    BOX(-1300, 0, 220, 700, 45, 60, MAT_GRATE),
    BOX(-1300, 0, 880, 700, 45, 60, MAT_GRATE),
    ITEM(-900, 120, 300, F_CARD),
    LOG(-1500, 0, 900, 100, 200, 200, 2, 0),
    BOX(-1500, 0, 100, 120, 250, 300, MAT_PANEL),
    BOX(-1100, 330, 500, 400, 20, 200, MAT_GLOW),
    /* el pasillo: la pared de los dormitorios y la enfermería, su luz y, al
       sur, la puerta del taller, tapiada */
    BOX(-1500, 0, 1600, 700, 350, 60, MAT_PANEL),
    BOX(-600, 0, 1600, 1200, 350, 60, MAT_PANEL),
    BOX(800, 0, 1600, 700, 350, 60, MAT_PANEL),
    BOX(-800, 300, 1600, 200, 50, 60, MAT_PANEL),
    BOX(600, 300, 1600, 200, 50, 60, MAT_PANEL),
    BOX(-1350, 330, 1380, 150, 20, 40, MAT_GLOW),
    BOX(-950, 330, 1380, 150, 20, 40, MAT_GLOW),
    LAMP(-550, 330, 1380, 150, 20, 40, L_FLICKER),
    BOX(-150, 330, 1380, 150, 20, 40, MAT_GLOW),
    BOX(250, 330, 1380, 150, 20, 40, MAT_GLOW),
    BOX(650, 330, 1380, 150, 20, 40, MAT_GLOW),
    BOX(1050, 330, 1380, 150, 20, 40, MAT_GLOW),
    BOX(300, 0, 1140, 500, 350, 60, MAT_PANEL),
    BOX(1000, 0, 1140, 500, 350, 60, MAT_PANEL),
    BOX(800, 260, 1140, 200, 90, 60, MAT_PANEL),
    BOX(800, 0, 1150, 200, 260, 40, MAT_DARK),
    BOX(780, 0, 1200, 120, 120, 100, MAT_ORANGE),
    BOX(920, 0, 1210, 100, 160, 90, MAT_HAZARD),
    BOX(800, 120, 1205, 180, 40, 80, MAT_METAL),
    /* los dormitorios: las literas, las taquillas, dos registros y la luz */
    BOX(-30, 0, 1660, 60, 350, 1140, MAT_PANEL),
    BOX(-1500, 0, 1750, 220, 50, 450, MAT_METAL),
    BOX(-1500, 160, 1750, 220, 30, 450, MAT_ORANGE),
    BOX(-1500, 0, 2300, 220, 50, 450, MAT_METAL),
    BOX(-1500, 160, 2300, 220, 30, 450, MAT_ORANGE),
    BOX(-900, 0, 2580, 450, 50, 220, MAT_METAL),
    BOX(-900, 160, 2580, 450, 30, 220, MAT_ORANGE),
    BOX(-400, 0, 2700, 300, 220, 100, MAT_METAL),
    LOG(-130, 0, 2000, 100, 200, 200, 0, 0),
    LOG(-1500, 0, 2200, 100, 200, 100, 5, F_CODE),
    BOX(-900, 330, 2100, 400, 20, 200, MAT_GLOW),
    /* la enfermería: las camas, el armario, el filtro en una mesa, un
       registro, el escáner y la luz */
    BOX(1250, 0, 1800, 250, 70, 200, MAT_METAL),
    BOX(1250, 0, 2200, 250, 70, 200, MAT_METAL),
    BOX(400, 0, 2700, 500, 200, 100, MAT_PANEL),
    BOX(700, 0, 2200, 200, 80, 200, MAT_METAL),
    ITEM(705, 100, 2205, F_FILTER),
    LOG(30, 0, 2400, 100, 200, 200, 4, 0),
    BOX(1300, 0, 2550, 200, 280, 200, MAT_CYAN),
    BOX(500, 330, 2100, 400, 20, 200, MAT_GLOW),
    /* el vestíbulo: las taquillas de los trajes */
    FURN(-300, 0, 650, 70, 230, 120, MAT_METAL),
    FURN(-300, 0, 790, 70, 230, 120, MAT_METAL),
    FURN(-300, 0, 930, 70, 230, 120, MAT_METAL),
    /* el comedor: una mesa volcada, sillas tiradas y bandejas */
    FURN(-640, 0, 30, 240, 120, 20, MAT_GRATE),
    DECO(-630, 15, 50, 15, 15, 70, MAT_GRATE),
    DECO(-415, 15, 50, 15, 15, 70, MAT_GRATE),
    DECO(-630, 95, 50, 15, 15, 70, MAT_GRATE),
    DECO(-415, 95, 50, 15, 15, 70, MAT_GRATE),
    DECO(-800, 0, 60, 45, 8, 50, MAT_GRATE),
    DECO(-800, 0, 110, 45, 45, 8, MAT_GRATE),
    DECO(-560, 0, 160, 50, 8, 45, MAT_GRATE),
    DECO(-510, 0, 160, 8, 45, 45, MAT_GRATE),
    DECO(-1200, 100, 350, 60, 4, 40, MAT_METAL),
    DECO(-700, 100, 380, 60, 4, 40, MAT_METAL),
    DECO(-1000, 0, 520, 60, 4, 40, MAT_METAL),
    /* los dormitorios: un colchón y mantas por el suelo y una taquilla abierta */
    DECO(-1150, 0, 1950, 200, 15, 400, MAT_GRATE),
    DECO(-1100, 15, 2050, 150, 12, 180, MAT_ORANGE),
    DECO(-700, 0, 2250, 120, 25, 90, MAT_ORANGE),
    DECO(-410, 0, 2560, 10, 220, 140, MAT_METAL),
    DECO(-330, 0, 2560, 80, 20, 60, MAT_ORANGE),
    /* la enfermería: el gotero junto a una cama y un carro volcado */
    DECO(1225, 0, 1880, 10, 190, 10, MAT_METAL),
    DECO(1210, 175, 1875, 40, 45, 20, MAT_CYAN),
    FURN(1000, 0, 1700, 90, 60, 160, MAT_METAL),
    DECO(990, 10, 1720, 10, 25, 25, MAT_GRATE),
    DECO(990, 10, 1820, 10, 25, 25, MAT_GRATE),
    DECO(1100, 0, 1750, 40, 20, 30, MAT_CYAN),
    DECO(1130, 0, 1820, 30, 15, 40, MAT_ORANGE),
};

static const entry_t hab_entries[] = {
    { 0, 200, 0, 0 },                   /* al entrar desde fuera */
    { 1350, 1400, 768, 0 },             /* al volver del invernadero */
};

static const piece_t green[] = {
    /* las paredes de fuera, con el hueco que abrieron las criaturas al este,
       tapado por los escombros */
    BOX(300, 0, -60, 3720, 350, 60, MAT_PANEL),
    BOX(3960, 0, 0, 60, 350, 1800, MAT_PANEL),
    BOX(3960, 0, 2200, 60, 350, 600, MAT_PANEL),
    BOX(1500, 0, 2800, 2520, 350, 60, MAT_PANEL),
    BOX(3940, 0, 1800, 120, 160, 180, MAT_ROCK),
    BOX(3920, 0, 1980, 140, 230, 220, MAT_ROCK),
    BOX(3950, 160, 1820, 100, 90, 140, MAT_ROCK),
    /* la pared del pasillo, con su puerta, y la del taller, con el paso */
    BOX(1500, 0, 1550, 60, 350, 1250, MAT_PANEL),
    BOX(1500, 0, 1140, 60, 350, 110, MAT_PANEL),
    BOX(1500, 320, 1250, 60, 30, 300, MAT_PANEL),
    DOOR(1560, 0, 1250, 20, 320, 300, 3, 1),
    BOX(1500, 0, 0, 60, 350, 300, MAT_PANEL),
    BOX(1500, 0, 600, 60, 350, 540, MAT_PANEL),
    BOX(1500, 300, 300, 60, 50, 300, MAT_PANEL),
    /* los cultivos, las bombas, un registro junto al hueco y las luces */
    BOX(1900, 0, 400, 1700, 80, 300, MAT_GRATE),
    BOX(1950, 80, 450, 1600, 60, 200, MAT_CYAN),
    BOX(1900, 0, 1000, 1700, 80, 300, MAT_GRATE),
    BOX(1950, 80, 1050, 1600, 60, 200, MAT_CYAN),
    BOX(1900, 0, 1700, 1700, 80, 300, MAT_GRATE),
    BOX(1950, 80, 1750, 900, 60, 200, MAT_CYAN),
    BOX(2850, 80, 1750, 700, 30, 200, MAT_SAND),
    BOX(1900, 0, 2300, 1700, 80, 300, MAT_GRATE),
    BOX(1950, 80, 2350, 1600, 60, 200, MAT_CYAN),
    BOX(3700, 0, 150, 200, 250, 200, MAT_HAZARD),
    LOG(3860, 0, 1350, 100, 200, 300, 1, 0),
    BOX(1750, 330, 800, 150, 20, 40, MAT_GLOW),
    BOX(2150, 330, 800, 150, 20, 40, MAT_GLOW),
    BOX(2550, 330, 800, 150, 20, 40, MAT_GLOW),
    BOX(2950, 330, 800, 150, 20, 40, MAT_GLOW),
    BOX(3350, 330, 800, 150, 20, 40, MAT_GLOW),
    BOX(1750, 330, 2050, 150, 20, 40, MAT_GLOW),
    BOX(2150, 330, 2050, 150, 20, 40, MAT_GLOW),
    BOX(2550, 330, 2050, 150, 20, 40, MAT_GLOW),
    BOX(2950, 330, 2050, 150, 20, 40, MAT_GLOW),
    BOX(3350, 330, 2050, 150, 20, 40, MAT_GLOW),
    /* el taller: sus paredes, con la puerta tapiada del pasillo y lo que
       la tapa por dentro */
    BOX(300, 0, 0, 60, 350, 1200, MAT_PANEL),
    BOX(360, 0, 1140, 440, 350, 60, MAT_PANEL),
    BOX(1000, 0, 1140, 500, 350, 60, MAT_PANEL),
    BOX(800, 260, 1140, 200, 90, 60, MAT_PANEL),
    BOX(800, 0, 1140, 200, 260, 40, MAT_DARK),
    BOX(780, 0, 1020, 150, 180, 120, MAT_ORANGE),
    BOX(930, 0, 1040, 100, 120, 100, MAT_HAZARD),
    /* el banco, la estantería con la pieza, la perforadora, un registro y la luz */
    BOX(400, 0, 150, 600, 90, 200, MAT_METAL),
    BOX(360, 0, 600, 150, 120, 400, MAT_GRATE),
    ITEM(470, 130, 750, F_PART1),
    BOX(900, 0, 500, 300, 200, 300, MAT_ORANGE),
    PYR(900, 200, 500, 300, 100, 300, MAT_HAZARD),
    LOG(1100, 0, 0, 200, 200, 100, 3, 0),
    BOX(600, 330, 500, 600, 20, 100, MAT_GLOW),
    /* el invernadero: macetas volcadas, la tierra por el suelo y una manguera */
    FURN(3650, 0, 2500, 50, 50, 50, MAT_ORANGE),
    FURN(3720, 0, 2560, 50, 50, 50, MAT_ORANGE),
    FURN(3780, 0, 2450, 50, 40, 60, MAT_ORANGE),
    DECO(3800, 0, 2380, 90, 6, 70, MAT_SAND),
    DECO(3640, 0, 2600, 70, 6, 90, MAT_SAND),
    DECO(1600, 0, 700, 15, 10, 600, MAT_GRATE),
    DECO(1600, 0, 690, 300, 10, 15, MAT_GRATE),
    /* el taller: bidones en un rincón y herramientas en el banco */
    BARREL(1260, 1000, MAT_HAZARD),
    BARREL(1340, 1070, MAT_ORANGE),
    BARREL(1420, 980, MAT_ORANGE),
    DECO(500, 90, 200, 120, 40, 50, MAT_HAZARD),
    DECO(700, 90, 250, 80, 6, 15, MAT_METAL),
    DECO(820, 90, 180, 15, 6, 70, MAT_METAL),
};

static const entry_t green_entries[] = {
    { 1700, 1400, 256, 0 },             /* al entrar desde el pasillo */
};

/* --- La mina ----------------------------------------------------------------------- */

/* Los tres niveles, cada uno una zona: se llega a cada uno por el sur, en la
 * jaula del ascensor o bajando por el pozo. */

static const uint8_t mine_pal[16][3] = {
    { 0, 0, 0 },
    { 46, 38, 32 }, { 80, 66, 54 }, { 118, 98, 78 }, { 160, 136, 110 },     /* roca */
    { 50, 58, 66 }, { 80, 90, 100 }, { 116, 128, 138 }, { 160, 170, 178 },   /* metal */
    { 150, 70, 20 }, { 230, 140, 40 },                                       /* vagonetas y avisos */
    { 20, 90, 100 }, { 90, 230, 140 },                                       /* pantallas */
    { 16, 12, 10 },                                                          /* el techo */
    { 50, 42, 36 },                                                          /* el fondo */
    { 250, 230, 180 },                                                       /* la luz */
};

static const uint8_t mine2_pal[16][3] = {
    { 0, 0, 0 },
    { 42, 42, 30 }, { 74, 72, 50 }, { 108, 106, 72 }, { 148, 146, 100 },     /* roca */
    { 50, 60, 56 }, { 80, 94, 86 }, { 116, 132, 120 }, { 160, 174, 160 },    /* metal */
    { 150, 70, 20 }, { 230, 140, 40 },                                       /* vagonetas y avisos */
    { 20, 90, 100 }, { 90, 230, 140 },                                       /* cian y verde */
    { 14, 18, 10 },                                                          /* el techo */
    { 62, 74, 38 },                                                          /* el gas */
    { 240, 240, 190 },                                                       /* la luz */
};

static const uint8_t nest_pal[16][3] = {
    { 0, 0, 0 },
    { 50, 24, 28 }, { 90, 40, 44 }, { 130, 62, 60 }, { 176, 92, 80 },       /* roca caliente */
    { 50, 52, 58 }, { 80, 82, 90 }, { 116, 118, 126 }, { 160, 162, 170 },    /* metal */
    { 180, 60, 20 }, { 255, 150, 50 },                                       /* lo que brilla en el nido */
    { 20, 100, 110 }, { 90, 230, 140 },                                      /* cian y verde */
    { 20, 8, 12 },                                                           /* el techo */
    { 70, 40, 60 },                                                          /* el gas */
    { 255, 240, 200 },                                                       /* la luz */
};

/* La entrada del pozo de ventilación: una rejilla en el suelo, o la escalera
 * para subir; se cruzan andando hacia ellas. */
#define SHAFT(x, y, z, sx, sy, sz, zone, entry) \
    { SH_BOX, OBJ_SOLID | OBJ_DOOR, zone, entry, x, y, z, sx, sy, sz, ALL(MAT_GRATE), 0, 0, 0 }
/* Lo que crece en el nido: brilla. */
#define POD(x, z, s) \
    { SH_OCTA, OBJ_SOLID, -1, 0, x, 0, z, s, s * 4 / 3, s, ALL(MAT_ORANGE), 0, 0, 0 }

static const piece_t mine1[] = {
    /* la jaula del ascensor, al sur, que baja al nivel 2, con sus postes */
    BOX(-360, 0, -60, 210, 300, 60, MAT_ROCK),
    BOX(150, 0, -60, 210, 300, 60, MAT_ROCK),
    BOX(-150, 260, -60, 300, 40, 60, MAT_METAL),
    DOOR(-150, 0, -20, 300, 260, 20, 6, 0),
    BOX(-190, 0, 0, 40, 260, 40, MAT_HAZARD),
    BOX(150, 0, 0, 40, 260, 40, MAT_HAZARD),
    /* la galería, con la escalera de emergencia al este y el cuarto de
       herramientas al oeste */
    BOX(300, 0, 0, 60, 300, 200, MAT_ROCK),
    BOX(300, 0, 500, 60, 300, 2560, MAT_ROCK),
    BOX(300, 260, 200, 60, 40, 300, MAT_ROCK),
    DOOR(280, 0, 200, 20, 260, 300, 0, 4),
    BOX(-360, 0, 0, 60, 300, 1500, MAT_ROCK),
    BOX(-360, 0, 2100, 60, 300, 960, MAT_ROCK),
    BOX(-360, 0, 3000, 720, 300, 60, MAT_ROCK),
    /* los marcos de la galería, las vagonetas, la perforadora y la luz */
    BOX(-300, 0, 900, 40, 300, 40, MAT_METAL),
    BOX(260, 0, 900, 40, 300, 40, MAT_METAL),
    BOX(-300, 260, 900, 600, 40, 40, MAT_METAL),
    BOX(-300, 0, 2600, 40, 300, 40, MAT_METAL),
    BOX(260, 0, 2600, 40, 300, 40, MAT_METAL),
    BOX(-300, 260, 2600, 600, 40, 40, MAT_METAL),
    BOX(-150, 0, 1200, 160, 140, 260, MAT_ORANGE),
    BOX(-150, 0, 2250, 160, 140, 260, MAT_ORANGE),
    BOX(-200, 0, 2750, 400, 180, 250, MAT_HAZARD),
    BOX(-50, 280, 460, 100, 20, 120, MAT_GLOW),
    BOX(-50, 280, 760, 100, 20, 120, MAT_GLOW),
    BOX(-50, 280, 1060, 100, 20, 120, MAT_GLOW),
    BOX(-50, 280, 1360, 100, 20, 120, MAT_GLOW),
    LAMP(-50, 280, 1660, 100, 20, 120, L_FLICKER),
    BOX(-50, 280, 1960, 100, 20, 120, MAT_GLOW),
    BOX(-50, 280, 2260, 100, 20, 120, MAT_GLOW),
    /* el cuarto de herramientas: sus paredes, el banco con el cortador, un
       registro, el punto de recarga, el armario y la luz */
    BOX(-1560, 0, 1440, 1200, 300, 60, MAT_ROCK),
    BOX(-1560, 0, 2100, 1200, 300, 60, MAT_ROCK),
    BOX(-1560, 0, 1500, 60, 300, 600, MAT_ROCK),
    BOX(-1400, 0, 1550, 600, 90, 150, MAT_METAL),
    ITEM(-850, 110, 1650, F_CUTTER),
    LOG(-1500, 0, 1800, 100, 200, 200, 10, 0),
    CHARGER(-1300, 1800),
    BOX(-900, 0, 2000, 400, 220, 100, MAT_PANEL),
    BOX(-1100, 280, 1700, 300, 20, 200, MAT_GLOW),
    /* los raíles, un puntal de madera, una vagoneta volcada con su carga y
       un pico tirado */
    DECO(-130, 0, 300, 10, 6, 2440, MAT_METAL),
    DECO(-20, 0, 300, 10, 6, 2440, MAT_METAL),
    FURN(-300, 0, 1300, 40, 300, 40, MAT_SAND),
    FURN(260, 0, 1300, 40, 300, 40, MAT_SAND),
    DECO(-300, 260, 1300, 600, 40, 40, MAT_SAND),
    FURN(160, 0, 1950, 140, 120, 220, MAT_ORANGE),
    STONES(60, 1980, 100),
    STONES(80, 2080, 70),
    DECO(120, 0, 1600, 90, 8, 10, MAT_SAND),
    DECO(120, 0, 1590, 10, 8, 40, MAT_METAL),
};

static const entry_t mine1_entries[] = {
    { 0, 300, 0, 0 },                   /* al llegar en el ascensor */
};

static const piece_t mine2[] = {
    /* la jaula del ascensor, que sube al nivel 1 */
    BOX(-360, 0, -60, 210, 300, 60, MAT_ROCK),
    BOX(150, 0, -60, 210, 300, 60, MAT_ROCK),
    BOX(-150, 260, -60, 300, 40, 60, MAT_METAL),
    DOOR(-150, 0, -20, 300, 260, 20, 5, 0),
    BOX(-190, 0, 0, 40, 260, 40, MAT_HAZARD),
    BOX(150, 0, 0, 40, 260, 40, MAT_HAZARD),
    /* la galería hacia el norte y, al final, la que va al este, hasta el
       frente de corte */
    BOX(-360, 0, 0, 60, 300, 2060, MAT_ROCK),
    BOX(300, 0, 0, 60, 300, 1400, MAT_ROCK),
    BOX(-300, 0, 2000, 3360, 300, 60, MAT_ROCK),
    BOX(360, 0, 1340, 2700, 300, 60, MAT_ROCK),
    BOX(3000, 0, 1400, 60, 300, 600, MAT_ROCK),
    /* los marcos, una vagoneta, un registro y la luz */
    BOX(-300, 0, 900, 40, 300, 40, MAT_METAL),
    BOX(260, 0, 900, 40, 300, 40, MAT_METAL),
    BOX(-300, 260, 900, 600, 40, 40, MAT_METAL),
    BOX(1200, 0, 1400, 40, 300, 40, MAT_METAL),
    BOX(1200, 0, 1960, 40, 300, 40, MAT_METAL),
    BOX(1200, 260, 1400, 40, 40, 600, MAT_METAL),
    BOX(-260, 0, 500, 200, 120, 300, MAT_ORANGE),
    LOG(-300, 0, 1100, 100, 200, 200, 11, 0),
    BOX(-50, 280, 360, 100, 20, 120, MAT_GLOW),
    BOX(-50, 280, 660, 100, 20, 120, MAT_GLOW),
    BOX(-50, 280, 960, 100, 20, 120, MAT_GLOW),
    LAMP(-50, 280, 1260, 100, 20, 120, L_FLICKER),
    BOX(-50, 280, 1560, 100, 20, 120, MAT_GLOW),
    BOX(400, 280, 1650, 120, 20, 100, MAT_GLOW),
    BOX(700, 280, 1650, 120, 20, 100, MAT_GLOW),
    BOX(1000, 280, 1650, 120, 20, 100, MAT_GLOW),
    BOX(1300, 280, 1650, 120, 20, 100, MAT_GLOW),
    BOX(1600, 280, 1650, 120, 20, 100, MAT_GLOW),
    BOX(1900, 280, 1650, 120, 20, 100, MAT_GLOW),
    /* el frente: la cortadora y el pozo de ventilación, abierto, que baja
       al nido */
    BOX(2750, 0, 1450, 250, 200, 250, MAT_HAZARD),
    SHAFT(2200, 0, 1550, 300, 60, 300, 7, 0),
    /* una criatura que persigue */
    CREATURE(1500, 1700, CR_CHASE),
    /* el registro del primero que bajó al nido */
    LOG(1600, 0, 1880, 200, 200, 100, 16, 0),
    /* los raíles, un puntal y un pico junto al frente */
    DECO(-130, 0, 300, 10, 6, 1700, MAT_METAL),
    DECO(-20, 0, 300, 10, 6, 1700, MAT_METAL),
    DECO(360, 0, 1640, 1800, 6, 10, MAT_METAL),
    DECO(360, 0, 1750, 1800, 6, 10, MAT_METAL),
    FURN(600, 0, 1400, 40, 300, 40, MAT_SAND),
    FURN(600, 0, 1960, 40, 300, 40, MAT_SAND),
    DECO(600, 260, 1400, 40, 40, 600, MAT_SAND),
    DECO(2550, 0, 1500, 90, 8, 10, MAT_SAND),
    DECO(2630, 0, 1490, 10, 8, 40, MAT_METAL),
};

static const entry_t mine2_entries[] = {
    { 0, 300, 0, 0 },                   /* al llegar en el ascensor */
    { 1950, 1700, 768, 0 },             /* al subir del nido por el pozo */
};

static const piece_t nest[] = {
    /* la galería desde el pozo, con la escalera para subir */
    BOX(-360, 0, -60, 720, 400, 60, MAT_ROCK),
    SHAFT(-100, 0, -20, 200, 400, 20, 6, 1),
    BOX(-360, 0, 0, 60, 400, 1200, MAT_ROCK),
    BOX(300, 0, 0, 60, 400, 1200, MAT_ROCK),
    /* la cueva */
    BOX(-2060, 0, 1140, 1760, 400, 60, MAT_ROCK),
    BOX(300, 0, 1140, 1760, 400, 60, MAT_ROCK),
    BOX(-2060, 0, 1200, 60, 400, 2860, MAT_ROCK),
    BOX(2000, 0, 1200, 60, 400, 2860, MAT_ROCK),
    BOX(-2060, 0, 4000, 4120, 400, 60, MAT_ROCK),
    /* las rocas y lo que crece en el nido */
    PYR(-1700, 0, 1900, 600, 380, 500, MAT_ROCK),
    PYR(-600, 0, 2600, 500, 300, 400, MAT_ROCK),
    PYR(900, 0, 1700, 500, 340, 600, MAT_ROCK),
    PYR(-1500, 0, 3300, 700, 400, 600, MAT_ROCK),
    PYR(400, 0, 3000, 400, 260, 400, MAT_ROCK),
    POD(-900, 1700, 120),
    POD(-300, 2000, 90),
    POD(-1200, 2700, 150),
    POD(200, 2400, 110),
    POD(1400, 2600, 140),
    POD(-1000, 3100, 120),
    /* al fondo, las cajas de las muestras, la pieza y un registro */
    BOX(1300, 0, 3500, 200, 150, 200, MAT_METAL),
    BOX(1550, 0, 3550, 200, 150, 200, MAT_METAL),
    BOX(1350, 150, 3550, 150, 100, 150, MAT_METAL),
    ITEM(1500, 60, 3350, F_PART3),
    LOG(1800, 0, 3800, 200, 200, 150, 12, 0),
    /* las criaturas y, al fondo, la incubadora */
    CREATURE(-1000, 2200, CR_AMBUSH),
    CREATURE(500, 2200, CR_CHASE),
    CREATURE(1100, 3000, CR_CHASE),
    MOTHER(-320, 3850),
    SPIT,
    /* lo que perdieron los mineros: un casco, una linterna y una bolsa */
    DECO(120, 0, 700, 50, 25, 40, MAT_HAZARD),
    DECO(135, 10, 690, 20, 10, 10, MAT_GLOW),
    DECO(-250, 0, 1500, 40, 12, 12, MAT_METAL),
    DECO(-210, 0, 1500, 6, 12, 12, MAT_GLOW),
    DECO(700, 0, 1350, 60, 30, 40, MAT_SAND),
};

static const entry_t nest_entries[] = {
    { 0, 300, 0, 0 },                   /* al bajar por el pozo */
};

/* --- La planta de terraformación ---------------------------------------------- */

/* El vestíbulo (x -600..600) da a la sala de control (al oeste), a los
 * depósitos (al este) y a la nave de los procesadores (al norte), con la
 * puerta soldada al fondo. */

static const uint8_t plant_pal[16][3] = {
    { 0, 0, 0 },
    { 38, 36, 26 }, { 66, 62, 44 }, { 100, 94, 66 }, { 140, 132, 94 },     /* el suelo */
    { 56, 64, 56 }, { 86, 98, 84 }, { 122, 138, 116 }, { 166, 182, 156 },   /* metal */
    { 170, 70, 20 }, { 250, 150, 40 },                                      /* los avisos */
    { 20, 90, 100 }, { 90, 230, 140 },                                      /* depósitos y pantallas */
    { 16, 18, 8 },                                                          /* el techo */
    { 80, 88, 40 },                                                         /* el gas */
    { 250, 245, 200 },                                                      /* la luz */
};

static const piece_t plant[] = {
    /* las paredes de fuera, con la puerta al sur y la soldada al norte */
    BOX(-2460, 0, -60, 2310, 500, 60, MAT_PANEL),
    BOX(150, 0, -60, 2310, 500, 60, MAT_PANEL),
    BOX(-150, 320, -60, 300, 180, 60, MAT_PANEL),
    DOOR(-150, 0, -20, 300, 320, 20, 0, 3),
    BOX(-2460, 0, 0, 60, 500, 4060, MAT_PANEL),
    BOX(2400, 0, 0, 60, 500, 4060, MAT_PANEL),
    BOX(-2460, 0, 4000, 2310, 500, 60, MAT_PANEL),
    BOX(150, 0, 4000, 2310, 500, 60, MAT_PANEL),
    BOX(-150, 320, 4000, 300, 180, 60, MAT_PANEL),
    LOCKED(-150, 0, 3980, 300, 320, 20, 8, 0, F_CUTTER),
    /* las del vestíbulo: a la sala de control, a los depósitos y a la nave */
    BOX(-660, 0, 0, 60, 500, 300, MAT_PANEL),
    BOX(-660, 0, 600, 60, 500, 800, MAT_PANEL),
    BOX(-660, 320, 300, 60, 180, 300, MAT_PANEL),
    BOX(600, 0, 0, 60, 500, 300, MAT_PANEL),
    BOX(600, 0, 600, 60, 500, 800, MAT_PANEL),
    BOX(600, 320, 300, 60, 180, 300, MAT_PANEL),
    BOX(-2400, 0, 1400, 2200, 500, 60, MAT_PANEL),
    BOX(200, 0, 1400, 2200, 500, 60, MAT_PANEL),
    BOX(-200, 320, 1400, 400, 180, 60, MAT_PANEL),
    /* el vestíbulo: el punto de recarga, unas cajas y la luz */
    CHARGER_NEEDS(200, 100, F_POWER),
    BOX(-550, 0, 1000, 250, 150, 250, MAT_ORANGE),
    BOX(-200, 480, 500, 400, 20, 200, MAT_GLOW),
    /* la sala de control: las pantallas, la consola que para la planta (con
       las tres bombas cerradas), un registro y la luz */
    BOX(-2300, 0, 200, 200, 100, 1000, MAT_METAL),
    BOX(-2300, 100, 250, 40, 200, 900, MAT_CYAN),
    TERM_S(-1400, 1150, 300, F_PUMPS, F_PLANT_OFF),
    LOG(-1000, 0, 100, 250, 200, 120, 13, 0),
    BOX(-1700, 480, 600, 600, 20, 200, MAT_GLOW),
    /* los depósitos, la primera bomba, las tuberías y la luz */
    BOX(1700, 0, 150, 500, 400, 500, MAT_CYAN),
    BOX(1700, 0, 800, 500, 400, 500, MAT_CYAN),
    INJECTOR(900, 1000, F_PUMP1),
    DECO(1150, 60, 1105, 550, 40, 40, MAT_METAL),
    BOX(700, 350, 700, 1700, 40, 40, MAT_METAL),
    BOX(1100, 480, 600, 600, 20, 200, MAT_GLOW),
    /* la nave: los procesadores con sus luces de aviso, la columna de
       inyección (a un lado, fuera del camino a la puerta soldada), las otras
       dos bombas, un registro, la luz y dos criaturas */
    { SH_BOX, OBJ_SOLID, -1, 0, -1800, 0, 2200, 800, 450, 900,
      { MAT_METAL, MAT_GRATE, MAT_PANEL, MAT_PANEL, MAT_PANEL, MAT_PANEL }, 0, 0, 0 },
    { SH_BOX, OBJ_SOLID, -1, 0, 1000, 0, 2200, 800, 450, 900,
      { MAT_METAL, MAT_GRATE, MAT_PANEL, MAT_PANEL, MAT_PANEL, MAT_PANEL }, 0, 0, 0 },
    WARN(-1500, 450, 2500, 200, 40, 200, F_PLANT_OFF),
    WARN(1300, 450, 2500, 200, 40, 200, F_PLANT_OFF),
    BOX(-600, 0, 2500, 200, 500, 200, MAT_METAL),
    INJECTOR(-2200, 3500, F_PUMP2),
    DECO(-2400, 60, 3605, 200, 40, 40, MAT_METAL),
    INJECTOR(1950, 3500, F_PUMP3),
    DECO(2200, 60, 3605, 200, 40, 40, MAT_METAL),
    LOG(2250, 0, 3000, 150, 200, 250, 14, 0),
    BOX(-900, 480, 2700, 120, 20, 100, MAT_GLOW),
    BOX(-500, 480, 2700, 120, 20, 100, MAT_GLOW),
    BOX(-100, 480, 2700, 120, 20, 100, MAT_GLOW),
    BOX(300, 480, 2700, 120, 20, 100, MAT_GLOW),
    BOX(700, 480, 2700, 120, 20, 100, MAT_GLOW),
    CREATURE(-200, 3300, CR_STALK),
    CREATURE(500, 1900, CR_STALK),
    /* la nave: las tuberías por el techo y las paredes, con sus válvulas, la
       pasarela del fondo y bidones */
    DECO(-2400, 440, 1460, 40, 40, 2540, MAT_METAL),
    DECO(2360, 440, 1460, 40, 40, 2540, MAT_METAL),
    DECO(-2400, 440, 1470, 4800, 40, 40, MAT_METAL),
    DECO(-2400, 0, 2000, 40, 440, 40, MAT_METAL),
    DECO(2360, 0, 1800, 40, 440, 40, MAT_METAL),
    DECO(-2360, 280, 1990, 20, 60, 60, MAT_HAZARD),
    DECO(2340, 280, 1790, 20, 60, 60, MAT_HAZARD),
    DECO(-2400, 300, 3820, 1700, 15, 160, MAT_GRATE),
    DECO(-2400, 390, 3820, 1700, 8, 8, MAT_METAL),
    DECO(-1500, 0, 3940, 20, 300, 20, MAT_METAL),
    DECO(-900, 0, 3940, 20, 300, 20, MAT_METAL),
    BARREL(2200, 1520, MAT_HAZARD),
    BARREL(2280, 1600, MAT_ORANGE),
    BARREL(2300, 1500, MAT_ORANGE),
    /* los depósitos: bidones junto a la pared */
    BARREL(2250, 60, MAT_HAZARD),
    BARREL(2320, 140, MAT_ORANGE),
    /* la sala de control: sillas ante las pantallas */
    CHAIR_E(-2020, 400),
    CHAIR_E(-2020, 800),
};

static const entry_t plant_entries[] = {
    { 0, 200, 0, 0 },                   /* al entrar desde fuera */
    { 0, 3850, 512, 0 },                /* al salir del cuarto sellado */
};

/* El cuarto donde se encerró el operario. */
static const piece_t sealed[] = {
    BOX(-460, 0, -60, 310, 300, 60, MAT_PANEL),
    BOX(150, 0, -60, 310, 300, 60, MAT_PANEL),
    BOX(-150, 260, -60, 300, 40, 60, MAT_PANEL),
    DOOR(-150, 0, -20, 300, 260, 20, 2, 1),
    BOX(-460, 0, 0, 60, 300, 860, MAT_PANEL),
    BOX(400, 0, 0, 60, 300, 860, MAT_PANEL),
    BOX(-400, 0, 800, 800, 300, 60, MAT_PANEL),
    /* el catre, el soldador, la cuarta pieza, su registro y la luz */
    BOX(-380, 0, 250, 180, 60, 400, MAT_METAL),
    BOX(250, 0, 600, 100, 120, 100, MAT_HAZARD),
    ITEM(-25, 60, 500, F_PART4),
    LOG(-100, 0, 700, 200, 200, 100, 15, 0),
    BOX(-100, 280, 300, 200, 20, 200, MAT_GLOW),
    /* lo que le dejó aguantar: latas vacías, garrafas y una manta */
    DECO(-170, 0, 300, 12, 15, 12, MAT_METAL),
    DECO(-140, 0, 345, 12, 15, 12, MAT_METAL),
    DECO(-185, 0, 390, 12, 15, 12, MAT_METAL),
    DECO(-90, 0, 250, 15, 12, 12, MAT_METAL),
    FURN(290, 0, 80, 30, 45, 30, MAT_CYAN),
    FURN(330, 0, 130, 30, 45, 30, MAT_CYAN),
    DECO(-370, 60, 300, 160, 12, 250, MAT_ORANGE),
    DECO(-340, 72, 380, 90, 15, 90, MAT_ORANGE),
};

static const entry_t sealed_entries[] = {
    { 0, 200, 0, 0 },                   /* al entrar por la puerta cortada */
};

const zone_t zones[] = {
    { "Exterior", "Outside", outside, sizeof outside / sizeof outside[0], outside_pal,
      outside_sky, outside_ground, outside_entries, 1, 0, 0 },
    { "Control", "Control", ctrl, sizeof ctrl / sizeof ctrl[0], ctrl_pal,
      inside_ceiling, inside_floor, ctrl_entries, 0, 0, F_POWER },
    { "Planta", "Plant", plant, sizeof plant / sizeof plant[0], plant_pal,
      inside_ceiling, inside_floor, plant_entries, 0, 3, 0 },
    { "Habitáculos", "Habitation", hab, sizeof hab / sizeof hab[0], hab_pal,
      inside_ceiling, inside_floor, hab_entries, 0, 0, 0 },
    { "Invernadero", "Greenhouse", green, sizeof green / sizeof green[0], green_pal,
      inside_ceiling, inside_floor, green_entries, 0, 0, 0 },
    { "Mina, nivel 1", "Mine, level 1", mine1, sizeof mine1 / sizeof mine1[0], mine_pal,
      inside_ceiling, inside_floor, mine1_entries, 0, 0, 0 },
    { "Mina, nivel 2", "Mine, level 2", mine2, sizeof mine2 / sizeof mine2[0], mine2_pal,
      inside_ceiling, inside_floor, mine2_entries, 0, 3, 0 },
    { "El nido", "The nest", nest, sizeof nest / sizeof nest[0], nest_pal,
      inside_ceiling, inside_floor, nest_entries, 0, 5, 0 },
    { "Cuarto sellado", "Sealed room", sealed, sizeof sealed / sizeof sealed[0], plant_pal,
      inside_ceiling, inside_floor, sealed_entries, 0, 0, 0 },
};
const int zone_count = sizeof zones / sizeof zones[0];
const int plant_zone = 2;

/* --- Las marcas y los registros ------------------------------------------------- */

#define PART(es, en, got_es, got_en) \
    { es, en, got_es, got_en, "FALTAN PIEZAS DEL MÓDULO", "LANDER PARTS ARE MISSING" }

const flag_info_t flag_info[FLAGS] = {
    PART("TOBERA", "NOZZLE", "HAS ENCONTRADO LA TOBERA DEL MÓDULO", "YOU FOUND THE LANDER NOZZLE"),
    PART("PLACA DE CONTROL", "CONTROL BOARD", "HAS ENCONTRADO LA PLACA DE CONTROL", "YOU FOUND THE CONTROL BOARD"),
    PART("CÉLULA DE ENERGÍA", "POWER CELL", "HAS ENCONTRADO LA CÉLULA DE ENERGÍA", "YOU FOUND THE POWER CELL"),
    PART("VÁLVULA DE COMBUSTIBLE", "FUEL VALVE", "HAS ENCONTRADO LA VÁLVULA DE COMBUSTIBLE",
         "YOU FOUND THE FUEL VALVE"),
    { "TARJETA DE ACCESO", "ACCESS CARD", "HAS COGIDO UNA TARJETA DE ACCESO", "YOU TOOK AN ACCESS CARD",
      "HACE FALTA UNA TARJETA DE ACCESO", "AN ACCESS CARD IS NEEDED" },
    { "FILTRO DEL TRAJE", "SUIT FILTER", "HAS COGIDO UN FILTRO PARA EL TRAJE", "YOU TOOK A SUIT FILTER",
      "HACE FALTA UN FILTRO", "A FILTER IS NEEDED" },
    { "CORTADOR", "CUTTER", "HAS COGIDO UN CORTADOR", "YOU TOOK A CUTTER",
      "LA PUERTA ESTÁ SOLDADA", "THE DOOR IS WELDED SHUT" },
    { "CÓDIGO DE LA PUERTA", "DOOR CODE", "HAS ANOTADO EL CÓDIGO", "YOU NOTED THE CODE",
      "LA PUERTA PIDE UN CÓDIGO", "THE DOOR ASKS FOR A CODE" },
    { 0, 0, "ENERGÍA RESTABLECIDA", "POWER RESTORED", "NO HAY ENERGÍA", "THERE IS NO POWER" },
    { 0, 0, "DEFENSAS DESACTIVADAS", "DEFENCES DEACTIVATED", 0, 0 },
    { 0, 0, "MÓDULO REPARADO", "LANDER REPAIRED", 0, 0 },
#define PUMP { 0, 0, "BOMBA DE INYECCIÓN CERRADA", "INJECTION PUMP SHUT", \
               "QUEDAN BOMBAS ABIERTAS", "SOME PUMPS ARE STILL OPEN" }
    PUMP, PUMP, PUMP,
    { 0, 0, "PLANTA PARADA", "PLANT SHUT DOWN", 0, 0 },
    { "PRUEBAS DEL SABOTAJE", "SABOTAGE EVIDENCE", "TIENES PRUEBAS DEL SABOTAJE",
      "YOU HAVE EVIDENCE OF THE SABOTAGE", 0, 0 },
};

/* --- La introducción y el final --------------------------------------------------- */

const log_text_t intro_text = {
    "SERVICIO DE RESCATE COLONIAL\n\n"
    "LA COLONIA MINERA DE BETTER WORLDS\n"
    "NO RESPONDE DESDE HACE SEMANAS. TU\n"
    "MISIÓN: AVERIGUAR QUÉ HA PASADO Y\n"
    "AYUDAR A QUIEN QUEDE CON VIDA.\n\n"
    "UNA TORMENTA TE HA OBLIGADO A UN\n"
    "ATERRIZAJE FORZOSO. EL MÓDULO NO\n"
    "DESPEGARÁ SIN CUATRO PIEZAS.",
    "COLONIAL RESCUE SERVICE\n\n"
    "THE BETTER WORLDS MINING COLONY HAS\n"
    "NOT ANSWERED FOR WEEKS. YOUR\n"
    "MISSION: FIND OUT WHAT HAPPENED AND\n"
    "HELP ANYONE STILL ALIVE.\n\n"
    "A STORM HAS FORCED YOU INTO A CRASH\n"
    "LANDING. THE LANDER WILL NOT TAKE\n"
    "OFF WITHOUT FOUR PARTS."
};

/* El final se compone con una parte de cada pareja, según se haya parado la
 * planta y se lleven las pruebas del sabotaje. */
const log_text_t ending[END_PARTS] = {
    [END_LIFT] = { "EL MÓDULO DESPEGA ENTRE LA TORMENTA.",
                   "THE LANDER LIFTS OFF INTO THE STORM." },
    [END_PLANT_OFF] = { "ABAJO, LA PLANTA ESTÁ EN SILENCIO.\n"
                        "LAS CRIATURAS VOLVERÁN A DORMIR.",
                        "BELOW, THE PLANT IS SILENT. THE\n"
                        "CREATURES WILL GO BACK TO SLEEP." },
    [END_PLANT_ON] = { "ABAJO, LA PLANTA SIGUE CALENTANDO EL\n"
                       "SUELO. LAS CRIATURAS SEGUIRÁN\n"
                       "DESPERTANDO.",
                       "BELOW, THE PLANT KEEPS HEATING THE\n"
                       "GROUND. THE CREATURES WILL KEEP\n"
                       "WAKING." },
    [END_TRUTH] = { "LLEVAS LOS REGISTROS DE CONTROL: LA\n"
                    "ORDEN DE BETTER WORLDS Y EL\n"
                    "SABOTAJE DE LA ANTENA. ESTA VEZ SE\n"
                    "SABRÁ.",
                    "YOU CARRY THE CONTROL LOGS: THE\n"
                    "BETTER WORLDS ORDER AND THE ANTENNA\n"
                    "SABOTAGE. THIS TIME IT WILL BE\n"
                    "KNOWN." },
    [END_NO_TRUTH] = { "NADIE SABRÁ POR QUÉ MURIÓ LA\n"
                       "COLONIA. BETTER WORLDS ENVIARÁ A\n"
                       "OTROS.",
                       "NOBODY WILL KNOW WHY THE COLONY\n"
                       "DIED. BETTER WORLDS WILL SEND\n"
                       "OTHERS." },
};

/* Los registros: primero los de los habitáculos, de varios colonos. */
const log_text_t logs[] = {
    { "LUCAS FERRO, TÉCNICO DE LA PLANTA\n\n"
      "DÍA 211. OTRA NOCHE CON RUIDOS BAJO\n"
      "EL SUELO. EN LA PLANTA DICEN QUE\n"
      "SON LAS BOMBAS DEL TERRAFORMADO.\n"
      "YO TRABAJO CON ESAS BOMBAS.\n"
      "NO SUENAN ASÍ.",
      "LUCAS FERRO, PLANT TECHNICIAN\n\n"
      "DAY 211. ANOTHER NIGHT OF NOISES\n"
      "UNDER THE FLOOR. AT THE PLANT THEY\n"
      "SAY IT IS THE TERRAFORMING PUMPS.\n"
      "I WORK WITH THOSE PUMPS.\n"
      "THEY DON'T SOUND LIKE THAT." },
    { "IRENE SAUCE, CULTIVOS\n\n"
      "LAS RAÍCES APARECEN CORTADAS DESDE\n"
      "ABAJO. EL SUELO ESTÁ CALIENTE, Y\n"
      "DESDE QUE SE AUMENTÓ EL RITMO DE LA\n"
      "PLANTA SE MUEVE. HE PEDIDO QUE LO\n"
      "PAREN. NADIE ME CONTESTA.",
      "IRENE SAUCE, CROPS\n\n"
      "THE ROOTS ARE CUT FROM BELOW. THE\n"
      "SOIL IS WARM, AND SINCE THE PLANT\n"
      "WAS SPED UP IT MOVES. I HAVE ASKED\n"
      "THEM TO STOP IT. NOBODY ANSWERS." },
    { "ANA RIUS, RESPONSABLE DE TURNO\n\n"
      "HEMOS VOTADO: NOS QUEDAMOS AQUÍ.\n"
      "CERRAMOS LOS HABITÁCULOS Y ESPERAMOS\n"
      "AL RELEVO. DEJO LA TARJETA DE\n"
      "CONTROL EN LA MESA. QUE NADIE LA\n"
      "USE SIN AVISAR.",
      "ANA RIUS, SHIFT LEADER\n\n"
      "WE HAVE VOTED: WE STAY HERE. WE\n"
      "SEAL THE HABITATION AND WAIT FOR\n"
      "THE RELIEF. I LEAVE THE CONTROL\n"
      "CARD ON THE TABLE. NOBODY TAKES IT\n"
      "WITHOUT TELLING ME." },
    { "OMAR VIDAL, MANTENIMIENTO\n\n"
      "TODO LO QUE PESA VA A LAS PUERTAS.\n"
      "HEMOS TAPIADO LA DEL PASILLO. AL\n"
      "TALLER SE ENTRA POR EL INVERNADERO.\n"
      "LOS REPUESTOS DE NAVE LOS DEJO EN LA\n"
      "ESTANTERÍA: SON LO ÚNICO QUE NOS\n"
      "SACA DE AQUÍ.",
      "OMAR VIDAL, MAINTENANCE\n\n"
      "EVERYTHING HEAVY GOES TO THE DOORS.\n"
      "WE WALLED UP THE CORRIDOR ONE. THE\n"
      "WORKSHOP IS REACHED THROUGH THE\n"
      "GREENHOUSE. I LEAVE THE SHIP SPARES\n"
      "ON THE SHELF: THEY ARE THE ONLY WAY\n"
      "OUT OF HERE." },
    { "DOCTORA HANA MORI, ENFERMERÍA\n\n"
      "YA NO DUERME NADIE. GOLPEAN LAS\n"
      "PAREDES TODA LA NOCHE. HE PREPARADO\n"
      "LOS FILTROS DE LOS TRAJES: EL AIRE\n"
      "DE LA PLANTA Y DE LA MINA ES VENENO.\n"
      "SI HAY QUE SALIR, QUE SEA CON ELLOS.",
      "DOCTOR HANA MORI, INFIRMARY\n\n"
      "NOBODY SLEEPS ANY MORE. THEY POUND\n"
      "ON THE WALLS ALL NIGHT. I HAVE MADE\n"
      "READY THE SUIT FILTERS: THE AIR OF\n"
      "THE PLANT AND THE MINE IS POISON.\n"
      "IF WE MUST GO OUT, WEAR THEM." },
    { "ANA RIUS, RESPONSABLE DE TURNO\n\n"
      "HAN ENTRADO POR EL INVERNADERO. NO\n"
      "PODEMOS AGUANTAR MÁS. SI ALGUIEN\n"
      "LLEGA: EL CÓDIGO DE LA PUERTA DE\n"
      "CONTROL ES 2-0-6-1. DESDE ALLÍ SE\n"
      "PUEDE DEVOLVER LA ENERGÍA.\n"
      "LO SIENTO.",
      "ANA RIUS, SHIFT LEADER\n\n"
      "THEY GOT IN THROUGH THE GREENHOUSE.\n"
      "WE CANNOT HOLD ANY LONGER. IF\n"
      "SOMEONE COMES: THE CONTROL DOOR\n"
      "CODE IS 2-0-6-1. FROM THERE THE\n"
      "POWER CAN BE RESTORED.\n"
      "I AM SORRY." },
    /* Los de control, del personal que trabajaba allí. */
    { "TEO BALSA, ENERGÍA\n\n"
      "HAN ENTRADO POR LOS CONDUCTOS DE\n"
      "VENTILACIÓN. VAN HACIA EL CALOR DE\n"
      "LOS GENERADORES. LOS HE PARADO Y HE\n"
      "SELLADO CONTROL. SIN ENERGÍA NO HAY\n"
      "RECARGA, PERO TAMPOCO HAY NADA QUE\n"
      "LAS ATRAIGA.",
      "TEO BALSA, POWER\n\n"
      "THEY GOT IN THROUGH THE AIR DUCTS.\n"
      "THEY GO FOR THE HEAT OF THE\n"
      "GENERATORS. I HAVE STOPPED THEM AND\n"
      "SEALED CONTROL. NO POWER MEANS NO\n"
      "RECHARGE, BUT NOTHING TO DRAW THEM\n"
      "EITHER." },
    { "MARA OSET, SEGURIDAD\n\n"
      "HE REVISADO EL REGISTRO DE LA\n"
      "ANTENA. NUESTRAS LLAMADAS DE SOCORRO\n"
      "NO HAN SALIDO NUNCA. LA ANTENA SÍ\n"
      "ENVÍA: DATOS DE LAS CRIATURAS, CADA\n"
      "HORA, A LA EMPRESA.",
      "MARA OSET, SECURITY\n\n"
      "I HAVE CHECKED THE ANTENNA LOG. OUR\n"
      "DISTRESS CALLS NEVER WENT OUT. THE\n"
      "ANTENNA DOES SEND: DATA ON THE\n"
      "CREATURES, EVERY HOUR, TO THE\n"
      "COMPANY." },
    { "JON ARIZA, COMUNICACIONES\n\n"
      "NO ES UNA AVERÍA. LA ANTENA SE\n"
      "CONFIGURÓ DESDE LA EMPRESA PARA\n"
      "BLOQUEAR TODO MENOS SUS DATOS. HE\n"
      "ENCONTRADO LA ORDEN. LA FIRMA LA\n"
      "DIRECCIÓN.",
      "JON ARIZA, COMMUNICATIONS\n\n"
      "IT IS NOT A FAULT. THE ANTENNA WAS\n"
      "SET UP FROM THE COMPANY TO BLOCK\n"
      "EVERYTHING BUT THEIR DATA. I HAVE\n"
      "FOUND THE ORDER. IT IS SIGNED BY\n"
      "THE BOARD." },
    { "MARA OSET, SEGURIDAD\n\n"
      "LA ORDEN DICE: NO EVACUAR. OBSERVAR.\n"
      "VALORAR SU USO COMO ARMA BIOLÓGICA.\n"
      "EL EXPERIMENTO SOMOS NOSOTROS.\n\n"
      "QUEDA OTRO REPUESTO DE NAVE EN LA\n"
      "MINA, EN EL ÚLTIMO NIVEL.",
      "MARA OSET, SECURITY\n\n"
      "THE ORDER SAYS: DO NOT EVACUATE.\n"
      "OBSERVE. ASSESS THEIR USE AS A\n"
      "BIOLOGICAL WEAPON. WE ARE THE\n"
      "EXPERIMENT.\n\n"
      "THERE IS ANOTHER SHIP SPARE IN THE\n"
      "MINE, ON THE LOWEST LEVEL." },
    /* Los de la mina, de los mineros, uno por nivel. */
    { "IKER SOLA, PERFORACIÓN\n\n"
      "EN EL TERCER NIVEL LA BARRENA HA\n"
      "DADO CON UN HUECO CALIENTE. DENTRO\n"
      "HAY ALGO QUE SE MUEVE, MUCHAS COSAS.\n"
      "HEMOS CERRADO LA GALERÍA Y LO HEMOS\n"
      "COMUNICADO A CONTROL.",
      "IKER SOLA, DRILLING\n\n"
      "ON THE THIRD LEVEL THE DRILL HAS\n"
      "BROKEN INTO A WARM HOLLOW. INSIDE\n"
      "SOMETHING MOVES, MANY THINGS. WE\n"
      "HAVE SEALED THE GALLERY AND\n"
      "REPORTED IT TO CONTROL." },
    { "NURIA PENAS, JEFA DE TURNO\n\n"
      "HA LLEGADO UNA ORDEN DE LA EMPRESA:\n"
      "REABRIR LA GALERÍA DEL TERCER NIVEL\n"
      "Y SACAR MUESTRAS VIVAS PARA LA\n"
      "LANZADERA. PAGAN EL TRIPLE. EL\n"
      "ASCENSOR YA NO BAJA: IREMOS POR EL\n"
      "POZO DE VENTILACIÓN.",
      "NURIA PENAS, SHIFT LEADER\n\n"
      "AN ORDER HAS COME FROM THE COMPANY:\n"
      "REOPEN THE THIRD LEVEL GALLERY AND\n"
      "TAKE LIVE SAMPLES FOR THE SHUTTLE.\n"
      "THEY PAY TRIPLE. THE LIFT NO LONGER\n"
      "GOES DOWN: WE WILL USE THE\n"
      "VENTILATION SHAFT." },
    { "ELENA CASAS, MINERA\n\n"
      "BAJAMOS SEIS CON LAS CAJAS DE LAS\n"
      "MUESTRAS Y EL REPUESTO DE NAVE DE\n"
      "LA LANZADERA. QUEDAMOS DOS.\n"
      "NINGUNA MUESTRA VALE ESTO.\n"
      "QUE NADIE MÁS BAJE.",
      "ELENA CASAS, MINER\n\n"
      "SIX OF US WENT DOWN WITH THE SAMPLE\n"
      "CASES AND THE SHUTTLE'S SHIP SPARE.\n"
      "TWO OF US ARE LEFT.\n"
      "NO SAMPLE IS WORTH THIS.\n"
      "NOBODY ELSE COME DOWN HERE." },
    /* Los de la planta: dos del personal y el del operario. */
    { "SARA LLEDÓ, OPERACIONES\n\n"
      "LA EMPRESA ORDENA MANTENER LA PLANTA\n"
      "AL MÁXIMO. CADA VEZ QUE SUBIMOS LA\n"
      "INYECCIÓN, LAS CRIATURAS SE ACERCAN\n"
      "MÁS A LA SUPERFICIE. LO HE APUNTADO\n"
      "TODO. NADIE QUIERE LEERLO.",
      "SARA LLEDÓ, OPERATIONS\n\n"
      "THE COMPANY ORDERS THE PLANT KEPT AT\n"
      "FULL POWER. EVERY TIME WE RAISE THE\n"
      "INJECTION, THE CREATURES COME CLOSER\n"
      "TO THE SURFACE. I HAVE WRITTEN IT\n"
      "ALL DOWN. NOBODY WANTS TO READ IT." },
    { "PAU RIBAS, MANTENIMIENTO\n\n"
      "LAS BOMBAS SE PUEDEN CERRAR A MANO,\n"
      "UNA A UNA, Y CONFIRMARLO LUEGO EN\n"
      "CONTROL. LO HE PROPUESTO TRES VECES.\n"
      "LA RESPUESTA ES SIEMPRE LA MISMA:\n"
      "SEGUIR.",
      "PAU RIBAS, MAINTENANCE\n\n"
      "THE PUMPS CAN BE SHUT BY HAND, ONE\n"
      "BY ONE, AND THEN CONFIRMED IN\n"
      "CONTROL. I HAVE PROPOSED IT THREE\n"
      "TIMES. THE ANSWER IS ALWAYS THE\n"
      "SAME: CARRY ON." },
    { "BRUNO ESTEVE, OPERARIO\n\n"
      "HE SOLDADO LA PUERTA DESDE DENTRO.\n"
      "TENGO LA PIEZA QUE FALTABA PARA\n"
      "ARREGLAR LA NAVE DE CARGA, PERO YA\n"
      "NO PUEDO LLEGAR HASTA ELLA. SI\n"
      "ALGUIEN LA ENCUENTRA, QUE LA USE Y\n"
      "SE VAYA DE AQUÍ.",
      "BRUNO ESTEVE, OPERATOR\n\n"
      "I HAVE WELDED THE DOOR FROM INSIDE.\n"
      "I HAVE THE PART THAT WAS MISSING TO\n"
      "FIX THE CARGO SHIP, BUT I CAN NO\n"
      "LONGER REACH IT. WHOEVER FINDS IT,\n"
      "USE IT AND GET AWAY FROM HERE." },
    /* El de la mina, junto al pozo, del primero que bajó al nido. */
    { "TOMÁS RUIZ, MINERO\n\n"
      "BAJÉ EL PRIMERO POR EL POZO. AL\n"
      "FONDO DE LA CUEVA HAY ALGO GRANDE\n"
      "PEGADO A LA ROCA, CON BOLSAS QUE\n"
      "BRILLAN Y LATEN. DE ELLAS SALEN LAS\n"
      "DEMÁS. MIENTRAS VIVA, EL NIDO NO\n"
      "DUERME. Y ESCUPE DE LEJOS.",
      "TOMÁS RUIZ, MINER\n\n"
      "I WENT DOWN THE SHAFT FIRST. AT THE\n"
      "BACK OF THE CAVE THERE IS SOMETHING\n"
      "BIG STUCK TO THE ROCK, WITH SACS\n"
      "THAT GLOW AND PULSE. THE OTHERS COME\n"
      "OUT OF THEM. WHILE IT LIVES, THE\n"
      "NEST NEVER SLEEPS. AND IT SPITS." },
};
