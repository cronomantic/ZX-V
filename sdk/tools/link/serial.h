/*
 * serial.h - Un puerto serie, en Windows (COM5) y en Linux o macOS
 * (/dev/ttyUSB0): 8N1, sin control de flujo, a la velocidad que se pida.
 *
 * MIT, como el resto del SDK.
 */
#ifndef ZXV_SERIAL_H
#define ZXV_SERIAL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct serial serial_t;

/* Por qué no se puede abrir. */
enum { SERIAL_E_OTHER, SERIAL_E_NONE, SERIAL_E_BUSY, SERIAL_E_DENIED };

/* NULL si no se puede abrir: por qué, en *err (SERIAL_E_*) y, en texto
 * del sistema, en why (128 bytes). */
serial_t *serial_open(const char *name, unsigned baud, int *err, char *why);
void serial_close(serial_t *s);

/* Lo que llegue en timeout_ms como mucho (en cuanto llega algo, vuelve):
 * cuántos bytes, 0 si nada, -1 si el puerto falla. */
int serial_read(serial_t *s, uint8_t *buf, size_t n, unsigned timeout_ms);
int serial_write(serial_t *s, const uint8_t *buf, size_t n);

/* Los puertos que hay, uno por línea en out (n bytes): cuántos. */
int serial_list(char *out, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* ZXV_SERIAL_H */
