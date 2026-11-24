/*
 * zlcrypto.h - Lo que hace falta para emparejar un PC con la máquina por la
 * red y firmar lo que se dicen (D106): SHA-256, HMAC-SHA-256 y X25519. En C
 * portable, para el PC y para la Pico CPU (sin memoria dinámica, sin tablas
 * grandes).
 *
 *   - SHA-256 y HMAC-SHA-256: FIPS 180-4 y RFC 2104.
 *   - X25519: RFC 7748, a partir de crypto_scalarmult de TweetNaCl (Bernstein,
 *     Janssen, Lange, Schwabe y Smetsers, de dominio público), con los
 *     productos en 32 bits y una llamada cada pocos pasos para que quien lo
 *     llama atienda lo que no puede esperar (el sonido, en la Pico).
 *
 * MIT, como el resto del SDK.
 */
#ifndef ZXV_ZLCRYPTO_H
#define ZXV_ZLCRYPTO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t h[8];
    uint64_t len;                       /* bytes ya metidos */
    uint8_t buf[64];
    unsigned used;
} zl_sha256_t;

void zl_sha256_init(zl_sha256_t *s);
void zl_sha256_update(zl_sha256_t *s, const void *data, size_t n);
void zl_sha256_final(zl_sha256_t *s, uint8_t out[32]);
void zl_sha256(const void *data, size_t n, uint8_t out[32]);

/* HMAC-SHA-256 de varios trozos seguidos (parts[i], lens[i]). */
void zl_hmac_sha256(const uint8_t *key, size_t key_len, const void *const *parts, const size_t *lens, int n,
                    uint8_t out[32]);

/* X25519: out = scalar * point (32 bytes cada uno, little endian). El
 * escalar se recorta como dice la RFC 7748. Cada 16 pasos de los 255 llama a
 * idle(ctx), si no es NULL. */
void zl_x25519(uint8_t out[32], const uint8_t scalar[32], const uint8_t point[32], void (*idle)(void *ctx),
               void *ctx);

/* La clave pública de un escalar: scalar * 9. */
void zl_x25519_base(uint8_t out[32], const uint8_t scalar[32], void (*idle)(void *ctx), void *ctx);

/* Comparar sin que el tiempo diga dónde está la diferencia: 0 si iguales. */
int zl_memcmp_ct(const void *a, const void *b, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* ZXV_ZLCRYPTO_H */
