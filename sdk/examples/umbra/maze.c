/*
 * maze.c - Los laberintos de Umbra: los mapas de umbra.gfx, uno por nivel,
 * que se pueden cambiar con el editor (zxv-gfx o gfxedit). Cada celda del
 * mapa es un número de tile que dice lo que hay en ella (MAP_FLOOR... en
 * umbra.h):
 *
 *    0      suelo
 *    1-5    pared de piedra, ladrillo, musgo, cristal o templo
 *    6      pared con una runa tallada
 *    7      la puerta de salida (en una pared, a la que se llega por un lado)
 *    8      una runa, sobre el suelo
 *    9      un frasco de aceite
 *    10     una sombra (donde empieza)
 *    11-14  la entrada: donde empieza el jugador, mirando al este, al sur,
 *           al oeste o al norte
 *
 * Fuera del mapa es pared. Un nivel necesita una entrada y una salida; sin
 * runas, la puerta está abierta desde el principio. El suelo y el techo
 * toman los colores de la pared que más hay.
 */
#include "umbra.h"

int maze_w, maze_h, level, levels, level_wall;
uint8_t maze[MAZE_MAX * MAZE_MAX];
uint8_t seen[MAZE_MAX * MAZE_MAX];
uint8_t dist[MAZE_MAX * MAZE_MAX];
item_t items[ITEMS];
int item_count, runes_total, runes_left;
shadow_t shadows[SHADOWS];
int shadow_count;
int start_x, start_y, start_ang, gate_x, gate_y, gate_open;

static const int8_t dx4[4] = { 1, 0, -1, 0 }, dy4[4] = { 0, 1, 0, -1 };
static uint16_t cells[MAZE_MAX * MAZE_MAX];

int cell_at(int x, int y)
{
    if ((unsigned)x >= (unsigned)maze_w || (unsigned)y >= (unsigned)maze_h)
        return CELL_WALL + WALL_STONE;
    return maze[y * MAZE_MAX + x];
}

int cell_solid(int x, int y)
{
    int c = cell_at(x, y);
    return c != CELL_FLOOR && (c != CELL_GATE || !gate_open);
}

/* Un recorrido en anchura desde (x, y) por lo que no es sólido. */
void maze_flood(uint8_t *out, int x, int y)
{
    static uint16_t queue[MAZE_MAX * MAZE_MAX];
    for (int i = 0; i < MAZE_MAX * MAZE_MAX; i++)
        out[i] = 255;
    if (cell_solid(x, y))
        return;
    int head = 0, tail = 0;
    out[y * MAZE_MAX + x] = 0;
    queue[tail++] = (uint16_t)(y * MAZE_MAX + x);
    while (head < tail) {
        int c = queue[head++], cx = c % MAZE_MAX, cy = c / MAZE_MAX;
        for (int d = 0; d < 4; d++) {
            int nx = cx + dx4[d], ny = cy + dy4[d];
            if (cell_solid(nx, ny) || out[ny * MAZE_MAX + nx] != 255)
                continue;
            out[ny * MAZE_MAX + nx] = (uint8_t)(out[c] < 254 ? out[c] + 1 : 254);
            queue[tail++] = (uint16_t)(ny * MAZE_MAX + nx);
        }
    }
}

void maze_distances(int x, int y) { maze_flood(dist, x, y); }

/* El informe de un nivel que no vale; vuelve al BASIC. */
static void bad_level(int n, const char *why)
{
    static char msg[64];
    const char *parts[3] = { T("umbra.gfx: nivel ", "umbra.gfx: level "), 0, why };
    char num[4] = { (char)('0' + (n + 1) / 10), (char)('0' + (n + 1) % 10), 0, 0 };
    parts[1] = num[0] == '0' ? num + 1 : num;
    int k = 0;
    for (int i = 0; i < 3; i++)
        for (const char *s = parts[i]; *s && k < 63; s++)
            msg[k++] = *s;
    msg[k] = 0;
    zxv_report_error(0, msg);
}

/* Lee el mapa del nivel lvl en maze[] y lo que hay en él; con check, da un
 * informe si no vale (y no vuelve). */
static void load(int lvl, int check)
{
    int w = 0, h = 0;
    if (!art_level(lvl, cells, sizeof(cells), &w, &h) || w > MAZE_MAX || h > MAZE_MAX) {
        if (check)
            bad_level(lvl, T(" demasiado grande", " too big"));
        w = h = 3;
        for (int i = 0; i < 9; i++)
            cells[i] = i == 4 ? MAP_START_E : MAP_STONE;
    }
    maze_w = w;
    maze_h = h;
    item_count = shadow_count = runes_total = 0;
    int starts = 0, gates = 0, walls[5] = { 0 };
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            int v = cells[y * w + x] & ZXV_CELL_TILE, c = CELL_FLOOR;
            int32_t cx = x * ONE + ONE / 2, cy = y * ONE + ONE / 2;
            if (v >= MAP_STONE && v <= MAP_TEMPLE) {
                c = CELL_WALL + WALL_STONE + v - MAP_STONE;
                walls[v - MAP_STONE]++;
            } else if (v == MAP_RUNE_WALL) {
                c = CELL_WALL + WALL_RUNE;
            } else if (v == MAP_EXIT) {
                c = CELL_GATE;
                gate_x = x;
                gate_y = y;
                gates++;
            } else if ((v == MAP_RUNE || v == MAP_FLASK) && item_count < ITEMS) {
                items[item_count++] = (item_t){ cx, cy, (uint8_t)(v == MAP_RUNE ? ITEM_RUNE : ITEM_FLASK), 0 };
                runes_total += v == MAP_RUNE;
            } else if (v == MAP_SHADOW && shadow_count < SHADOWS) {
                shadows[shadow_count++] = (shadow_t){ cx, cy, (int8_t)x, (int8_t)y, 0, 0, 0 };
            } else if (v >= MAP_START_E && v <= MAP_START_N) {
                start_x = x;
                start_y = y;
                start_ang = (v - MAP_START_E) * TURN / 4;
                starts++;
            } else if (v != MAP_FLOOR) {
                c = CELL_WALL + WALL_STONE;     /* lo que no se sabe qué es, pared */
            }
            maze[y * MAZE_MAX + x] = (uint8_t)c;
        }
    if (check && (starts != 1 || gates != 1))
        bad_level(lvl, T(" sin entrada o salida", " has no start or exit"));
    level_wall = WALL_STONE;
    for (int i = 1; i < 5; i++)
        if (walls[i] > walls[level_wall])
            level_wall = i;
    runes_left = runes_total;
    gate_open = runes_total == 0;
}

void maze_init(void)
{
    levels = art_levels();
    if (!levels)
        zxv_report_error(0, T("umbra.gfx no tiene niveles", "umbra.gfx has no levels"));
    for (int i = 0; i < levels; i++)
        load(i, 1);
}

void maze_build(int lvl)
{
    level = lvl;
    load(lvl, 0);
    for (int i = 0; i < MAZE_MAX * MAZE_MAX; i++)
        seen[i] = 0;
}

/* --- Las sombras ---------------------------------------------------------------- */

void shadows_respawn(shadow_t *s, int min_dist)
{
    int x = start_x, y = start_y, best = -1;
    for (int tries = 0; tries < 200; tries++) {
        int cx = (int)(rnd() % (unsigned)maze_w), cy = (int)(rnd() % (unsigned)maze_h);
        int d = dist[cy * MAZE_MAX + cx];
        if (d == 255 || maze[cy * MAZE_MAX + cx] != CELL_FLOOR || d <= best)
            continue;
        best = d;
        x = cx;
        y = cy;
        if (d >= min_dist)
            break;
    }
    *s = (shadow_t){ x * ONE + ONE / 2, y * ONE + ONE / 2, (int8_t)x, (int8_t)y, 0, 0, 0 };
}

static int shadow_can(int x, int y) { return cell_at(x, y) == CELL_FLOOR; }

/* La celda siguiente de una sombra que ha llegado a la suya: hacia el jugador
 * si lo huele (está a 8 pasos o menos), y si no, por donde caiga, sin volver
 * atrás salvo en un callejón. */
static void shadow_next(shadow_t *s)
{
    int cx = s->tx, cy = s->ty;
    int d0 = dist[cy * MAZE_MAX + cx];
    s->chasing = d0 <= 8;
    int best = -1, options[4], k = 0;
    for (int d = 0; d < 4; d++) {
        int nx = cx + dx4[d], ny = cy + dy4[d];
        if (!shadow_can(nx, ny))
            continue;
        if (s->chasing && dist[ny * MAZE_MAX + nx] < d0)
            best = d;
        if (d != ((s->dir + 2) & 3))
            options[k++] = d;
    }
    if (best < 0) {
        if (k)
            best = options[rnd() % (unsigned)k];
        else if (shadow_can(cx + dx4[(s->dir + 2) & 3], cy + dy4[(s->dir + 2) & 3]))
            best = (s->dir + 2) & 3;
        else
            return;
    }
    s->dir = (int8_t)best;
    s->tx = (int8_t)(cx + dx4[best]);
    s->ty = (int8_t)(cy + dy4[best]);
}

void shadows_update(int speed)
{
    for (int i = 0; i < shadow_count; i++) {
        shadow_t *s = &shadows[i];
        int step = per_frame(&s->rest, speed * 256);
        int32_t gx = s->tx * ONE + ONE / 2, gy = s->ty * ONE + ONE / 2;
        int32_t ex = gx - s->x, ey = gy - s->y;
        int32_t left = (ex < 0 ? -ex : ex) + (ey < 0 ? -ey : ey);
        if (left <= step) {
            s->x = gx;
            s->y = gy;
            step -= left;
            shadow_next(s);
            ex = s->tx * ONE + ONE / 2 - s->x;
            ey = s->ty * ONE + ONE / 2 - s->y;
        }
        if (ex)
            s->x += ex > 0 ? (step < ex ? step : ex) : (step < -ex ? -step : ex);
        else if (ey)
            s->y += ey > 0 ? (step < ey ? step : ey) : (step < -ey ? -step : ey);
    }
}
