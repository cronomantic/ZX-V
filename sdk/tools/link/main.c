/*
 * zxv-link - La carga remota de ZX-V (D101): habla con la máquina por la tira
 * J5 de la placa (su UART), con un adaptador USB-serie de 3,3 V. La
 * conversación, en linkcore.c (la comparte zxv-link-gui, con ventana).
 *
 *   zxv-link ports                         los puertos serie que hay
 *   zxv-link --port PUERTO info            la versión de la máquina
 *   zxv-link --port PUERTO send FICHERO [DESTINO]   a la tarjeta
 *   zxv-link --port PUERTO run FICHERO [MÁS...]   a la tarjeta (con los
 *                                          ficheros que carga), y lo ejecuta
 *                                          (si hay un programa en marcha,
 *                                          antes lo para)
 *   zxv-link --port PUERTO log             el informe de la máquina (Ctrl+C para)
 *   zxv-link --port PUERTO serve [--folder CARPETA [--run PROGRAMA]]
 *            [--basic-debug PUERTO_TCP] [FICHERO...]
 *                                          sirve CARPETA como la unidad EXT: de
 *                                          la máquina (spec §7.7) hasta Ctrl+C
 *                                          (con --run, antes ejecuta PROGRAMA,
 *                                          de dentro de CARPETA, desde EXT:);
 *                                          y/o el depurador del BASIC por TCP,
 *                                          como el emulador (lo usa la
 *                                          extensión de VS Code)
 *
 * Lo que ya está en la tarjeta igual (el mismo tamaño y el mismo CRC) no se
 * vuelve a mandar. Lo que hay distinto con el mismo nombre se sustituye si
 * lo puso zxv-link (lo apunta en el PC); si no, no se toca nada y sale con
 * el código 4, salvo con --overwrite. PUERTO: COM5 en Windows, /dev/ttyUSB0
 * en Linux. Los mensajes, en el idioma del sistema (español o inglés), o el
 * de --lang es|en. Si el puerto no se puede abrir, sale con el código 3 (la
 * extensión ofrece elegir otro).
 *
 * MIT, como el resto del SDK.
 */
#ifndef _WIN32
#define _DEFAULT_SOURCE
#endif
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "linkcore.h"
#include "serial.h"
#include "tcp.h"
#include "zxvlink.h"

#define EXIT_PORT       3               /* el puerto no se puede abrir */
#define EXIT_EXISTS     4               /* sustituiría lo que no puso zxv-link */

#define TR(en, s) (lc_spanish ? (s) : (en))

static void detect_lang(void)
{
#ifdef _WIN32
    lc_spanish = PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_SPANISH;
#else
    const char *l = getenv("LC_ALL");
    if (!l || !*l)
        l = getenv("LC_MESSAGES");
    if (!l || !*l)
        l = getenv("LANG");
    lc_spanish = l && !strncmp(l, "es", 2);
#endif
}

static unsigned long now_ms(void)
{
#ifdef _WIN32
    return (unsigned long)GetTickCount64();
#else
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (unsigned long)t.tv_sec * 1000ul + (unsigned long)(t.tv_nsec / 1000000);
#endif
}

static void die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fputs("zxv-link: ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
    exit(1);
}

static lc_link_t l;

/* Si falla, el porqué y fuera (sin sustituir lo que no puso zxv-link, con
 * su código: la extensión de VS Code pregunta y vuelve con --overwrite). */
static void ok(int r)
{
    if (r == LC_E_EXISTS) {
        fprintf(stderr, "zxv-link: %s (%s)\n", l.error,
                TR("--overwrite replaces it", "con --overwrite se sustituye"));
        exit(EXIT_EXISTS);
    }
    if (r < 0)
        die("%s", l.error);
}

static bool show_log;

static void on_log(void *ctx, const char *text, unsigned n)
{
    (void)ctx;
    if (show_log)
        fwrite(text, 1, n, stderr);
}

static void on_progress(void *ctx, const char *name, size_t done, size_t total, bool skipped)
{
    (void)ctx;
    if (skipped)
        fprintf(stderr, "%s: %s\n", name, TR("already on the card", "ya está en la tarjeta"));
    else if (done == total)
        fprintf(stderr, "\r%s: %zu %s\n", name, total, TR("bytes", "bytes"));
    else
        fprintf(stderr, "\r%s: %zu / %zu", name, done, total);
}

/* --- Las órdenes ------------------------------------------------------------------- */

static void info(void)
{
    lc_info_t i;
    ok(lc_hello(&l, &i));
    printf(TR("%s (spec %u.%u, protocol %u)\nBASIC: %s\n", "%s (spec %u.%u, protocolo %u)\nBASIC: %s\n"),
           i.firmware, (unsigned)(i.spec >> 16), (unsigned)(i.spec & 0xFFFF), i.protocol,
           i.waiting ? TR("waiting for a command", "esperando una orden") : TR("running a program", "con un programa en marcha"));
}

static void run(char **files, int n)
{
    ok(lc_run(&l, (const char *const *)files, n));
    char cmd[200];
    lc_load_command(files[0], cmd, sizeof cmd);
    fprintf(stderr, "%s\n", cmd);
}

static void log_forever(void)
{
    show_log = true;
    for (;;)
        lc_pump(&l, 3600000u);
}

/* --- serve: el depurador del BASIC, como el emulador ----------------------------- */

/* Lo mismo que el emulador con --basic-debug (emulator/src/basicdbg.h en
 * ZX-V): líneas de texto por TCP en 127.0.0.1, con un cliente (la extensión
 * de VS Code). El buzón está en la máquina: se lee y se escribe por J5. */
#define DBG_MAGIC       0x00
#define DBG_STATE       0x20
#define DBG_ACK         0x28
#define DBG_ENDS        0x30
#define DBG_HEAD        0x3C            /* lo que se mira a menudo: hasta los puntos de parada */
#define DBG_TEXT        0x100
#define DBG_POLL_MS     20

static void send_box(intptr_t c, unsigned id)
{
    static uint8_t box[ZL_DBG_SIZE];
    static char line[32 + 2 * ZL_DBG_SIZE];
    ok(lc_dbg_read(&l, 0, ZL_DBG_SIZE, box));
    int n = snprintf(line, sizeof line, "b %u ", id);
    for (unsigned i = 0; i < ZL_DBG_SIZE; i++)
        n += snprintf(line + n, sizeof line - (size_t)n, "%02x", box[i]);
    line[n++] = '\n';
    tcp_send(c, line, (size_t)n);
}

static int hexval(int c)
{
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
}

/* Una línea del cliente. */
static void dbg_command(intptr_t c, char *line)
{
    unsigned off, size, id;
    unsigned long val;
    if (sscanf(line, "w %u %u %lu", &off, &size, &val) == 3) {
        if ((size == 1 || size == 2 || size == 4) && off + size <= ZL_DBG_SIZE) {
            uint8_t v[4];
            zl_put32(v, (uint32_t)val);
            ok(lc_dbg_write(&l, off, v, size));
        }
    } else if (!strncmp(line, "t ", 2)) {
        uint8_t text[0x200];
        unsigned n = 0;
        for (const char *p = line + 2; p[0] && p[1] && n < sizeof text - 1; p += 2) {
            int hi = hexval(p[0]), lo = hexval(p[1]);
            if (hi < 0 || lo < 0)
                break;
            text[n++] = (uint8_t)(hi << 4 | lo);
        }
        text[n++] = 0;
        ok(lc_dbg_write(&l, DBG_TEXT, text, n));
    } else if (sscanf(line, "g %u", &id) == 1) {
        send_box(c, id);
    } else if (!strncmp(line, "paste ", 6)) {
        ok(lc_type(&l, line + 6));
    }
    /* keys: no hay teclado de mentira en la placa. */
}

/* Lo que hace el servidor de EXT:, en la consola. */
static void on_ext(void *ctx, const char *what, const char *zpath)
{
    (void)ctx;
    fprintf(stderr, "EXT:%s (%s)\n", zpath, what);
}

/* La carpeta como EXT: hasta Ctrl+C (sin depurador). */
static void serve_folder_forever(void)
{
    show_log = true;
    for (;;)
        ok(lc_pump(&l, 100));
}

/* Los ficheros a la tarjeta, la máquina esperando una orden y, luego, el
 * depurador por TCP hasta que el cliente se va. Con una carpeta, también la
 * unidad EXT: mientras tanto (sin depurador, hasta Ctrl+C). */
static void serve(unsigned port, const char *folder, const char *run_file, char **files, int n)
{
    ok(lc_hello(&l, NULL));
    if (l.version < 2)
        die("%s", TR("the machine's firmware is too old for serve: update it",
                     "el firmware de la máquina es demasiado antiguo para serve: actualízalo"));
    if (folder) {
        ok(lc_serve(&l, folder, l.overwrite));
        l.ext->on_event = on_ext;
        fprintf(stderr, TR("Serving %s as the EXT: drive\n", "Sirviendo %s como la unidad EXT:\n"), folder);
    }
    if (n)
        ok(lc_copy(&l, (const char *const *)files, n));
    if (run_file && !port) {
        char cmd[400];
        lc_ext_command(folder, run_file, cmd, sizeof cmd);
        ok(lc_run_ext(&l, folder, run_file));
        fprintf(stderr, "%s\n", cmd);
    }
    if (!port)
        serve_folder_forever();
    ok(lc_ready(&l));
    intptr_t ls = tcp_listen(port);
    if (ls < 0)
        die("127.0.0.1:%u: %s", port, TR("cannot listen", "no se puede escuchar"));
    fprintf(stderr, TR("Waiting for the debugger on 127.0.0.1:%u\n", "Esperando al depurador en 127.0.0.1:%u\n"), port);
    show_log = true;
    intptr_t c = -1;
    while (c < 0) {
        lc_pump(&l, 10);                /* el informe, mientras */
        c = tcp_accept(ls, 50);
    }
    tcp_send(c, "hello zxv-basic-debug 1\n", 24);
    send_box(c, 0);
    uint8_t seen[DBG_HEAD];
    ok(lc_dbg_read(&l, 0, DBG_HEAD, seen));
    static char rx[4096];
    size_t rl = 0;
    unsigned long next = now_ms() + DBG_POLL_MS;
    for (;;) {
        int got = tcp_recv(c, rx + rl, sizeof rx - 1 - rl, 5);
        if (got < 0)
            break;                      /* el cliente se ha ido: se acaba */
        rl += (size_t)got;
        char *nlp;
        while ((nlp = memchr(rx, '\n', rl)) != NULL) {
            *nlp = 0;
            if (nlp > rx && nlp[-1] == '\r')
                nlp[-1] = 0;
            dbg_command(c, rx);
            size_t used = (size_t)(nlp + 1 - rx);
            memmove(rx, nlp + 1, rl - used);
            rl -= used;
        }
        if (rl == sizeof rx - 1)
            rl = 0;                     /* una línea que no acaba: fuera */
        if ((long)(now_ms() - next) >= 0) {
            next = now_ms() + DBG_POLL_MS;
            uint8_t h[DBG_HEAD];
            ok(lc_dbg_read(&l, 0, DBG_HEAD, h));
            /* Como el emulador: avisa si cambia la firma, el estado, la
             * respuesta o el contador de finales. */
            static const unsigned watch[] = { DBG_MAGIC, DBG_STATE, DBG_ACK, DBG_ENDS };
            bool changed = false;
            for (unsigned i = 0; i < 4; i++)
                changed |= memcmp(h + watch[i], seen + watch[i], 4) != 0;
            memcpy(seen, h, sizeof h);
            if (changed)
                send_box(c, 0);
            lc_pump(&l, 1);             /* el informe */
        }
    }
    tcp_close(c);
    tcp_close(ls);
}

static void usage(void)
{
    fputs(TR("usage: zxv-link ports\n"
             "       zxv-link --port PORT info | send FILE [DEST] | run FILE [MORE...] | log\n"
             "       zxv-link --port PORT serve [--folder FOLDER [--run PROGRAM]] [--basic-debug TCP_PORT] [FILE...]\n"
             "  --overwrite: also replace on the card what zxv-link did not put there\n"
             "  PORT: the USB-serial adapter on J5 (COM5, /dev/ttyUSB0...)\n",
             "uso: zxv-link ports\n"
             "     zxv-link --port PUERTO info | send FICHERO [DESTINO] | run FICHERO [MÁS...] | log\n"
             "     zxv-link --port PUERTO serve [--folder CARPETA [--run PROGRAMA]] [--basic-debug PUERTO_TCP] [FICHERO...]\n"
             "  --overwrite: sustituir también en la tarjeta lo que no puso zxv-link\n"
             "  PUERTO: el adaptador USB-serie en J5 (COM5, /dev/ttyUSB0...)\n"),
          stderr);
    exit(2);
}

int main(int argc, char **argv)
{
    detect_lang();
    const char *port = NULL;
    int a = 1;
    for (; a < argc && argv[a][0] == '-'; a++) {
        if (!strcmp(argv[a], "--port") && a + 1 < argc)
            port = argv[++a];
        else if (!strcmp(argv[a], "--lang") && a + 1 < argc)
            lc_spanish = !strcmp(argv[++a], "es");
        else if (!strcmp(argv[a], "--overwrite"))
            l.overwrite = true;
        else
            usage();
    }
    if (a >= argc)
        usage();
    const char *cmd = argv[a++];
    if (!strcmp(cmd, "ports")) {
        char list[4096];
        if (!serial_list(list, sizeof list))
            fputs(TR("No serial ports.\n", "No hay puertos serie.\n"), stderr);
        fputs(list, stdout);
        return 0;
    }
    if (!port)
        usage();
    unsigned tcp = 0;
    const char *folder = NULL, *run_file = NULL;
    if (!strcmp(cmd, "serve")) {
        for (;;) {
            if (a + 1 < argc && !strcmp(argv[a], "--basic-debug"))
                tcp = (unsigned)strtoul(argv[a + 1], NULL, 10), a += 2;
            else if (a + 1 < argc && !strcmp(argv[a], "--folder"))
                folder = argv[a + 1], a += 2;
            else if (a + 1 < argc && !strcmp(argv[a], "--run"))
                run_file = argv[a + 1], a += 2;
            else
                break;
        }
        if ((!tcp && !folder) || tcp > 65535 || (run_file && (!folder || tcp)))
            usage();
    }
    l.on_log = on_log;
    l.on_progress = on_progress;
    l.allow_stop = true;                /* run: lo pide quien lo escribe */
    if (lc_open(&l, port, NULL) < 0) {
        /* Con su código de salida: la extensión de VS Code ofrece elegir
         * otro puerto. */
        fprintf(stderr, "zxv-link: %s%s\n", l.error,
                strstr(l.error, "USB") ? TR(" (zxv-link ports lists them)", " (zxv-link ports los dice)") : "");
        return EXIT_PORT;
    }
    if (!strcmp(cmd, "info") && a == argc)
        info();
    else if (!strcmp(cmd, "send") && (argc - a == 1 || argc - a == 2))
        ok(lc_send(&l, argv[a], argc - a == 2 ? argv[a + 1] : lc_base_name(argv[a])));
    else if (!strcmp(cmd, "run") && argc - a >= 1)
        run(argv + a, argc - a);
    else if (!strcmp(cmd, "log") && a == argc)
        log_forever();
    else if (!strcmp(cmd, "serve"))
        serve(tcp, folder, run_file, argv + a, argc - a);
    else
        usage();
    lc_close(&l);
    return 0;
}
