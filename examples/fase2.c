/*
 * fase2.c - prueba de la libc de MiOS (Fase 2).
 *
 * Ejercita: malloc/realloc/calloc/free, stdio con FILE (incluidos stdout y
 * environ, que obligan a una relocalizacion R_X86_64_COPY), snprintf/sscanf,
 * string, ctype, qsort/bsearch, getenv/setenv, stat, dirent, tiempo y errno.
 *
 * Termina con un resumen; el codigo de salida es el numero de fallos.
 */

#include <mios.h>

static int fallos = 0;

static void check(int cond, const char *que) {
    if (cond) {
        printf("  ok    %s\n", que);
    } else {
        printf("  FALLO %s\n", que);
        fallos++;
    }
}

static int cmp_int(const void *a, const void *b) {
    int x = *(const int *)a;
    int y = *(const int *)b;

    return (x > y) - (x < y);
}

static void al_salir(void) {
    printf("  ok    atexit se ejecuta al salir\n");
}

int main(int argc, char **argv, char **envp) {
    printf("== MiOS Fase 2: libc propia ==\n");
    printf("argc=%d argv[0]=%s\n", argc, argv[0]);

    /* --- memoria dinamica --- */
    printf("[memoria]\n");
    {
        char *p = (char *)malloc(1000);
        int i;
        char *q;

        check(p != NULL, "malloc(1000) devuelve memoria");
        for (i = 0; i < 1000; i++) {
            p[i] = (char)(i & 0x7f);
        }

        q = (char *)realloc(p, 4000);
        check(q != NULL, "realloc(4000) funciona");
        check(q != NULL && q[10] == (char)(10 & 0x7f) && q[999] == (char)(999 & 0x7f),
              "realloc conserva el contenido");

        {
            int *z = (int *)calloc(64, sizeof(int));
            int cero = 1;
            check(z != NULL, "calloc(64, 4) devuelve memoria");
            if (z != NULL) {
                for (i = 0; i < 64; i++) {
                    if (z[i] != 0) {
                        cero = 0;
                    }
                }
                check(cero, "calloc deja todo a cero");
                free(z);
            }
        }

        {
            void *v[200];
            int todo = 1;
            for (i = 0; i < 200; i++) {
                v[i] = malloc((mios_size_t)(i + 1) * 37);
                if (v[i] == NULL) {
                    todo = 0;
                } else {
                    memset(v[i], i & 0xff, (mios_size_t)(i + 1) * 37);
                }
            }
            for (i = 0; i < 200; i += 2) {
                free(v[i]);
            }
            for (i = 0; i < 200; i++) {
                if (v[i] != NULL) {
                    free(v[i]);
                }
            }
            check(todo, "200 malloc encadenados");
        }
        free(q);
    }

    /* --- snprintf / sscanf / floats --- */
    printf("[formato]\n");
    {
        char buf[128];
        int n = snprintf(buf, sizeof(buf), "%d/%s/%.2f/%04x/%c", -7, "hola",
                         3.14159, 255, 'Z');
        int a = 0;
        char s[32];
        double d = 0.0;
        unsigned int h = 0;

        printf("  snprintf -> \"%s\" (%d bytes)\n", buf, n);
        check(strcmp(buf, "-7/hola/3.14/00ff/Z") == 0, "snprintf con todos los tipos");

        a = sscanf("41 2.5 abc 1f", "%d %lf %s %x", (int *)&a, &d, s, &h);
        check(a == 4 && s[0] == 'a', "sscanf lee 4 campos");
        check(d > 2.49 && d < 2.51, "sscanf lee un flotante");
        check(h == 0x1f, "sscanf lee hexadecimal");
        check(a == 4, "sscanf devuelve cuantos campos leyo (4)");
    }

    /* --- cadenas y ctype --- */
    printf("[cadenas]\n");
    {
        char buf[64];
        char *tok;
        char *save = NULL;

        strcpy(buf, "uno");
        strcat(buf, ",dos,tres");
        check(strcmp(buf, "uno,dos,tres") == 0, "strcpy + strcat");
        check(strstr(buf, "dos") == buf + 4, "strstr encuentra la subcadena");
        check(strrchr(buf, ',') == buf + 7, "strrchr encuentra el ultimo separador");

        tok = strtok_r(buf, ",", &save);
        check(tok != NULL && strcmp(tok, "uno") == 0, "strtok_r primer campo");
        tok = strtok_r(NULL, ",", &save);
        check(tok != NULL && strcmp(tok, "dos") == 0, "strtok_r segundo campo");

        {
            char *dup = strdup("copia");
            check(dup != NULL && strcmp(dup, "copia") == 0, "strdup");
            free(dup);
        }
        check(strcasecmp("Hola", "hOLa") == 0, "strcasecmp");
        check(isdigit('7') && !isdigit('x'), "isdigit");
        check(isalpha('q') && !isalpha('1'), "isalpha");
        check(toupper('a') == 'A' && tolower('B') == 'b', "toupper/tolower");
        check(isspace(' ') && !isspace('z'), "isspace");
    }

    /* --- qsort / bsearch --- */
    printf("[ordenacion]\n");
    {
        int v[10] = {9, 3, 7, 1, 8, 2, 6, 4, 5, 0};
        int key = 6;
        int i;
        int ordenado = 1;

        qsort(v, 10, sizeof(int), cmp_int);
        for (i = 1; i < 10; i++) {
            if (v[i - 1] > v[i]) {
                ordenado = 0;
            }
        }
        check(ordenado, "qsort ordena 10 enteros");
        check(bsearch(&key, v, 10, sizeof(int), cmp_int) != NULL, "bsearch encuentra 6");
        key = 42;
        check(bsearch(&key, v, 10, sizeof(int), cmp_int) == NULL, "bsearch no encuentra 42");
    }

    /* --- entorno (necesita environ, con R_X86_64_COPY) --- */
    printf("[entorno]\n");
    {
        const char *v;

        check(environ != NULL, "environ esta relleno");
        check(envp != NULL && environ == envp, "environ coincide con envp");
        check(setenv("MIOS_TEST", "42", 1) == 0, "setenv");
        v = getenv("MIOS_TEST");
        check(v != NULL && strcmp(v, "42") == 0, "getenv devuelve lo puesto");
        check(getenv("NO_EXISTE_ESTA") == NULL, "getenv de algo que no existe");
    }

    /* --- stdio con FILE --- */
    printf("[stdio FILE]\n");
    {
        FILE *f = fopen("/tmp/fase2.txt", "w");

        check(f != NULL, "fopen(...,\"w\")");
        if (f != NULL) {
            fprintf(f, "linea 1\n");
            fputs("linea 2\n", f);
            fwrite("linea 3\n", 1, 8, f);
            check(ftell(f) == 24, "ftell acumula lo escrito");
            fclose(f);
        }

        f = fopen("/tmp/fase2.txt", "r");
        if (f != NULL) {
            char linea[64];
            char otro[16];
            size_t got;

            check(fgets(linea, sizeof(linea), f) != NULL &&
                      strcmp(linea, "linea 1\n") == 0,
                  "fgets lee la primera linea");
            check(fseek(f, 8, SEEK_SET) == 0, "fseek a la posicion 8");
            got = fread(otro, 1, 5, f);
            otro[got] = '\0';
            check(got == 5 && strcmp(otro, "linea") == 0, "fread tras fseek");
            check(!feof(f), "feof es falso antes del final");
            fclose(f);
        } else {
            check(0, "fopen(...,\"r\")");
        }

        check(remove("/tmp/fase2.txt") == 0, "remove borra el fichero");

        {
            FILE *g = fopen("/no/existe/nada.txt", "r");
            check(g == NULL, "fopen de algo inexistente devuelve NULL");
            check(errno == ENOENT, "errno = ENOENT en ese caso");
        }

        fprintf(stdout, "  ok    fprintf por stdout (simbolo de datos con COPY)\n");
        fflush(stdout);
    }

    /* --- stat / dirent --- */
    printf("[ficheros]\n");
    {
        struct stat st;
        DIR *d;
        struct dirent *e;
        int visto = 0;
        int entradas = 0;

        check(stat("/lib", &st) == 0, "stat(/lib)");
        check(S_ISDIR(st.st_mode), "/lib es un directorio");
        check(stat("/lib/libmios.so.6", &st) == 0 && st.st_size > 0,
              "stat de libmios.so.6 con tamano");

        d = opendir("/lib");
        check(d != NULL, "opendir(/lib)");
        if (d != NULL) {
            while ((e = readdir(d)) != NULL) {
                entradas++;
                if (strcmp(e->d_name, "libmios.so.6") == 0) {
                    visto = 1;
                }
            }
            closedir(d);
        }
        printf("  /lib tiene %d entradas\n", entradas);
        check(visto, "readdir encuentra libmios.so.6");
    }

    /* --- tiempo --- */
    printf("[tiempo]\n");
    {
        time_t t = time(NULL);
        struct tm *tm = gmtime(&t);
        char buf[64];

        check(t > 1600000000, "time() da una fecha razonable");
        check(tm != NULL && tm->tm_year >= 124, "gmtime devuelve un anio >= 2024");
        if (tm != NULL) {
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
            printf("  fecha UTC: %s\n", buf);
            check(strlen(buf) == 19, "strftime da 19 caracteres");
        }
    }

    /* --- syscall extra --- */
    printf("[sistema]\n");
    {
        struct utsname u;

        check(uname(&u) == 0, "uname");
        printf("  %s %s %s\n", u.sysname, u.release, u.machine);
        check(lseek(0, 0, SEEK_CUR) >= 0 || errno == ESPIPE, "lseek");
        check(isatty(1) == 0 || isatty(1) == 1, "isatty no revienta");
    }

    atexit(al_salir);

    printf("\n== resultado: %d fallo(s) ==\n", fallos);
    return fallos;
}
