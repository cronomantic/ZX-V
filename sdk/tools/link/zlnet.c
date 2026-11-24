/*
 * zlnet.c - La carga remota por la red: lo común (ver zlnet.h).
 */
#include "zlnet.h"

#include <string.h>

#include "zlcrypto.h"

int zln_type(const uint8_t *p, size_t n)
{
    if (n < 4 || p[0] != 'Z' || p[1] != 'X' || p[2] != 'V')
        return 0;
    return p[3];
}

/* SHA-256 de una etiqueta y varios trozos. */
static void hash(const char *label, const uint8_t *const *parts, const size_t *lens, int n, uint8_t out[32])
{
    zl_sha256_t s;
    zl_sha256_init(&s);
    zl_sha256_update(&s, label, strlen(label));
    for (int i = 0; i < n; i++)
        zl_sha256_update(&s, parts[i], lens[i]);
    zl_sha256_final(&s, out);
}

void zln_commit(const uint8_t nm[ZLN_NONCE], const uint8_t pk_machine[32], const uint8_t pk_pc[32],
                uint8_t out[32])
{
    const uint8_t *parts[3] = { nm, pk_machine, pk_pc };
    const size_t lens[3] = { ZLN_NONCE, 32, 32 };
    hash("ZX-V commit", parts, lens, 3, out);
}

uint32_t zln_code(const uint8_t pk_pc[32], const uint8_t pk_machine[32], const uint8_t npc[ZLN_NONCE],
                  const uint8_t nm[ZLN_NONCE])
{
    const uint8_t *parts[4] = { pk_pc, pk_machine, npc, nm };
    const size_t lens[4] = { 32, 32, ZLN_NONCE, ZLN_NONCE };
    uint8_t h[32];
    hash("ZX-V code", parts, lens, 4, h);
    return ((uint32_t)h[0] << 24 | (uint32_t)h[1] << 16 | (uint32_t)h[2] << 8 | h[3]) % 1000000u;
}

void zln_key(const uint8_t shared[32], const uint8_t pk_pc[32], const uint8_t pk_machine[32],
             const uint8_t npc[ZLN_NONCE], const uint8_t nm[ZLN_NONCE], uint8_t out[ZLN_KEY])
{
    const uint8_t *parts[5] = { shared, pk_pc, pk_machine, npc, nm };
    const size_t lens[5] = { 32, 32, 32, ZLN_NONCE, ZLN_NONCE };
    hash("ZX-V key", parts, lens, 5, out);
}

/* La firma de un paquete de la red: HMAC-SHA-256(clave, "ZX-V net" |
 * ZLN_VERSION | el paquete). Con la versión dentro, una máquina y un PC de
 * versiones distintas no se aceptan nada. */
static void tag(const uint8_t *p, size_t n, const uint8_t key[ZLN_KEY], uint8_t out[32])
{
    static const uint8_t label[9] = { 'Z', 'X', '-', 'V', ' ', 'n', 'e', 't', ZLN_VERSION };
    const void *parts[2] = { label, p };
    const size_t lens[2] = { sizeof label, n };
    zl_hmac_sha256(key, ZLN_KEY, parts, lens, 2, out);
}

size_t zln_seal(uint8_t *buf, int type, const uint8_t id[ZLN_ID], const uint8_t boot[ZLN_BOOT], uint64_t counter,
                const uint8_t *frame, size_t n, const uint8_t key[ZLN_KEY])
{
    if (n > ZL_FRAME_MAX || (type != ZLN_DATA && type != ZLN_DATA_R))
        return 0;
    memcpy(buf, "ZXV", 3);
    buf[3] = (uint8_t)type;
    memcpy(buf + 4, id, ZLN_ID);
    memcpy(buf + 4 + ZLN_ID, boot, ZLN_BOOT);
    zln_put64(buf + 4 + ZLN_ID + ZLN_BOOT, counter);
    memmove(buf + ZLN_HEAD, frame, n);
    uint8_t t[32];
    tag(buf, ZLN_HEAD + n, key, t);
    memcpy(buf + ZLN_HEAD + n, t, ZLN_TAG);
    return ZLN_HEAD + n + ZLN_TAG;
}

int zln_open(const uint8_t *p, size_t n, int type, const uint8_t key[ZLN_KEY], const uint8_t **frame,
             size_t *frame_n, uint64_t *counter)
{
    if (zln_type(p, n) != type || (type != ZLN_DATA && type != ZLN_DATA_R) || n < ZLN_HEAD + ZLN_TAG)
        return 0;
    uint8_t t[32];
    tag(p, n - ZLN_TAG, key, t);
    if (zl_memcmp_ct(t, p + n - ZLN_TAG, ZLN_TAG))
        return 0;
    *frame = p + ZLN_HEAD;
    *frame_n = n - ZLN_HEAD - ZLN_TAG;
    *counter = zln_get64(p + 4 + ZLN_ID + ZLN_BOOT);
    return 1;
}

size_t zln_seal_boot(uint8_t *buf, const uint8_t id[ZLN_ID], const uint8_t boot[ZLN_BOOT], uint64_t counter,
                     const uint8_t key[ZLN_KEY])
{
    memcpy(buf, "ZXVb", 4);
    memcpy(buf + 4, id, ZLN_ID);
    memcpy(buf + 4 + ZLN_ID, boot, ZLN_BOOT);
    zln_put64(buf + 4 + ZLN_ID + ZLN_BOOT, counter);
    uint8_t t[32];
    tag(buf, ZLN_BOOT_SIZE - ZLN_TAG, key, t);
    memcpy(buf + ZLN_BOOT_SIZE - ZLN_TAG, t, ZLN_TAG);
    return ZLN_BOOT_SIZE;
}

int zln_open_boot(const uint8_t *p, size_t n, const uint8_t key[ZLN_KEY], uint8_t boot[ZLN_BOOT],
                  uint64_t *counter)
{
    if (zln_type(p, n) != ZLN_BOOT_T || n != ZLN_BOOT_SIZE)
        return 0;
    uint8_t t[32];
    tag(p, n - ZLN_TAG, key, t);
    if (zl_memcmp_ct(t, p + n - ZLN_TAG, ZLN_TAG))
        return 0;
    memcpy(boot, p + 4 + ZLN_ID, ZLN_BOOT);
    *counter = zln_get64(p + 4 + ZLN_ID + ZLN_BOOT);
    return 1;
}

void zln_proof(const char *label, const uint8_t key[ZLN_KEY], const uint8_t pc_id[ZLN_ID],
               const uint8_t machine_id[ZLN_ID], uint8_t out[ZLN_PROOF])
{
    const void *parts[3] = { label, pc_id, machine_id };
    const size_t lens[3] = { strlen(label), ZLN_ID, ZLN_ID };
    uint8_t h[32];
    zl_hmac_sha256(key, ZLN_KEY, parts, lens, 3, h);
    memcpy(out, h, ZLN_PROOF);
}
