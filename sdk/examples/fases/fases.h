/*
 * fases.h - Un ejemplo de los bancos (D109): dos fases, cada una con su
 * banco de código (lo que se mueve) y su banco de datos (el fondo), que se
 * cargan del propio fichero al empezar la fase. Lo de las dos fases no está
 * nunca a la vez en la memoria: comparten una zona.
 */
#ifndef FASES_H
#define FASES_H

#include <stdint.h>

#define W           256
#define H           192
#define SKY_Y       24                  /* el fondo, de 256 x 96 */
#define SKY_H       96
#define GROUND_Y    (SKY_Y + SKY_H)     /* lo de abajo lo pinta cada fase */

/* Banco de código "valle" (valle.c). */
void valle_draw(int frame);

/* Banco de código "espacio" (espacio.c). */
void espacio_draw(int frame);

#endif
