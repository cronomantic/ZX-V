/*
 * netpc.c - La carga remota por la red, en el PC (ver netpc.h).
 */
#ifndef _WIN32
#define _DEFAULT_SOURCE                 /* getifaddrs */
#endif
#include "netpc.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#include <bcrypt.h>
#include <process.h>
typedef SOCKET sock_t;
#define BAD_SOCK INVALID_SOCKET
#define close_sock closesocket
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
typedef int sock_t;
#define BAD_SOCK (-1)
#define close_sock close
#endif

#include "zlcrypto.h"

#define REPLY_MS        400             /* lo que se espera cada respuesta */
#define TRIES           3
#define SENT            32              /* los contadores de los últimos D, para creer un b */
/*
 * El contador del PC nunca baja para una máquina (zlnet.h), aunque el reloj
 * del PC se atrase entre una ejecución y otra o mientras tanto: se apunta en
 * el PC, con la hora a la que se apunta, cada vez que sube CTR_STEP (un
 * segundo, si sale del reloj) o CTR_STEP_BEHIND (si va por delante del
 * reloj: el reloj se ha atrasado y sube de uno en uno). Al empezar, se
 * sigue por encima de lo apuntado:
 *   - si el reloj no ha bajado desde entonces, lo que se usó después de
 *     apuntarlo (hasta un Ctrl+C) salió del reloj, que ya va por delante, o
 *     va como mucho CTR_STEP_BEHIND por encima: se sigue desde ahí (o el
 *     reloj). Otro programa del mismo PC que esté en marcha no se queda
 *     atrás: la máquina tira lo que no pasa de su suelo, y un programa que
 *     empezara muy por delante dejaría sordo al otro;
 *   - si ha bajado, CTR_STEP por encima.
 */
#define CTR_STEP        ((uint64_t)1000 << 10)
#define CTR_STEP_BEHIND 256u

struct netpc {
    sock_t fd;
    struct sockaddr_in to;
    char cfg[1024];
    char name[ZLN_NAME + 1];
    char addr[24];
    uint8_t pc_id[ZLN_ID], machine_id[ZLN_ID], boot[ZLN_BOOT], key[ZLN_KEY];
    bool paired;
    uint64_t rx_counter, tx_counter;
    uint64_t lane;                      /* la vía de este programa en el contador (zlnet.h) */
    uint64_t ctr_saved;                 /* el último contador apuntado en el PC (CTR_STEP) */
    char ctr_file[1040];                /* dónde */
    uint64_t sent[SENT];                /* los contadores de los últimos D con este arranque */
    unsigned sent_n;
    uint8_t last[ZL_FRAME_MAX];         /* la última trama, por si hay que repetirla con otro arranque */
    size_t last_n;
    /* El emparejamiento en curso. */
    uint8_t sk[32], pk[32], pk_m[32], npc[ZLN_NONCE], nm[ZLN_NONCE], new_key[ZLN_KEY];
    bool pairing;
};

/* --- El sistema --------------------------------------------------------------------------- */

static void net_init(void)
{
#ifdef _WIN32
    static bool done;
    if (!done) {
        WSADATA w;
        WSAStartup(MAKEWORD(2, 2), &w);
        done = true;
    }
#endif
}

static void os_random(uint8_t *p, size_t n)
{
#ifdef _WIN32
    BCryptGenRandom(NULL, p, (ULONG)n, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
#else
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f || fread(p, 1, n, f) != n) {
        for (size_t i = 0; i < n; i++)  /* no debería pasar */
            p[i] = (uint8_t)(rand() ^ (int)time(NULL));
    }
    if (f)
        fclose(f);
#endif
}

static uint64_t now_ms(void)
{
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    return (((uint64_t)ft.dwHighDateTime << 32 | ft.dwLowDateTime) / 10000u);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000u + (uint64_t)tv.tv_usec / 1000u;
#endif
}

static sock_t udp_socket(bool broadcast)
{
    net_init();
    sock_t fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd == BAD_SOCK)
        return fd;
    if (broadcast) {
        int on = 1;
        setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (const char *)&on, sizeof on);
    }
    return fd;
}

/* Lo que llega en ms: cuántos bytes (de *from), 0 si nada, -1 si falla. */
static int recv_wait(sock_t fd, uint8_t *buf, size_t n, unsigned ms, struct sockaddr_in *from)
{
    uint64_t end = now_ms() + ms;
    for (;;) {
        uint64_t t = now_ms();
        unsigned left = t < end ? (unsigned)(end - t) : 0;
        fd_set set;
        FD_ZERO(&set);
        FD_SET(fd, &set);
        struct timeval tv = { (long)(left / 1000), (long)(left % 1000) * 1000 };
        int r = select((int)fd + 1, &set, NULL, NULL, &tv);
        if (r <= 0)
            return r < 0 ? -1 : 0;
        socklen_t fl = sizeof *from;
        int k = (int)recvfrom(fd, (char *)buf, (int)n, 0, (struct sockaddr *)from, &fl);
#ifdef _WIN32
        if (k < 0) {
            /* El ICMP de un paquete anterior, o un paquete más grande que
             * buf (cualquiera de la red puede mandarlo; Windows lo tira y da
             * un error, otros lo cortan): no es nuestro, y se sigue
             * esperando. */
            int e = WSAGetLastError();
            if (e == WSAECONNRESET || e == WSAEMSGSIZE)
                continue;
        }
#endif
        return k < 0 ? -1 : k;
    }
}

/* --- Lo que recuerda este PC ---------------------------------------------------------------- */

static void to_hex(const uint8_t *p, size_t n, char *out)
{
    for (size_t i = 0; i < n; i++)
        sprintf(out + 2 * i, "%02x", p[i]);
}

static bool from_hex(const char *s, uint8_t *out, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        unsigned v;
        if (!isxdigit((unsigned char)s[2 * i]) || !isxdigit((unsigned char)s[2 * i + 1]) ||
            sscanf(s + 2 * i, "%2x", &v) != 1)
            return false;
        out[i] = (uint8_t)v;
    }
    return true;
}

/* El id de este PC (si no hay, uno nuevo, y se guarda) y, si machine no es
 * NULL, la clave de esa máquina (true si la hay). */
static bool cfg_load(const char *cfg, uint8_t pc_id[ZLN_ID], const uint8_t *machine, uint8_t key[ZLN_KEY])
{
    bool have_id = false, have_key = false;
    FILE *f = fopen(cfg, "r");
    char line[256];
    while (f && fgets(line, sizeof line, f)) {
        if (!strncmp(line, "id ", 3) && from_hex(line + 3, pc_id, ZLN_ID))
            have_id = true;
        uint8_t mid[ZLN_ID];
        if (machine && !strncmp(line, "pair ", 5) && from_hex(line + 5, mid, ZLN_ID) &&
            !memcmp(mid, machine, ZLN_ID) && line[5 + 2 * ZLN_ID] == ' ' &&
            from_hex(line + 6 + 2 * ZLN_ID, key, ZLN_KEY))
            have_key = true;
    }
    if (f)
        fclose(f);
    if (!have_id) {
        os_random(pc_id, ZLN_ID);
        f = fopen(cfg, "a");
        if (f) {
            char h[2 * ZLN_ID + 1];
            to_hex(pc_id, ZLN_ID, h);
            fprintf(f, "id %s\n", h);
            fclose(f);
        }
    }
    return have_key;
}

/* La clave de una máquina, en lugar de la que hubiera. */
static void cfg_save_pair(const char *cfg, const uint8_t machine[ZLN_ID], const uint8_t key[ZLN_KEY], const char *name)
{
    char mid[2 * ZLN_ID + 1], k[2 * ZLN_KEY + 1];
    to_hex(machine, ZLN_ID, mid);
    to_hex(key, ZLN_KEY, k);
    char *keep = NULL;
    size_t used = 0;
    FILE *f = fopen(cfg, "r");
    char line[256];
    while (f && fgets(line, sizeof line, f)) {
        if (!strncmp(line, "pair ", 5) && !strncmp(line + 5, mid, 2 * ZLN_ID))
            continue;
        size_t l = strlen(line);
        char *grown = realloc(keep, used + l + 1);
        if (!grown)
            break;
        keep = grown;
        memcpy(keep + used, line, l + 1);
        used += l;
    }
    if (f)
        fclose(f);
    f = fopen(cfg, "w");
    if (f) {
        if (keep)
            fputs(keep, f);
        fprintf(f, "pair %s %s %s\n", mid, k, name);
        fclose(f);
    }
    free(keep);
}

/* El contador de cada máquina, apuntado aparte (cfg sin ".txt", con
 * "-counters.txt"): una línea "id contador hora" por máquina, en
 * hexadecimal (la hora, en ms desde 1970). Se reescribe entero en un
 * temporal que luego pasa a su nombre: un corte deja el de antes o el
 * nuevo, nunca uno a medias, y las claves no se tocan. */
static void ctr_file(const char *cfg, char *out, size_t n)
{
    size_t l = strlen(cfg);
    if (l >= 4 && !strcmp(cfg + l - 4, ".txt"))
        l -= 4;
    snprintf(out, n, "%.*s-counters.txt", (int)l, cfg);
}

/* Una línea del fichero: si es de mid, su contador y su hora. */
static bool ctr_line(const char *line, const char *mid, uint64_t *c, uint64_t *when)
{
    unsigned long long a, b = 0;
    if (strncmp(line, mid, 2 * ZLN_ID) || line[2 * ZLN_ID] != ' ' ||
        sscanf(line + 2 * ZLN_ID + 1, "%llx %llx", &a, &b) < 1)
        return false;
    *c = a;
    *when = b;
    return true;
}

/* Lo apuntado para machine (0 si nada) y cuándo. */
static uint64_t ctr_load(const char *file, const uint8_t machine[ZLN_ID], uint64_t *when)
{
    char mid[2 * ZLN_ID + 1], line[128];
    to_hex(machine, ZLN_ID, mid);
    uint64_t v = 0, c, w;
    *when = 0;
    FILE *f = fopen(file, "r");
    while (f && fgets(line, sizeof line, f))
        if (ctr_line(line, mid, &c, &w) && c > v) {
            v = c;
            *when = w;
        }
    if (f)
        fclose(f);
    return v;
}

/* Apuntar v para machine (si lo apuntado no es ya más alto: otro programa
 * del mismo PC puede ir por delante). */
static void ctr_save(const char *file, const uint8_t machine[ZLN_ID], uint64_t v)
{
    char mid[2 * ZLN_ID + 1], line[128], tmp[1100];
    to_hex(machine, ZLN_ID, mid);
#ifdef _WIN32
    snprintf(tmp, sizeof tmp, "%s.%d.tmp", file, _getpid());
#else
    snprintf(tmp, sizeof tmp, "%s.%d.tmp", file, (int)getpid());
#endif
    FILE *out = fopen(tmp, "w");
    if (!out)
        return;
    uint64_t when = now_ms(), c, w;
    FILE *f = fopen(file, "r");
    while (f && fgets(line, sizeof line, f)) {
        if (ctr_line(line, mid, &c, &w)) {
            if (c > v) {
                v = c;
                when = w;
            }
            continue;
        }
        if (strchr(line, '\n'))
            fputs(line, out);           /* las de las otras máquinas, tal cual */
    }
    if (f)
        fclose(f);
    fprintf(out, "%s %016llx %llx\n", mid, (unsigned long long)v, (unsigned long long)when);
    bool ok = fflush(out) == 0;
    ok &= fclose(out) == 0;
#ifdef _WIN32
    ok = ok && MoveFileExA(tmp, file, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
#else
    ok = ok && rename(tmp, file) == 0;
#endif
    if (!ok)
        remove(tmp);
}

/* --- Buscar ------------------------------------------------------------------------------- */

static void send_to(sock_t fd, uint32_t ip_be, uint16_t port, const uint8_t *p, size_t n)
{
    struct sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = ip_be;
    sendto(fd, (const char *)p, (int)n, 0, (const struct sockaddr *)&a, sizeof a);
}

/* A la dirección de difusión de cada red del equipo (y a la general). */
static void send_broadcast(sock_t fd, const uint8_t *p, size_t n)
{
    send_to(fd, htonl(INADDR_BROADCAST), ZLN_PORT, p, n);
#ifdef _WIN32
    ULONG size = 16384;
    IP_ADAPTER_ADDRESSES *list = malloc(size);
    if (list && GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
                                     NULL, list, &size) == NO_ERROR) {
        for (IP_ADAPTER_ADDRESSES *a = list; a; a = a->Next) {
            if (a->OperStatus != IfOperStatusUp)
                continue;
            for (IP_ADAPTER_UNICAST_ADDRESS *u = a->FirstUnicastAddress; u; u = u->Next) {
                struct sockaddr_in *s = (struct sockaddr_in *)u->Address.lpSockaddr;
                unsigned bits = u->OnLinkPrefixLength;
                if (s->sin_family != AF_INET || bits == 0 || bits >= 32)
                    continue;
                uint32_t ip = ntohl(s->sin_addr.s_addr), mask = 0xFFFFFFFFu << (32 - bits);
                send_to(fd, htonl(ip | ~mask), ZLN_PORT, p, n);
            }
        }
    }
    free(list);
#else
    struct ifaddrs *ifs;
    if (getifaddrs(&ifs) == 0) {
        for (struct ifaddrs *i = ifs; i; i = i->ifa_next)
            if (i->ifa_addr && i->ifa_addr->sa_family == AF_INET && (i->ifa_flags & IFF_BROADCAST) && i->ifa_broadaddr)
                send_to(fd, ((struct sockaddr_in *)i->ifa_broadaddr)->sin_addr.s_addr, ZLN_PORT, p, n);
        freeifaddrs(ifs);
    }
#endif
}

/* Un f: lo que dice la máquina. */
static bool parse_found(const uint8_t *p, int n, uint8_t id[ZLN_ID], uint8_t boot[ZLN_BOOT], netpc_found_t *f)
{
    if (zln_type(p, (size_t)n) != ZLN_FOUND || n < 4 + ZLN_ID + ZLN_BOOT + 1 + 2 + ZLN_NAME)
        return false;
    const uint8_t *q = p + 4;
    memcpy(id, q, ZLN_ID);
    memcpy(boot, q + ZLN_ID, ZLN_BOOT);
    f->paired = q[ZLN_ID + ZLN_BOOT] != 0;
    q += ZLN_ID + ZLN_BOOT + 1 + 2;
    memcpy(f->name, q, ZLN_NAME);
    f->name[ZLN_NAME] = 0;
    q += ZLN_NAME;
    size_t k = (size_t)(p + n - q);
    /* Detrás del firmware, un 0 y la versión de la red; sin ellos, la 1. */
    const uint8_t *z = memchr(q, 0, k);
    f->version = 1;
    if (z) {
        f->version = z + 1 < p + n ? z[1] : 0;
        k = (size_t)(z - q);
    }
    if (k > sizeof f->firmware - 1)
        k = sizeof f->firmware - 1;
    memcpy(f->firmware, q, k);
    f->firmware[k] = 0;
    for (char *c = f->name; *c; c++)
        if ((unsigned char)*c < 32 || (unsigned char)*c > 126)
            *c = '?';
    for (char *c = f->firmware; *c; c++)
        if ((unsigned char)*c < 32 || (unsigned char)*c > 126)
            *c = '?';
    return true;
}

static void addr_text(const struct sockaddr_in *a, char *out)
{
    uint32_t ip = ntohl(a->sin_addr.s_addr);
    int k = sprintf(out, "%u.%u.%u.%u", ip >> 24, (ip >> 16) & 255, (ip >> 8) & 255, ip & 255);
    if (ntohs(a->sin_port) != ZLN_PORT)
        sprintf(out + k, ":%u", ntohs(a->sin_port));
}

int netpc_find(const char *cfg, netpc_found_t *out, int max, unsigned ms)
{
    uint8_t pc_id[ZLN_ID];
    cfg_load(cfg, pc_id, NULL, NULL);
    sock_t fd = udp_socket(true);
    if (fd == BAD_SOCK)
        return 0;
    uint8_t q[4 + ZLN_ID + 1] = { 'Z', 'X', 'V', ZLN_FIND };
    memcpy(q + 4, pc_id, ZLN_ID);
    q[4 + ZLN_ID] = ZLN_VERSION;
    uint8_t ids[16][ZLN_ID];
    int found = 0;
    uint64_t end = now_ms() + ms;
    for (int round = 0; round < 2; round++) {   /* dos veces: un paquete se puede perder */
        send_broadcast(fd, q, sizeof q);
        uint64_t until = round ? end : now_ms() + ms / 2;
        for (uint64_t t = now_ms(); t < until && found < max; t = now_ms()) {
            uint8_t buf[256];
            struct sockaddr_in from;
            int n = recv_wait(fd, buf, sizeof buf, (unsigned)(until - t), &from);
            if (n <= 0)
                continue;
            uint8_t id[ZLN_ID], boot[ZLN_BOOT];
            netpc_found_t f;
            if (!parse_found(buf, n, id, boot, &f))
                continue;
            bool dup = false;
            for (int i = 0; i < found && i < 16; i++)
                dup |= !memcmp(ids[i], id, ZLN_ID);
            if (dup)
                continue;
            uint8_t key[ZLN_KEY];
            f.paired = f.paired && cfg_load(cfg, pc_id, id, key);
            addr_text(&from, f.addr);
            if (found < 16)
                memcpy(ids[found], id, ZLN_ID);
            out[found++] = f;
        }
    }
    close_sock(fd);
    return found;
}

/* --- Hablar con una --------------------------------------------------------------------------- */

bool netpc_is_target(const char *port)
{
    const char *p = strchr(port, '(');
    if (p)
        port = p + 1;
    unsigned a, b, c, d;
    if (sscanf(port, "%u.%u.%u.%u", &a, &b, &c, &d) == 4 && a < 256 && b < 256 && c < 256 && d < 256)
        return true;
    return !strncmp(port, "ZX-V-", 5) || !strncmp(port, "zx-v-", 5);
}

/* La dirección de target: la IP (de "ZX-V-1A2B (IP)" también) o, con un
 * nombre, la de la máquina que lo tiene. */
static bool resolve(const char *cfg, const char *target, struct sockaddr_in *to, char *why)
{
    memset(to, 0, sizeof *to);
    to->sin_family = AF_INET;
    to->sin_port = htons(ZLN_PORT);
    const char *p = strchr(target, '(');
    char ip[64];
    if (p)
        target = p + 1;
    unsigned a, b, c, d, port = ZLN_PORT;
    if (sscanf(target, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
        const char *colon = strchr(target, ':');
        if (colon)
            port = (unsigned)strtoul(colon + 1, NULL, 10);
        snprintf(ip, sizeof ip, "%u.%u.%u.%u", a, b, c, d);
        to->sin_addr.s_addr = inet_addr(ip);
        to->sin_port = htons((uint16_t)port);
        return true;
    }
    netpc_found_t list[16];
    int n = netpc_find(cfg, list, 16, 1500);
    for (int i = 0; i < n; i++)
        if (!strcmp(list[i].name, target))
            return resolve(cfg, list[i].addr, to, why);
    snprintf(why, 160, "%s", target);
    return false;
}

const char *netpc_name(const netpc_t *n) { return n->name; }
const char *netpc_addr(const netpc_t *n) { return n->addr; }

static void send_raw(netpc_t *n, const uint8_t *p, size_t k)
{
    sendto(n->fd, (const char *)p, (int)k, 0, (const struct sockaddr *)&n->to, sizeof n->to);
}

/* Lo que contesta la máquina de tipo type a lo que se manda (TRIES veces). */
static int ask(netpc_t *n, const uint8_t *q, size_t qn, int type, uint8_t *buf, size_t max, unsigned ms)
{
    for (int t = 0; t < TRIES; t++) {
        send_raw(n, q, qn);
        uint64_t end = now_ms() + ms;
        for (uint64_t now = now_ms(); now < end; now = now_ms()) {
            struct sockaddr_in from;
            int k = recv_wait(n->fd, buf, max, (unsigned)(end - now), &from);
            if (k < 0)
                return -1;
            if (k > 0 && from.sin_addr.s_addr == n->to.sin_addr.s_addr && zln_type(buf, (size_t)k) == type)
                return k;
        }
    }
    return 0;
}

/* La máquina de to, si contesta (y, con name, si es la que se llama así). */
static bool hello(netpc_t *n, const char *name, netpc_found_t *f)
{
    addr_text(&n->to, n->addr);
    uint8_t q[4 + ZLN_ID + 1] = { 'Z', 'X', 'V', ZLN_FIND }, buf[256];
    memcpy(q + 4, n->pc_id, ZLN_ID);
    q[4 + ZLN_ID] = ZLN_VERSION;
    int k = ask(n, q, sizeof q, ZLN_FOUND, buf, sizeof buf, REPLY_MS);
    return k > 0 && parse_found(buf, k, n->machine_id, n->boot, f) && (!name || !strcmp(f->name, name));
}

netpc_t *netpc_open(const char *cfg, const char *target, bool *paired, char *why, unsigned *version)
{
    if (version)
        *version = 0;
    netpc_t *n = calloc(1, sizeof *n);
    if (!n)
        return NULL;
    snprintf(n->cfg, sizeof n->cfg, "%s", cfg);
    cfg_load(cfg, n->pc_id, NULL, NULL);
    n->fd = udp_socket(false);
    /* "ZX-V-1A2B (IP)", como lo dicen zxv-link ports y la lista de la
     * ventana (y lo guardan): si en esa IP ya no está (el router le ha dado
     * otra), se busca por el nombre. */
    char name[ZLN_NAME + 1] = "";
    const char *paren = strchr(target, '(');
    if (paren && !strncmp(target, "ZX-V-", 5)) {
        size_t l = strcspn(target, " (");
        if (l <= ZLN_NAME)
            snprintf(name, sizeof name, "%.*s", (int)l, target);
    }
    netpc_found_t f;
    bool ok = n->fd != BAD_SOCK && resolve(cfg, target, &n->to, why) && hello(n, name[0] ? name : NULL, &f);
    if (!ok && n->fd != BAD_SOCK && name[0])
        ok = resolve(cfg, name, &n->to, why) && hello(n, name, &f);
    if (!ok) {
        if (n->fd != BAD_SOCK) {
            if (n->addr[0])
                snprintf(why, 160, "%s", n->addr);
            close_sock(n->fd);
        }
        free(n);
        return NULL;
    }
    if (version)
        *version = f.version;
    if (f.version != ZLN_VERSION) {     /* otra versión de la red: no se entenderían */
        snprintf(why, 160, "%s", n->addr);
        close_sock(n->fd);
        free(n);
        return NULL;
    }
    snprintf(n->name, sizeof n->name, "%s", f.name);
    n->paired = f.paired && cfg_load(cfg, n->pc_id, n->machine_id, n->key);
    uint8_t lane;
    os_random(&lane, 1);
    n->lane = (uint64_t)(lane & 63) << 4;
    /* Por encima de lo que se apuntó la última vez (zlnet.h). */
    ctr_file(cfg, n->ctr_file, sizeof n->ctr_file);
    uint64_t when;
    n->ctr_saved = ctr_load(n->ctr_file, n->machine_id, &when);
    if (n->ctr_saved)
        n->tx_counter = n->ctr_saved + (now_ms() >= when ? CTR_STEP_BEHIND : CTR_STEP);
    *paired = n->paired;
    return n;
}

void netpc_close(netpc_t *n)
{
    if (!n)
        return;
    if (n->tx_counter > n->ctr_saved)
        ctr_save(n->ctr_file, n->machine_id, n->tx_counter);
    close_sock(n->fd);
    memset(n, 0, sizeof *n);
    free(n);
}

static void send_frame(netpc_t *n, const uint8_t *frame, size_t len)
{
    uint8_t buf[ZLN_DATAGRAM_MAX];
    /* Del reloj, con la vía de este programa: otro del mismo PC no da el
     * mismo en el mismo ms. Sube siempre, también entre ejecuciones (lo
     * apuntado) y aunque el reloj se atrase. */
    uint64_t c = ((uint64_t)now_ms() << 10) | n->lane;
    n->tx_counter = c > n->tx_counter ? c : n->tx_counter + 1;
    bool behind = n->tx_counter >= c + 1024;    /* el reloj se ha atrasado: de uno en uno */
    if (n->tx_counter >= n->ctr_saved + (behind ? CTR_STEP_BEHIND : CTR_STEP)) {
        ctr_save(n->ctr_file, n->machine_id, n->tx_counter);
        n->ctr_saved = n->tx_counter;
    }
    n->sent[n->sent_n++ % SENT] = n->tx_counter;
    size_t k = zln_seal(buf, ZLN_DATA, n->pc_id, n->boot, n->tx_counter, frame, len, n->key);
    if (k)
        send_raw(n, buf, k);
}

int netpc_write(netpc_t *n, const uint8_t *frame, size_t len)
{
    if (!n->paired || len > sizeof n->last)
        return -1;
    memcpy(n->last, frame, len);
    n->last_n = len;
    send_frame(n, frame, len);
    return (int)len;
}

/* Si c es de un D de este programa con el arranque que tiene (de los
 * últimos SENT). */
static bool sent_recently(const netpc_t *n, uint64_t c)
{
    unsigned k = n->sent_n < SENT ? n->sent_n : SENT;
    for (unsigned i = 0; i < k; i++)
        if (n->sent[i] == c)
            return true;
    return false;
}

int netpc_read(netpc_t *n, uint8_t *buf, size_t max, unsigned timeout_ms)
{
    uint64_t end = now_ms() + timeout_ms;
    for (;;) {
        uint64_t now = now_ms();
        uint8_t p[ZLN_DATAGRAM_MAX + 16];
        struct sockaddr_in from;
        int k = recv_wait(n->fd, p, sizeof p, now < end ? (unsigned)(end - now) : 0, &from);
        if (k < 0)
            return -1;
        if (k == 0)
            return 0;
        if (from.sin_addr.s_addr != n->to.sin_addr.s_addr || !n->paired)
            continue;
        int type = zln_type(p, (size_t)k);
        if (type == ZLN_BOOT_T) {
            /* Se ha reiniciado: su arranque nuevo y, otra vez, lo último.
             * Solo si va bien firmado, contesta a un D nuestro con el
             * arranque que tenemos y trae otro: uno grabado o falso no hace
             * repetir nada ni deja volver a aceptar lo ya recibido. */
            uint8_t nb[ZLN_BOOT];
            uint64_t c;
            if (!memcmp(p + 4, n->pc_id, ZLN_ID) && zln_open_boot(p, (size_t)k, n->key, nb, &c) &&
                memcmp(nb, n->boot, ZLN_BOOT) && sent_recently(n, c)) {
                memcpy(n->boot, nb, ZLN_BOOT);
                n->rx_counter = 0;
                n->sent_n = 0;          /* lo mandado con el de antes ya no cuenta */
                if (n->last_n)
                    send_frame(n, n->last, n->last_n);
            }
            continue;
        }
        const uint8_t *frame;
        size_t fn;
        uint64_t counter;
        if (type != ZLN_DATA_R || memcmp(p + 4, n->pc_id, ZLN_ID) || memcmp(p + 4 + ZLN_ID, n->boot, ZLN_BOOT) ||
            !zln_open(p, (size_t)k, ZLN_DATA_R, n->key, &frame, &fn, &counter) || counter <= n->rx_counter ||
            fn > max)
            continue;
        n->rx_counter = counter;
        memcpy(buf, frame, fn);
        return (int)fn;
    }
}

/* --- Emparejar ---------------------------------------------------------------------------- */

int netpc_pair_begin(netpc_t *n, const char *pc_name, uint32_t *code, char *why)
{
    uint8_t q[4 + ZLN_ID + 32 + 32], buf[256];
    os_random(n->sk, 32);
    zl_x25519_base(n->pk, n->sk, NULL, NULL);
    size_t nl = strlen(pc_name);
    if (nl > 23)
        nl = 23;
    memcpy(q, "ZXVP", 4);
    memcpy(q + 4, n->pc_id, ZLN_ID);
    memcpy(q + 4 + ZLN_ID, n->pk, 32);
    memcpy(q + 4 + ZLN_ID + 32, pc_name, nl);
    int k = ask(n, q, 4 + ZLN_ID + 32 + nl, ZLN_PAIR_R, buf, sizeof buf, 1500);   /* la máquina calcula X25519 */
    if (k <= 0) {
        snprintf(why, 160, "%s", n->addr);
        return -100;
    }
    if (k < 5 || buf[4] != ZLN_PAIR_WAIT || k < 5 + 64)
        return k >= 5 ? -(int)buf[4] : -ZLN_PAIR_NO;
    uint8_t commit[32];
    memcpy(n->pk_m, buf + 5, 32);
    memcpy(commit, buf + 37, 32);
    os_random(n->npc, ZLN_NONCE);
    uint8_t qn[4 + ZLN_ID + ZLN_NONCE];
    memcpy(qn, "ZXVN", 4);
    memcpy(qn + 4, n->pc_id, ZLN_ID);
    memcpy(qn + 4 + ZLN_ID, n->npc, ZLN_NONCE);
    k = ask(n, qn, sizeof qn, ZLN_NONCE_R, buf, sizeof buf, 1500);
    if (k <= 0) {
        snprintf(why, 160, "%s", n->addr);
        return -100;
    }
    if (k < 5 + ZLN_NONCE || buf[4] != ZLN_PAIR_WAIT)
        return k >= 5 ? -(int)buf[4] : -ZLN_PAIR_NO;
    memcpy(n->nm, buf + 5, ZLN_NONCE);
    uint8_t check[32];
    zln_commit(n->nm, n->pk_m, n->pk, check);
    if (zl_memcmp_ct(check, commit, 32)) {
        snprintf(why, 160, "commit");     /* no cumple lo que prometió: alguien en medio */
        return -ZLN_PAIR_NO;
    }
    *code = zln_code(n->pk, n->pk_m, n->npc, n->nm);
    uint8_t shared[32];
    zl_x25519(shared, n->sk, n->pk_m, NULL, NULL);
    zln_key(shared, n->pk, n->pk_m, n->npc, n->nm, n->new_key);
    memset(shared, 0, sizeof shared);
    memset(n->sk, 0, sizeof n->sk);
    n->pairing = true;
    return 0;
}

int netpc_pair_poll(netpc_t *n)
{
    if (!n->pairing)
        return ZLN_PAIR_NO;
    /* Con la prueba de qué clave tiene este PC: la máquina dice que sí solo
     * si la ha guardado, y lo prueba (zlnet.h). */
    uint8_t q[4 + ZLN_ID + ZLN_PROOF], buf[64], want[ZLN_PROOF];
    memcpy(q, "ZXVQ", 4);
    memcpy(q + 4, n->pc_id, ZLN_ID);
    zln_proof("ZX-V query", n->new_key, n->pc_id, n->machine_id, q + 4 + ZLN_ID);
    int k = ask(n, q, sizeof q, ZLN_QUERY_R, buf, sizeof buf, REPLY_MS);
    if (k < 5)
        return ZLN_PAIR_WAIT;           /* sin respuesta: se sigue esperando */
    if (buf[4] == ZLN_PAIR_OK) {
        zln_proof("ZX-V paired", n->new_key, n->pc_id, n->machine_id, want);
        if (k < 5 + ZLN_PROOF || zl_memcmp_ct(want, buf + 5, ZLN_PROOF))
            return ZLN_PAIR_WAIT;       /* no lo prueba: no es la máquina */
        memcpy(n->key, n->new_key, ZLN_KEY);
        n->paired = true;
        n->pairing = false;
        n->rx_counter = 0;
        n->sent_n = 0;
        cfg_save_pair(n->cfg, n->machine_id, n->key, n->name);
    } else if (buf[4] == ZLN_PAIR_NO) {
        n->pairing = false;
    } else {
        return ZLN_PAIR_WAIT;
    }
    return buf[4];
}
