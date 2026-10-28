/*
 * files.c - La biblioteca estándar de C con la tarjeta: crea una
 * carpeta, escribe y lee con fprintf y fgets, recorre la carpeta con
 * opendir y readdir, cambia nombres y borra; deja lo que ve en
 * informe.txt y en la pantalla, y espera una tecla. Se compila con
 * picolibc (ver el Makefile).
 */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static FILE *out;

/* A la pantalla y al informe. */
static void say(const char *fmt, const char *a, long n)
{
    printf(fmt, a, n);
    fprintf(out, fmt, a, n);
}

int main(void)
{
    /* Si se ha cargado desde el disco ROM (CD "ROM:"), en él no se escribe:
     * a la raíz de la tarjeta. */
    char where[256];
    if (getcwd(where, sizeof(where)) && !strncmp(where, "ROM:", 4))
        chdir("SD:");
    out = fopen("informe.txt", "w");
    if (!out)
        return 1;
    mkdir("datos", 0777);
    char cwd[64];
    chdir("datos");
    say("carpeta %s %ld\n", getcwd(cwd, sizeof(cwd)), 0);

    /* Unas líneas con fprintf, leídas con fgets. */
    FILE *f = fopen("puntos.txt", "w");
    for (int i = 1; i <= 3; i++)
        fprintf(f, "jugador%d %d\n", i, i * 100);
    fclose(f);
    f = fopen("puntos.txt", "r");
    char line[32];
    long total = 0;
    while (fgets(line, sizeof(line), f)) {
        char *sp = strchr(line, ' ');
        total += sp ? atol(sp + 1) : 0;
    }
    fclose(f);
    say("%s %ld\n", "total", total);

    /* Un bloque con malloc, fwrite y fread, y su tamaño con stat. */
    unsigned char *buf = malloc(1000);
    for (int i = 0; i < 1000; i++)
        buf[i] = (unsigned char)i;
    f = fopen("bloque.bin", "wb");
    fwrite(buf, 1, 1000, f);
    fclose(f);
    memset(buf, 0, 1000);
    f = fopen("bloque.bin", "rb");
    size_t n = fread(buf, 1, 1000, f);
    fclose(f);
    int ok = n == 1000 && buf[999] == (unsigned char)999;
    free(buf);
    struct stat st;
    stat("bloque.bin", &st);
    say("%s %ld\n", ok ? "bloque bien" : "bloque mal", (long)st.st_size);

    /* La carpeta, recorrida. */
    rename("puntos.txt", "records.txt");
    DIR *d = opendir(".");
    struct dirent *e;
    while ((e = readdir(d)))
        say("%s %ld\n", e->d_name, (long)e->d_size);
    closedir(d);

    /* Se borra todo y la carpeta queda vacía. */
    unlink("records.txt");
    unlink("bloque.bin");
    chdir("..");
    say("rmdir %s %ld\n", rmdir("datos") == 0 ? "bien" : "mal", 0);
    fclose(out);
    printf("Pulsa una tecla\n");
    getchar();                          /* la consola: el teclado */
    return 0;
}
