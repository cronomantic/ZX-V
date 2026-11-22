/*
 * linkcore.h - La conversación con la máquina por J5 (D101), para zxv-link
 * (la consola) y zxv-link-gui (la ventana): abrir el puerto, hola, mandar
 * ficheros (lo que ya está igual en la tarjeta, no), ejecutar, el buzón del
 * depurador del BASIC y el informe que llega.
 *
 * Nada se para: cada función devuelve 0 o un error (LC_E_*, o un ZXV_E_*
 * de la máquina) y deja el porqué, en texto, en l->error. Los textos, en
 * español o en inglés según lc_spanish.
 *
 * MIT, como el resto del SDK.
 */
#ifndef ZXV_LINKCORE_H
#define ZXV_LINKCORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "extserve.h"
#include "serial.h"
#include "zxvlink.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Los errores de la máquina que se miran aquí (los de sdk/include/zxv.h,
 * que no se incluye: es de los programas de la máquina). */
#ifndef ZXV_E_NOENT
#define ZXV_E_NOENT     (-1)
#define ZXV_E_NAME      (-3)
#define ZXV_E_ISDIR     (-5)
#define ZXV_E_BADF      (-7)
#define ZXV_E_MFILE     (-8)
#define ZXV_E_ACCESS    (-9)
#define ZXV_E_IO        (-11)
#define ZXV_E_NOCARD    (-12)
#define ZXV_E_NOSPC     (-13)
#define ZXV_E_INVAL     (-17)
#endif

/* Errores de este lado (los de la máquina son ZXV_E_*, de -1 a -20). */
#define LC_E_PORT       (-100)          /* el puerto no se puede abrir */
#define LC_E_SERIAL     (-101)          /* el puerto ha fallado */
#define LC_E_NOANSWER   (-102)          /* la máquina no contesta */
#define LC_E_REPLY      (-103)          /* una respuesta que no se entiende */
#define LC_E_FILE       (-104)          /* el fichero del PC no se puede leer */
#define LC_E_OLD        (-105)          /* el firmware no sabe hacer eso */
#define LC_E_CANCEL     (-106)          /* parado desde fuera (cancel) */
#define LC_E_EXISTS     (-107)          /* sustituiría en la tarjeta lo que no puso zxv-link */
#define LC_E_RUNNING    (-108)          /* habría que parar un programa en marcha */

/* Lo que hay en la tarjeta con el nombre de un fichero del PC. */
enum {
    LC_CARD_ABSENT,                     /* nada */
    LC_CARD_SAME,                       /* lo mismo (no se manda) */
    LC_CARD_OURS,                       /* otro, pero el que puso zxv-link la última vez */
    LC_CARD_OTHER                       /* otro, que no puso zxv-link: se pregunta */
};

extern bool lc_spanish;

typedef struct {
    serial_t *s;
    zl_rx_t rx;
    uint8_t seq;
    unsigned version;                   /* la del protocolo de la máquina (tras hello), o 0 */
    char error[256];                    /* el último error, en texto */
    /* Lo que llega sin pedirlo (el informe), y cómo va un fichero (de
     * total bytes; done = total y skipped si ya estaba). */
    void *ctx;
    void (*on_log)(void *ctx, const char *text, unsigned n);
    void (*on_progress)(void *ctx, const char *name, size_t done, size_t total, bool skipped);
    /* A 1 desde otro hilo (la ventana), lo que esté esperando acaba en
     * menos de 100 ms con LC_E_CANCEL. Se vuelve a poner a 0 a mano. */
    volatile int cancel;
    /* Lo que se permite sin preguntar (si no, LC_E_EXISTS y LC_E_RUNNING):
     * sustituir en la tarjeta lo que no puso zxv-link, y parar el programa
     * que esté en marcha para ejecutar otro. */
    bool overwrite, allow_stop;
    /* Lo leído del puerto que venía detrás de una respuesta (para la vez
     * siguiente). */
    uint8_t pend[256];
    int pend_n, pend_at;
    /* La unidad EXT: que sirve (lc_serve), o NULL. */
    ext_server_t *ext;
    unsigned long ext_next;             /* el próximo ZL_SERVE */
    bool ext_busy;
} lc_link_t;

typedef struct {
    unsigned protocol;
    uint32_t spec;                      /* ZXV_SV_VERSION */
    bool waiting;                       /* el BASIC espera una orden */
    char firmware[64];                  /* "ZX-V 0.10.0" */
} lc_info_t;

/* Abrir el puerto (con serial_open: el porqué de LC_E_PORT, en *why_kind,
 * SERIAL_E_*) y cerrarlo. */
int lc_open(lc_link_t *l, const char *port, int *why_kind);
void lc_close(lc_link_t *l);

/* Una petición y su respuesta: el estado de la máquina (o LC_E_*), y lo
 * que viene detrás en more (*more_len bytes). */
int32_t lc_request(lc_link_t *l, uint8_t type, const void *data, unsigned len, unsigned timeout_ms, int tries,
                   uint8_t *more, unsigned *more_len);

/* Lo que llegue en ms (el informe, a on_log): 0, o LC_E_SERIAL si el
 * puerto falla (el adaptador se ha desenchufado). */
int lc_pump(lc_link_t *l, unsigned ms);

int lc_hello(lc_link_t *l, lc_info_t *info);

/* Servir la carpeta folder como la unidad EXT: de la máquina (versión 4):
 * desde aquí, lo que pida la máquina se contesta mientras se espera
 * cualquier cosa (lc_pump, lc_request...), y se anuncia cada segundo.
 * overwrite: puede sustituir lo que no escribió la máquina. 0, o un error
 * (LC_E_FILE si la carpeta no existe; LC_E_OLD con un firmware anterior). */
int lc_serve(lc_link_t *l, const char *folder, bool overwrite);

/* Dejar de servirla (se lo dice a la máquina). */
void lc_unserve(lc_link_t *l);

/* Un fichero del PC a la tarjeta, con el nombre dest. Si ya está igual, no
 * se manda; si hay otro que no puso zxv-link, sin overwrite, LC_E_EXISTS.
 * Lo que manda (y lo que ya estaba igual) lo apunta en el PC, para saber
 * después qué puso. La máquina lo escribe aparte y solo al final, entero,
 * pasa a su nombre: un corte no estropea el que hubiera. */
int lc_send(lc_link_t *l, const char *file, const char *dest);

/* Lo que hay en la tarjeta con el nombre dest frente al fichero del PC:
 * LC_CARD_*, o un error (ZXV_E_NOCARD sin tarjeta; LC_E_OLD con un firmware
 * que no sabe decirlo). */
int lc_card_state(lc_link_t *l, const char *file, const char *dest);

/* Varios ficheros a la raíz de la tarjeta, cada uno con su nombre. Antes de
 * tocar nada mira que haya tarjeta y, sin overwrite, que no se vaya a
 * sustituir lo que no puso zxv-link (LC_E_EXISTS, con sus nombres en
 * l->error). */
int lc_copy(lc_link_t *l, const char *const *files, int n);

/* La orden que se teclea para cargar un programa, como el panel de la
 * extensión de VS Code: LOAD "SD:x", con ": RUN" un .bas y con ": GO TO 0"
 * un programa grabado sin LINE. */
void lc_load_command(const char *file, char *cmd, size_t n);

/* La de cargarlo desde la unidad EXT: (que sirve folder): CD a su carpeta
 * dentro de EXT: (así un .zxv encuentra sus ficheros) y LOAD; por ejemplo,
 * CD "EXT:/juegos": LOAD "a.bas": RUN. LC_E_FILE si no está dentro. */
int lc_ext_command(const char *folder, const char *file, char *cmd, size_t n);

/* Teclearla. Si hay un programa en marcha, la máquina se reinicia entera
 * (ZL_ST_RESTART): se espera a que vuelva el BASIC y se teclea entonces. */
int lc_type(lc_link_t *l, const char *cmd);

/* Que el BASIC espere una orden: si hay un programa en marcha, la máquina
 * se reinicia entera, como con Ctrl+Alt+Supr, y se espera a que vuelva
 * (como mucho 15 s). */
int lc_ready(lc_link_t *l);

/* El programa y los ficheros que carga, a la raíz de la tarjeta, y a
 * ejecutarlo (files[0]). Lo mismo que lc_copy antes de tocar nada y, si hay
 * un programa en marcha, sin allow_stop, LC_E_RUNNING; con él, antes se para
 * (el BASIC vuelve a esperar una orden): puede tener abiertos esos
 * ficheros. */
int lc_run(lc_link_t *l, const char *const *files, int n);

/* Ejecutar file desde la unidad EXT:, que se está sirviendo (lc_serve de
 * folder): nada a la tarjeta. Si hay un programa en marcha, como lc_run. */
int lc_run_ext(lc_link_t *l, const char *folder, const char *file);

/* Los ficheros de la carpeta de un programa que puede cargar (los .gfx,
 * .zxm, .pt3, .afb, .wav...: como la extensión de VS Code), sin él: hasta
 * max rutas en out (cada una con malloc). Cuántas. */
int lc_folder_files(const char *program, char **out, int max);

const char *lc_base_name(const char *path);

/* El fichero de configuración name en la carpeta de zxv-link (ZXV_HOME si
 * está, para las pruebas; si no, %APPDATA%\ZX-V o ~/.config/ZX-V, que se
 * crea): false si no hay dónde. */
bool lc_config_file(const char *name, char *out, size_t n);

/* El buzón del depurador del BASIC (versión 2). */
int lc_dbg_read(lc_link_t *l, unsigned off, unsigned n, uint8_t *out);
int lc_dbg_write(lc_link_t *l, unsigned off, const uint8_t *p, unsigned n);

#ifdef __cplusplus
}
#endif

#endif /* ZXV_LINKCORE_H */
