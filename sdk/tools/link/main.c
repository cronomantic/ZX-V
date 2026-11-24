/*
 * zxv-link - La carga remota de ZX-V (D101): habla con la máquina por la tira
 * J5 de la placa (su UART), con un adaptador USB-serie de 3,3 V, o por la
 * red si la Pico CPU es una Pico 2 W (D106). La conversación, en linkcore.c
 * (la comparte zxv-link-gui, con ventana).
 *
 *   zxv-link ports                         los puertos serie que hay y las ZX-V
 *                                          de la red que contestan (D106); las
 *                                          de otra versión de la red, aparte
 *   zxv-link --port PUERTO info            la versión de la máquina
 *   zxv-link --port PUERTO send FICHERO [DESTINO]   a la tarjeta
 *   zxv-link --port PUERTO run FICHERO [MÁS...]   a la tarjeta (con los
 *                                          ficheros que carga), y lo ejecuta
 *                                          (si hay un programa en marcha,
 *                                          antes lo para)
 *   zxv-link --port PUERTO run --pc FICHERO   lo ejecuta directo a la memoria,
 *                                          sin copiarlo a la tarjeta (la máquina
 *                                          lo lee del PC por EXT:; D112). Si el
 *                                          programa lo deja abierto (un .zxv con
 *                                          bancos de código), lo sigue sirviendo
 *                                          hasta que lo cierra o Ctrl+C, y lo
 *                                          dice en la salida estándar con la
 *                                          línea "zxv-link: serving-ext"
 *   zxv-link --port PUERTO exec RUTA       ejecuta lo que ya está en la máquina:
 *                                          SD:/juegos/a.zxv, ROM:/b.bas o EXT:/c.zxv
 *   zxv-link --port PUERTO ls [RUTA]       lo que hay en una carpeta de la
 *                                          tarjeta (SD:/ si no se dice) o de ROM:
 *   zxv-link --port PUERTO get RUTA [FICHERO]   de la tarjeta (o ROM:) al PC
 *   zxv-link --port PUERTO rm RUTA         borra un fichero o una carpeta vacía
 *   zxv-link --port PUERTO mv RUTA NUEVA   renombra o mueve, en la misma unidad
 *   zxv-link --port PUERTO mkdir RUTA      crea una carpeta
 *   zxv-link --port PUERTO log             el informe de la máquina (Ctrl+C para)
 *   zxv-link --port MÁQUINA pair           emparejar este PC con una ZX-V de la
 *                                          red (con EDIT > Wi-Fi > Emparejar un PC abierto
 *                                          en ella): los dos enseñan un código; si
 *                                          coincide, se acepta en la máquina
 *   zxv-link --port PUERTO [--yes] update FICHERO
 *                                          actualiza el firmware de la máquina con
 *                                          FICHERO (un zxv-update.bin), sin tarjeta:
 *                                          la máquina lo lee del PC por J5 (D105);
 *                                          antes pregunta, salvo con --yes
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
 * en Linux, o una ZX-V de la red (D106): su nombre (ZX-V-1A2B) o su IP. Los mensajes, en el idioma del sistema (español o inglés), o el
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

/* Ejecutar un fichero del PC directo a la memoria: se sirve mientras la
 * máquina lo tiene abierto. Un .zxv con bancos de código lo tiene abierto
 * mientras el programa está en marcha (carga de él cada banco): entonces se
 * sigue sirviendo, sin límite de tiempo, hasta que lo cierra (o se acaba el
 * programa, o la máquina deja de contestar) o Ctrl+C. Los 30 s son solo para
 * que empiece a leerlo. */
#define RUN_PC_START_MS 30000           /* lo que se espera a que lo abra */
#define RUN_PC_CLOSED_MS 1500           /* cerrado tanto tiempo: ya lo ha leído */
#define RUN_PC_KEPT_MS  2000            /* abierto sin pedir nada: lo guarda el programa */
#define RUN_PC_CHECK_MS 5000            /* mientras, si la máquina sigue ahí */

static void run_pc(const char *file)
{
    ok(lc_run_pc(&l, file));
    fprintf(stderr, "%s\n", TR("Running it from the PC...", "Ejecutándolo desde el PC..."));
    const char *name = lc_base_name(file);
    unsigned long t0 = now_ms(), asked = t0, closed = 0, check = 0;
    unsigned long reqs = l.ext ? l.ext->requests : 0;
    bool seen = false, kept = false;
    for (;;) {
        ok(lc_pump(&l, 100));
        unsigned long t = now_ms();
        if (l.ext && l.ext->requests != reqs) {
            reqs = l.ext->requests;
            asked = t;
        }
        if (lc_ext_busy(&l)) {
            seen = true;
            closed = 0;
            if (!kept && t - asked >= RUN_PC_KEPT_MS) {
                kept = true;
                check = t + RUN_PC_CHECK_MS;
                fprintf(stderr,
                        TR("The program keeps %s open (it loads its code banks from it): serving it while it runs "
                           "(Ctrl+C stops)\n",
                           "El programa tiene %s abierto (carga de él sus bancos de código): se sirve mientras está "
                           "en marcha (Ctrl+C para)\n"),
                        name);
                /* Y para los programas (la extensión de VS Code): una línea
                 * fija, sin traducir, en la salida estándar. */
                fputs("zxv-link: serving-ext\n", stdout);
                fflush(stdout);
            }
            if (kept && (long)(t - check) >= 0) {
                lc_info_t i;
                ok(lc_hello(&l, &i));   /* si no contesta, fuera, con el porqué */
                if (i.waiting)
                    break;              /* el BASIC espera una orden: el programa se ha acabado */
                check = now_ms() + RUN_PC_CHECK_MS;
            }
        } else if (seen) {
            if (!closed)
                closed = t;
            else if (t - closed >= RUN_PC_CLOSED_MS)
                break;
        } else if (t - t0 >= RUN_PC_START_MS) {
            die("%s", TR("the machine has not read it (is BASIC waiting?)", "la máquina no lo ha leído (¿espera el BASIC?)"));
        }
    }
    if (kept)
        fprintf(stderr, TR("The program has finished with %s\n", "El programa ha acabado con %s\n"), name);
}

static void exec_path(const char *path)
{
    ok(lc_run_path(&l, path));
    char cmd[400];
    lc_drive_command(path, cmd, sizeof cmd);
    fprintf(stderr, "%s\n", cmd);
}

static void list(const char *path)
{
    lc_entry_t *e;
    int n = lc_list(&l, path, &e);
    ok(n);
    for (int i = 0; i < n; i++)
        if (e[i].dir)
            printf("%s/\n", e[i].name);
        else
            printf("%-40s %10lu\n", e[i].name, (unsigned long)e[i].size);
    free(e);
}

static void run(char **files, int n)
{
    ok(lc_run(&l, (const char *const *)files, n));
    char cmd[200];
    lc_load_command(files[0], cmd, sizeof cmd);
    fprintf(stderr, "%s\n", cmd);
}

/* Actualizar el firmware con un zxv-update.bin (D105): las versiones, la
 * pregunta y, mientras tanto, lo que cuenta la máquina. */
static void update(const char *file, bool yes)
{
    char version[17];
    lc_info_t i;
    ok(lc_update_version(&l, file, version));
    ok(lc_hello(&l, &i));
    fprintf(stderr, TR("The machine: %s\nThe file:    ZX-V %s\n", "La máquina: %s\nEl fichero: ZX-V %s\n"),
            i.firmware, version);
    if (!yes) {
        fprintf(stderr, "%s", TR("Update the firmware of both Picos? The machine restarts (what is in memory is "
                                 "lost) and it takes a few minutes. [y/N] ",
                                 "¿Actualizar el firmware de las dos Pico? La máquina se reinicia (se pierde lo "
                                 "que tenga en la memoria) y tarda unos minutos. [s/N] "));
        char a[16];
        if (!fgets(a, sizeof a, stdin) || (a[0] != 'y' && a[0] != 'Y' && a[0] != 's' && a[0] != 'S')) {
            fputs(TR("Nothing done.\n", "No se ha hecho nada.\n"), stderr);
            exit(1);
        }
    }
    fputs(TR("Updating (do not switch the machine off or unplug the adapter)...\n",
             "Actualizando (no apagues la máquina ni desenchufes el adaptador)...\n"),
          stderr);
    show_log = true;
    ok(lc_update(&l, file));
    fprintf(stderr, TR("Done: the machine is running ZX-V %s.\n", "Hecho: la máquina lleva ZX-V %s.\n"), version);
}

/* Emparejar con una máquina de la red: el código, y esperar a que se acepte
 * en ella (como mucho dos minutos). */
static void pair(void)
{
    if (!l.net)
        die("%s", TR("pair is for machines on the network (zxv-link ports lists them)",
                     "pair es para las máquinas de la red (zxv-link ports las dice)"));
    if (!l.unpaired) {
        fprintf(stderr, TR("%s is already paired with this PC.\n", "%s ya está emparejada con este PC.\n"),
                netpc_name(l.net));
        return;
    }
    char name[64] = "";
    const char *env = getenv("COMPUTERNAME");
    if (!env)
        env = getenv("HOSTNAME");
    snprintf(name, sizeof name, "%s", env && *env ? env : "PC");
    uint32_t code;
    ok(lc_pair_begin(&l, name, &code));
    fprintf(stderr, TR("Code: %03u %03u\nCheck that %s shows the same code and accept there (Enter).\n",
                       "Código: %03u %03u\nComprueba que %s enseña el mismo código y acepta allí (Enter).\n"),
            (unsigned)(code / 1000), (unsigned)(code % 1000), netpc_name(l.net));
    unsigned long end = now_ms() + 120000;
    for (;;) {
        int st = lc_pair_poll(&l);
        if (st == ZLN_PAIR_OK) {
            fprintf(stderr, TR("Paired: from now on, --port %s.\n", "Emparejada: desde ahora, --port %s.\n"),
                    netpc_name(l.net));
            return;
        }
        if (st == ZLN_PAIR_NO)
            die("%s", TR("the machine said no (or its box was closed)", "la máquina ha dicho que no (o se cerró su cuadro)"));
        if ((long)(now_ms() - end) > 0)
            die("%s", TR("nobody accepted on the machine in two minutes", "nadie ha aceptado en la máquina en dos minutos"));
#ifdef _WIN32
        Sleep(500);
#else
        struct timespec t = { 0, 500000000 };
        nanosleep(&t, NULL);
#endif
    }
}

static void log_forever(void)
{
    show_log = true;
    /* Por la red, la máquina manda el informe al último que le habló: hay
     * que saludarla (si no, seguiría yendo a otro zxv-link, ya cerrado). */
    ok(lc_hello(&l, NULL));
    for (;;)
        lc_pump(&l, 1000);              /* por la red, saluda cada tanto (lc_pump) */
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
             "       zxv-link --port PORT info | send FILE [DEST] | run FILE [MORE...] | run --pc FILE | log\n"
             "       zxv-link --port PORT exec PATH | ls [PATH] | get PATH [FILE] | rm PATH | mv PATH NEW | mkdir PATH\n"
             "       zxv-link --port PORT serve [--folder FOLDER [--run PROGRAM]] [--basic-debug TCP_PORT] [FILE...]\n"
             "       zxv-link --port PORT [--yes] update zxv-update.bin\n"
             "       zxv-link --port MACHINE pair\n"
             "  --overwrite: also replace on the card what zxv-link did not put there\n"
             "  --yes: update without asking\n"
             "  PATH: on the machine, SD:/games/a.zxv, ROM:/b.bas (read only) or, for exec, EXT:/c.zxv\n"
             "  PORT: the USB-serial adapter on J5 (COM5, /dev/ttyUSB0...) or a ZX-V on the network\n"
             "        (its name, ZX-V-1A2B, or its IP; zxv-link ports lists them)\n",
             "uso: zxv-link ports\n"
             "     zxv-link --port PUERTO info | send FICHERO [DESTINO] | run FICHERO [MÁS...] | run --pc FICHERO | log\n"
             "     zxv-link --port PUERTO exec RUTA | ls [RUTA] | get RUTA [FICHERO] | rm RUTA | mv RUTA NUEVA | mkdir RUTA\n"
             "     zxv-link --port PUERTO serve [--folder CARPETA [--run PROGRAMA]] [--basic-debug PUERTO_TCP] [FICHERO...]\n"
             "     zxv-link --port PUERTO [--yes] update zxv-update.bin\n"
             "     zxv-link --port MÁQUINA pair\n"
             "  --overwrite: sustituir también en la tarjeta lo que no puso zxv-link\n"
             "  --yes: actualizar sin preguntar\n"
             "  RUTA: en la máquina, SD:/juegos/a.zxv, ROM:/b.bas (solo leer) o, con exec, EXT:/c.zxv\n"
             "  PUERTO: el adaptador USB-serie en J5 (COM5, /dev/ttyUSB0...) o una ZX-V de la red\n"
             "          (su nombre, ZX-V-1A2B, o su IP; zxv-link ports las dice)\n"),
          stderr);
    exit(2);
}

int main(int argc, char **argv)
{
    detect_lang();
    const char *port = NULL;
    bool yes = false;
    int a = 1;
    for (; a < argc && argv[a][0] == '-'; a++) {
        if (!strcmp(argv[a], "--port") && a + 1 < argc)
            port = argv[++a];
        else if (!strcmp(argv[a], "--lang") && a + 1 < argc)
            lc_spanish = !strcmp(argv[++a], "es");
        else if (!strcmp(argv[a], "--overwrite"))
            l.overwrite = true;
        else if (!strcmp(argv[a], "--yes"))
            yes = true;
        else
            usage();
    }
    if (a >= argc)
        usage();
    const char *cmd = argv[a++];
    if (!strcmp(cmd, "ports")) {
        /* En stdout, solo con las que se puede hablar (VS Code y otros la
         * leen línea a línea como puertos); las de otra versión de la red,
         * en stderr, con qué actualizar. */
        char list[4096], other[4096];
        list[0] = 0;
        int k = lc_list_ports(list, sizeof list, other, sizeof other, 1500);
        fputs(list, stdout);
        fflush(stdout);
        if (other[0])
            fprintf(stderr, "%s%s", TR("Not compatible:\n", "No compatibles:\n"), other);
        else if (!k)
            fputs(TR("No serial ports, and no ZX-V answers on the network.\n",
                     "No hay puertos serie, y no contesta ninguna ZX-V en la red.\n"),
                  stderr);
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
    else if (!strcmp(cmd, "run") && argc - a == 2 && !strcmp(argv[a], "--pc"))
        run_pc(argv[a + 1]);
    else if (!strcmp(cmd, "run") && argc - a >= 1 && argv[a][0] != '-')
        run(argv + a, argc - a);
    else if (!strcmp(cmd, "exec") && argc - a == 1)
        exec_path(argv[a]);
    else if (!strcmp(cmd, "ls") && argc - a <= 1)
        list(argc - a == 1 ? argv[a] : "SD:/");
    else if (!strcmp(cmd, "get") && (argc - a == 1 || argc - a == 2))
        ok(lc_get(&l, argv[a], argc - a == 2 ? argv[a + 1] : lc_base_name(argv[a])));
    else if (!strcmp(cmd, "rm") && argc - a == 1)
        ok(lc_remove(&l, argv[a]));
    else if (!strcmp(cmd, "mv") && argc - a == 2)
        ok(lc_rename(&l, argv[a], argv[a + 1]));
    else if (!strcmp(cmd, "mkdir") && argc - a == 1)
        ok(lc_mkdir(&l, argv[a]));
    else if (!strcmp(cmd, "log") && a == argc)
        log_forever();
    else if (!strcmp(cmd, "pair") && a == argc)
        pair();
    else if (!strcmp(cmd, "update") && argc - a == 1)
        update(argv[a], yes);
    else if (!strcmp(cmd, "serve"))
        serve(tcp, folder, run_file, argv + a, argc - a);
    else
        usage();
    lc_close(&l);
    return 0;
}
