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
#include "netpc.h"
#include "serial.h"
#include "zxvlink.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Los errores de la máquina que se miran aquí (los de sdk/include/zxv.h,
 * que no se incluye: es de los programas de la máquina). */
#ifndef ZXV_E_NOENT
#define ZXV_E_NOENT     (-1)
#define ZXV_E_EXIST     (-2)
#define ZXV_E_NAME      (-3)
#define ZXV_E_NOTDIR    (-4)
#define ZXV_E_ISDIR     (-5)
#define ZXV_E_NOTEMPTY  (-6)
#define ZXV_E_BADF      (-7)
#define ZXV_E_MFILE     (-8)
#define ZXV_E_ACCESS    (-9)
#define ZXV_E_IO        (-11)
#define ZXV_E_NOCARD    (-12)
#define ZXV_E_NOSPC     (-13)
#define ZXV_E_INVAL     (-17)
#endif
#ifndef ZXV_E_OFFLINE
#define ZXV_E_OFFLINE   (-18)
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
#define LC_E_UPDATE     (-109)          /* la actualización no ha acabado bien */
#define LC_E_UNPAIRED   (-110)          /* por la red, sin emparejar con este PC */

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
    netpc_t *net;                       /* o, por la red (D106), esto */
    bool unpaired;                      /* por la red: aún sin emparejar con este PC */
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
    uint8_t pend[ZL_FRAME_MAX];         /* por la red llega una trama entera de golpe */
    int pend_n, pend_at;
    /* La unidad EXT: que sirve (lc_serve), o NULL. */
    ext_server_t *ext;
    unsigned long ext_next;             /* el próximo ZL_SERVE */
    unsigned long net_quiet;            /* por la red: cuándo toca saludar si no se ha dicho nada (ZLN_KEEPALIVE_MS) */
    bool ext_busy;
} lc_link_t;

typedef struct {
    unsigned protocol;
    uint32_t spec;                      /* ZXV_SV_VERSION */
    bool waiting;                       /* el BASIC espera una orden */
    bool updating;                      /* está actualizando el firmware (versión 5) */
    char firmware[64];                  /* "ZX-V 0.12.0" */
} lc_info_t;

/* Abrir el puerto (con serial_open: el porqué de LC_E_PORT, en *why_kind,
 * SERIAL_E_*) y cerrarlo. port puede ser también una máquina de la red
 * (netpc_is_target: "192.168.0.24", "ZX-V-1A2B" o "ZX-V-1A2B (192.168.0.24)");
 * si no está emparejada con este PC, se abre igual (l->unpaired) pero solo
 * se puede emparejar (lc_pair_*): lo demás da LC_E_UNPAIRED. */
int lc_open(lc_link_t *l, const char *port, int *why_kind);
void lc_close(lc_link_t *l);

/* Los puertos serie y las máquinas de la red que contestan (en ms) y con las
 * que se puede hablar, uno por línea en out (n bytes), como los acepta
 * lc_open: cuántos. Las que hablan otra versión del protocolo de la red
 * van aparte, en other (on bytes), una por línea: "ZX-V-1A2B (IP): " y qué
 * hacer (lc_net_compatible). */
int lc_list_ports(char *out, size_t n, char *other, size_t on, unsigned ms);

/* Las k máquinas de list, repartidas como en lc_list_ports (detrás de lo
 * que ya haya en out y other): cuántas van a out. */
int lc_list_found(const netpc_found_t *list, int k, char *out, size_t n, char *other, size_t on);

/* Si se puede hablar con una máquina de la red que contesta con la versión
 * version del protocolo de la red (netpc_found_t.version); si no, en why
 * (n bytes) por qué y qué actualizar: ella (más antigua) o zxv-link. */
bool lc_net_compatible(unsigned version, char *why, size_t n);

/* Emparejar con la máquina de la red abierta (con EDIT > Wi-Fi > Emparejar un PC
 * abierto en ella): el código de 6 cifras que enseñan los dos y, luego,
 * cada poco, cómo va (ZLN_PAIR_WAIT, ZLN_PAIR_OK, ZLN_PAIR_NO). 0, o un
 * error con su porqué en l->error. */
int lc_pair_begin(lc_link_t *l, const char *pc_name, uint32_t *code);
int lc_pair_poll(lc_link_t *l);

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

/* Lo mismo con un solo fichero del PC, file, que la máquina ve como
 * EXT:/name y solo puede leer. */
int lc_serve_file(lc_link_t *l, const char *file, const char *name);

/* Dejar de servirla (se lo dice a la máquina). */
void lc_unserve(lc_link_t *l);

/* La versión de ZX-V de un fichero de actualización (zxv-update.bin, de
 * pico/tools/mkimages.py) en version (17 bytes): 0, o LC_E_FILE si no se
 * puede leer o no es uno (con el porqué en l->error). */
int lc_update_version(lc_link_t *l, const char *file, char version[17]);

/* Actualizar el firmware de la máquina con file (versión 5, D105): la
 * máquina lo lee como EXT:/zxv-update.bin. Si hay un programa en marcha, se
 * para antes (como lc_ready). Sigue a la máquina hasta que acaba, con sus
 * dos reinicios (unos minutos; lo que cuenta va a on_log), y comprueba que
 * vuelve con la versión del fichero: 0, o un error (LC_E_UPDATE si no
 * acaba bien: lo dice su pantalla). */
int lc_update(lc_link_t *l, const char *file);

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

/* --- La tarjeta y la ROM, como un explorador (versión 6, D112) ------------
 * Las rutas, con SD: o ROM: delante ("SD:/juegos/a.zxv"; sin unidad, SD:).
 * En ROM: solo se lee. Con un firmware anterior, LC_E_OLD. */

typedef struct {
    char name[64];
    uint32_t size;
    bool dir;
} lc_entry_t;

/* Lo que hay en la carpeta path: cuántas entradas (en *out, con malloc;
 * free al acabar), o un error. */
int lc_list(lc_link_t *l, const char *path, lc_entry_t **out);

/* Traer el fichero path de la máquina al PC, como local (se escribe aparte
 * y, entero, pasa a su nombre; si ya hay uno, lo sustituye: quien llama
 * pregunta antes). on_progress, como al mandar. */
int lc_get(lc_link_t *l, const char *path, const char *local);

/* Borrar un fichero o una carpeta vacía, renombrar (o mover dentro de la
 * misma unidad; si el nombre nuevo existe, ZXV_E_EXIST) y crear una
 * carpeta. */
int lc_remove(lc_link_t *l, const char *path);
int lc_rename(lc_link_t *l, const char *from, const char *to);
int lc_mkdir(lc_link_t *l, const char *path);

/* La orden que ejecuta path de la tarjeta, la ROM o EXT: entrando antes en
 * su carpeta (así encuentra sus ficheros): CD "SD:/juegos": LOAD "a.zxv". */
void lc_drive_command(const char *path, char *cmd, size_t n);

/* Ejecutar path (SD:, ROM: o EXT:): si hay un programa en marcha, como
 * lc_run. */
int lc_run_path(lc_link_t *l, const char *path);

/* Ejecutar un fichero del PC directo a la memoria (D112): la máquina lo lee
 * de EXT: (si se sirve una carpeta, como EXT:/.pc/nombre; si no, se sirve
 * solo él) y no se copia a la tarjeta. Lo que cargue después es cosa del
 * programa. Hay que seguir sirviendo (lc_pump) mientras lo lee:
 * lc_ext_busy dice si tiene algo abierto. */
int lc_run_pc(lc_link_t *l, const char *file);
bool lc_ext_busy(const lc_link_t *l);

/* El buzón del depurador del BASIC (versión 2). */
int lc_dbg_read(lc_link_t *l, unsigned off, unsigned n, uint8_t *out);
int lc_dbg_write(lc_link_t *l, unsigned off, const uint8_t *p, unsigned n);

#ifdef __cplusplus
}
#endif

#endif /* ZXV_LINKCORE_H */
