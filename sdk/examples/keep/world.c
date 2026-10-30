/*
 * world.c - La planta de The Keep: su mapa (de keep.gfx), lo que se pisa y lo
 * que no, las puertas y lo que sale en cada sala.
 *
 * Cada planta es un mapa de 48 x 33 celdas: 3 x 3 salas de 16 x 11 tiles de
 * 16 x 16. Se pisan los suelos (tiles 0-3), los pinchos (11, que hacen daño
 * sin las botas), la escalera (15), la alfombra (16) y el suelo agrietado
 * (17); lo demás es pared u obstáculo. Los murciélagos vuelan también sobre
 * el agua (10) y los pinchos.
 *
 * Las marcas (tiles 32-43, keep.h) dicen lo que sale en esa celda: el
 * héroe, el sabio, los enemigos, el hechicero y los objetos. Al cargar la
 * planta se apuntan en spawns y la celda queda con el suelo de la planta
 * (el más común de los 0-3 del mapa).
 */
#include "keep.h"

uint16_t cells[MAP_H][MAP_W];
zxv_tilemap floor_map;
spawn_t spawns[SPAWNS];
int nspawns;
int floor_base;

void world_load(int f)
{
    art_floor(f, &cells[0][0]);         /* art_init ya ha visto que está */
    int count[4] = { 0, 0, 0, 0 };
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) {
            cells[y][x] &= ZXV_CELL_TILE;
            if (cells[y][x] < 4)
                count[cells[y][x]]++;
        }
    floor_base = 0;
    for (int i = 1; i < 4; i++)
        if (count[i] > count[floor_base])
            floor_base = i;
    nspawns = 0;
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) {
            int m = cells[y][x] - T_MARK;
            if (m < 0 || m >= MARKS)
                continue;
            cells[y][x] = (uint16_t)floor_base;
            if (nspawns < SPAWNS)
                spawns[nspawns++] = (spawn_t){ (uint8_t)m, (uint8_t)x, (uint8_t)y, 1 };
        }
}

int cell_at(int px, int py)
{
    if (px < 0 || py < 0 || px >= MAP_W * TILE || py >= MAP_H * TILE)
        return T_WALL;
    return cells[py / TILE][px / TILE];
}

int solid_tile(int t)
{
    return !(t <= T_FLOOR3 || t == T_SPIKES || t == T_STAIRS || t == T_RUG || t == T_CRACK);
}

int wall_tile(int t) { return solid_tile(t) && t != T_WATER && t != T_SPIKES; }

/* La puerta (o la zarza) de la celda y las del mismo tile pegadas a ella:
 * las de un paso son dos. */
void open_gate(int cx, int cy)
{
    if (cx < 0 || cy < 0 || cx >= MAP_W || cy >= MAP_H)
        return;
    int t = cells[cy][cx];
    cells[cy][cx] = (uint16_t)floor_base;
    static const int8_t d[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    for (int i = 0; i < 4; i++) {
        int x = cx + d[i][0], y = cy + d[i][1];
        if (x >= 0 && y >= 0 && x < MAP_W && y < MAP_H && cells[y][x] == t)
            open_gate(x, y);
    }
}

int find_start(int *px, int *py)
{
    for (int i = 0; i < nspawns; i++)
        if (spawns[i].mark == M_START) {
            *px = spawns[i].cx * TILE;
            *py = spawns[i].cy * TILE;
            return 1;
        }
    *px = ROOM_W * TILE + 7 * TILE;     /* sin marca: en medio de la sala de abajo */
    *py = 2 * ROOM_H * TILE + 5 * TILE;
    return 0;
}
