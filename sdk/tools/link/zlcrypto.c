/*
 * zlcrypto.c - SHA-256, HMAC-SHA-256 y X25519 (ver zlcrypto.h).
 */
#include "zlcrypto.h"

#include <string.h>

/* --- SHA-256 (FIPS 180-4) -------------------------------------------------------------- */

static const uint32_t K256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

static uint32_t ror(uint32_t x, unsigned n) { return x >> n | x << (32 - n); }

static void block(zl_sha256_t *s, const uint8_t *p)
{
    uint32_t w[64];
    for (int i = 0; i < 16; i++)
        w[i] = (uint32_t)p[4 * i] << 24 | (uint32_t)p[4 * i + 1] << 16 | (uint32_t)p[4 * i + 2] << 8 | p[4 * i + 3];
    for (int i = 16; i < 64; i++) {
        uint32_t s0 = ror(w[i - 15], 7) ^ ror(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = ror(w[i - 2], 17) ^ ror(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    uint32_t a = s->h[0], b = s->h[1], c = s->h[2], d = s->h[3], e = s->h[4], f = s->h[5], g = s->h[6], h = s->h[7];
    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + (ror(e, 6) ^ ror(e, 11) ^ ror(e, 25)) + ((e & f) ^ (~e & g)) + K256[i] + w[i];
        uint32_t t2 = (ror(a, 2) ^ ror(a, 13) ^ ror(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    s->h[0] += a;
    s->h[1] += b;
    s->h[2] += c;
    s->h[3] += d;
    s->h[4] += e;
    s->h[5] += f;
    s->h[6] += g;
    s->h[7] += h;
}

void zl_sha256_init(zl_sha256_t *s)
{
    static const uint32_t h0[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
    memcpy(s->h, h0, sizeof h0);
    s->len = 0;
    s->used = 0;
}

void zl_sha256_update(zl_sha256_t *s, const void *data, size_t n)
{
    const uint8_t *p = data;
    s->len += n;
    while (n) {
        size_t k = 64 - s->used < n ? 64 - s->used : n;
        memcpy(s->buf + s->used, p, k);
        s->used += (unsigned)k;
        p += k;
        n -= k;
        if (s->used == 64) {
            block(s, s->buf);
            s->used = 0;
        }
    }
}

void zl_sha256_final(zl_sha256_t *s, uint8_t out[32])
{
    uint64_t bits = s->len * 8;
    uint8_t pad = 0x80;
    zl_sha256_update(s, &pad, 1);
    pad = 0;
    while (s->used != 56)
        zl_sha256_update(s, &pad, 1);
    uint8_t l[8];
    for (int i = 0; i < 8; i++)
        l[i] = (uint8_t)(bits >> (56 - 8 * i));
    zl_sha256_update(s, l, 8);
    for (int i = 0; i < 8; i++) {
        out[4 * i] = (uint8_t)(s->h[i] >> 24);
        out[4 * i + 1] = (uint8_t)(s->h[i] >> 16);
        out[4 * i + 2] = (uint8_t)(s->h[i] >> 8);
        out[4 * i + 3] = (uint8_t)s->h[i];
    }
}

void zl_sha256(const void *data, size_t n, uint8_t out[32])
{
    zl_sha256_t s;
    zl_sha256_init(&s);
    zl_sha256_update(&s, data, n);
    zl_sha256_final(&s, out);
}

/* --- HMAC-SHA-256 (RFC 2104) ----------------------------------------------------------- */

void zl_hmac_sha256(const uint8_t *key, size_t key_len, const void *const *parts, const size_t *lens, int n,
                    uint8_t out[32])
{
    uint8_t k[64], pad[64], inner[32];
    memset(k, 0, sizeof k);
    if (key_len > 64)
        zl_sha256(key, key_len, k);
    else
        memcpy(k, key, key_len);
    zl_sha256_t s;
    for (int i = 0; i < 64; i++)
        pad[i] = k[i] ^ 0x36;
    zl_sha256_init(&s);
    zl_sha256_update(&s, pad, 64);
    for (int i = 0; i < n; i++)
        zl_sha256_update(&s, parts[i], lens[i]);
    zl_sha256_final(&s, inner);
    for (int i = 0; i < 64; i++)
        pad[i] = k[i] ^ 0x5c;
    zl_sha256_init(&s);
    zl_sha256_update(&s, pad, 64);
    zl_sha256_update(&s, inner, 32);
    zl_sha256_final(&s, out);
}

int zl_memcmp_ct(const void *a, const void *b, size_t n)
{
    const uint8_t *x = a, *y = b;
    uint8_t d = 0;
    for (size_t i = 0; i < n; i++)
        d |= x[i] ^ y[i];
    return d;
}

/* --- X25519 (RFC 7748), de crypto_scalarmult de TweetNaCl ------------------------------- */

typedef int64_t gf[16];                 /* 16 trozos de 16 bits (con signo, sin acarrear) */

static void car25519(gf o)
{
    for (int i = 0; i < 16; i++) {
        o[i] += (int64_t)1 << 16;
        int64_t c = o[i] >> 16;         /* desplazamiento aritmético, como en TweetNaCl */
        if (i < 15)
            o[i + 1] += c - 1;
        else
            o[0] += 38 * (c - 1);
        o[i] -= c * 65536;
    }
}

static void sel25519(gf p, gf q, int b)
{
    int64_t c = ~((int64_t)b - 1);
    for (int i = 0; i < 16; i++) {
        int64_t t = c & (p[i] ^ q[i]);
        p[i] ^= t;
        q[i] ^= t;
    }
}

static void pack25519(uint8_t *o, const gf n)
{
    gf m, t;
    for (int i = 0; i < 16; i++)
        t[i] = n[i];
    car25519(t);
    car25519(t);
    car25519(t);
    for (int j = 0; j < 2; j++) {
        m[0] = t[0] - 0xffed;
        for (int i = 1; i < 15; i++) {
            m[i] = t[i] - 0xffff - ((m[i - 1] >> 16) & 1);
            m[i - 1] &= 0xffff;
        }
        m[15] = t[15] - 0x7fff - ((m[14] >> 16) & 1);
        int b = (int)((m[15] >> 16) & 1);
        m[14] &= 0xffff;
        sel25519(t, m, 1 - b);
    }
    for (int i = 0; i < 16; i++) {
        o[2 * i] = (uint8_t)(t[i] & 0xff);
        o[2 * i + 1] = (uint8_t)(t[i] >> 8);
    }
}

static void unpack25519(gf o, const uint8_t *n)
{
    for (int i = 0; i < 16; i++)
        o[i] = n[2 * i] + ((int64_t)n[2 * i + 1] << 8);
    o[15] &= 0x7fff;
}

static void A(gf o, const gf a, const gf b)
{
    for (int i = 0; i < 16; i++)
        o[i] = a[i] + b[i];
}

static void Z(gf o, const gf a, const gf b)
{
    for (int i = 0; i < 16; i++)
        o[i] = a[i] - b[i];
}

/* Los trozos caben en 32 bits (con signo): el producto, de 32 x 32. */
static void M(gf o, const gf a, const gf b)
{
    int64_t t[31];
    for (int i = 0; i < 31; i++)
        t[i] = 0;
    for (int i = 0; i < 16; i++)
        for (int j = 0; j < 16; j++)
            t[i + j] += (int64_t)(int32_t)a[i] * (int32_t)b[j];
    for (int i = 0; i < 15; i++)
        t[i] += 38 * t[i + 16];
    for (int i = 0; i < 16; i++)
        o[i] = t[i];
    car25519(o);
    car25519(o);
}

static void S(gf o, const gf a) { M(o, a, a); }

static void inv25519(gf o, const gf in)
{
    gf c;
    for (int a = 0; a < 16; a++)
        c[a] = in[a];
    for (int a = 253; a >= 0; a--) {
        S(c, c);
        if (a != 2 && a != 4)
            M(c, c, in);
    }
    for (int a = 0; a < 16; a++)
        o[a] = c[a];
}

void zl_x25519(uint8_t out[32], const uint8_t scalar[32], const uint8_t point[32], void (*idle)(void *ctx),
               void *ctx)
{
    static const gf k121665 = { 0xDB41, 1 };
    uint8_t z[32];
    gf x, a, b, c, d, e, f;
    for (int i = 0; i < 31; i++)
        z[i] = scalar[i];
    z[31] = (uint8_t)((scalar[31] & 127) | 64);
    z[0] &= 248;
    unpack25519(x, point);
    for (int i = 0; i < 16; i++) {
        b[i] = x[i];
        d[i] = a[i] = c[i] = 0;
    }
    a[0] = d[0] = 1;
    for (int i = 254; i >= 0; --i) {
        int r = (z[i >> 3] >> (i & 7)) & 1;
        sel25519(a, b, r);
        sel25519(c, d, r);
        A(e, a, c);
        Z(a, a, c);
        A(c, b, d);
        Z(b, b, d);
        S(d, e);
        S(f, a);
        M(a, c, a);
        M(c, b, e);
        A(e, a, c);
        Z(a, a, c);
        S(b, a);
        Z(c, d, f);
        M(a, c, k121665);               /* RFC 7748: z2 = E * (AA + a24 * E), x2 = AA * BB */
        A(a, a, d);
        M(c, c, a);
        M(a, d, f);
        M(d, b, x);
        S(b, e);
        sel25519(a, b, r);
        sel25519(c, d, r);
        if (idle && !(i & 15))
            idle(ctx);
    }
    inv25519(c, c);
    M(a, a, c);
    pack25519(out, a);
    memset(z, 0, sizeof z);
}

void zl_x25519_base(uint8_t out[32], const uint8_t scalar[32], void (*idle)(void *ctx), void *ctx)
{
    static const uint8_t nine[32] = { 9 };
    zl_x25519(out, scalar, nine, idle, ctx);
}
