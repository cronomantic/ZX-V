/*
 * zxvlink.c - Las tramas de la carga remota (ver zxvlink.h).
 */
#include "zxvlink.h"

#include <string.h>

uint32_t zl_crc32(uint32_t crc, const void *data, size_t n)
{
    const uint8_t *p = (const uint8_t *)data;
    crc = ~crc;
    while (n--) {
        crc ^= *p++;
        for (int k = 0; k < 8; k++)
            crc = crc >> 1 ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

/* COBS, byte a byte: cada cero, el número de bytes hasta el siguiente. */
typedef struct {
    uint8_t *out;
    size_t o, code_at;
    uint8_t code;
} cobs_t;

static void cobs_byte(cobs_t *c, uint8_t b)
{
    if (b == 0) {
        c->out[c->code_at] = c->code;
        c->code_at = c->o++;
        c->code = 1;
        return;
    }
    c->out[c->o++] = b;
    if (++c->code == 0xFF) {
        c->out[c->code_at] = c->code;
        c->code_at = c->o++;
        c->code = 1;
    }
}

size_t zl_pack2(uint8_t ch, uint8_t type, uint8_t seq, const void *head, unsigned hlen, const void *data,
                unsigned len, uint8_t *out)
{
    if (hlen > ZL_MAX_DATA || len > ZL_MAX_DATA - hlen)
        return 0;
    const uint8_t frame[3] = { ch, type, seq }, *h = (const uint8_t *)head, *d = (const uint8_t *)data;
    uint8_t crc[4];
    zl_put32(crc, zl_crc32(zl_crc32(zl_crc32(0, frame, 3), h, hlen), d, len));
    out[0] = 0;
    cobs_t c = { out, 2, 1, 1 };
    for (int i = 0; i < 3; i++)
        cobs_byte(&c, frame[i]);
    for (unsigned i = 0; i < hlen; i++)
        cobs_byte(&c, h[i]);
    for (unsigned i = 0; i < len; i++)
        cobs_byte(&c, d[i]);
    for (int i = 0; i < 4; i++)
        cobs_byte(&c, crc[i]);
    out[c.code_at] = c.code;
    out[c.o++] = 0;
    return c.o;
}

size_t zl_pack(uint8_t ch, uint8_t type, uint8_t seq, const void *data, unsigned len, uint8_t *out)
{
    return zl_pack2(ch, type, seq, data, len, NULL, 0, out);
}

void zl_rx_init(zl_rx_t *rx)
{
    memset(rx, 0, sizeof(*rx));
}

/* Deshace COBS en buf (len bytes), en su sitio (lo de dentro nunca ocupa
 * más): cuántos bytes, o 0 si está mal. */
static size_t uncobs(uint8_t *buf, size_t len)
{
    size_t i = 0, o = 0;
    while (i < len) {
        uint8_t code = buf[i++];
        if (code == 0 || i + code - 1 > len)
            return 0;
        for (unsigned k = 1; k < code; k++)
            buf[o++] = buf[i++];
        if (code != 0xFF && i < len)
            buf[o++] = 0;
    }
    return o;
}

bool zl_feed(zl_rx_t *rx, uint8_t byte, zl_msg_t *m)
{
    if (byte != 0) {
        if (rx->len < sizeof rx->buf)
            rx->buf[rx->len++] = byte;
        else
            rx->overflow = true;
        return false;
    }
    /* Un cero: acaba la trama (dos seguidos, entre tramas, no cuentan). */
    size_t len = rx->len;
    bool over = rx->overflow;
    rx->len = 0;
    rx->overflow = false;
    if (len == 0)
        return false;
    size_t n = over ? 0 : uncobs(rx->buf, len);
    if (n < 3 + 4 || n > ZL_RAW_MAX || zl_crc32(0, rx->buf, n - 4) != zl_get32(rx->buf + n - 4)) {
        rx->bad++;
        return false;
    }
    rx->good++;
    m->ch = rx->buf[0];
    m->type = rx->buf[1];
    m->seq = rx->buf[2];
    m->data = rx->buf + 3;
    m->len = (unsigned)(n - 3 - 4);
    return true;
}
