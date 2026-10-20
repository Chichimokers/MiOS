/*
 * ld-mios.so.1 - cargador dinamico propio de MiOS.
 *
 * Es el equivalente a ld.so / ld-linux: lo lanza el kernel cuando el programa
 * tiene PT_INTERP. Sus tareas (version minima de la Fase 0):
 *
 *   1. Leer el auxv (AT_PHDR, AT_PHNUM, AT_ENTRY...) que deja el kernel.
 *   2. Cargar las librerias de DT_NEEDED (por ahora, libmios.so.6) buscandolas
 *      en LD_LIBRARY_PATH, en el RPATH/RUNPATH del ejecutable y en /lib, en
 *      ese orden. Poder dar una ruta alternativa es lo que permite probar un
 *      binario de MiOS fuera de MiOS (p. ej. en un chroot o en el VPS mismo
 *      con --dynamic-linker absoluto y -rpath: asi puede correr 'configure').
 *   3. Preparar el TLS (variables __thread) y poner %fs: el ejecutable espera
 *      su bloque justo debajo del puntero de hilo (modelo local-exec).
 *   4. Aplicar las relocalizaciones de la libreria y del ejecutable.
 *   5. Ejecutar los constructores (DT_INIT_ARRAY).
 *   6. Saltar al punto de entrada del ejecutable (AT_ENTRY).
 *
 * NO depende de libmios (no puede: es quien la carga). Habla al kernel con
 * syscalls directas, igual que src/kernel/syscall.c.
 *
 * Supuesto de la Fase 0: el ejecutable es ET_EXEC (no PIE), por lo que su
 * "load bias" es 0 y sus r_offsets son direcciones absolutas. La libreria si
 * es ET_DYN y se le asigna una base propia.
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long  u64;
typedef long           i64;

#define MIOS_PAGE 4096UL
#define align_down(x) ((x) & ~(MIOS_PAGE - 1))
#define align_up(x)   (((x) + MIOS_PAGE - 1) & ~(MIOS_PAGE - 1))

/* ------------------------------------------------------------------ */
/* syscalls                                                           */
/* ------------------------------------------------------------------ */

#define SYS_read        0
#define SYS_write       1
#define SYS_open        2
#define SYS_close       3
#define SYS_lseek       8
#define SYS_mmap        9
#define SYS_mprotect    10
#define SYS_arch_prctl  158
#define SYS_exit_group  231

/* arch_prctl: poner el puntero de hilo (%fs). */
#define ARCH_SET_FS 0x1002

#define PROT_NONE   0x0
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define PROT_EXEC   0x4
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define MAP_ANONYMOUS 0x20

#define O_RDONLY 0
#define SEEK_SET 0

static long sys1(long n, long a) {
    long r;
    __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a) : "rcx", "r11", "memory");
    return r;
}

static long sys3(long n, long a, long b, long c) {
    long r;
    __asm__ volatile("syscall"
                     : "=a"(r)
                     : "a"(n), "D"(a), "S"(b), "d"(c)
                     : "rcx", "r11", "memory");
    return r;
}

static long sys6(long n, long a, long b, long c, long d, long e, long f) {
    long r;
    register long r10 __asm__("r10") = d;
    register long r8  __asm__("r8")  = e;
    register long r9  __asm__("r9")  = f;
    __asm__ volatile("syscall"
                     : "=a"(r)
                     : "a"(n), "D"(a), "S"(b), "d"(c),
                       "r"(r10), "r"(r8), "r"(r9)
                     : "rcx", "r11", "memory");
    return r;
}

static void *mios_mmap(void *addr, u64 len, int prot, int flags, int fd, u64 off) {
    return (void *)sys6(SYS_mmap, (long)addr, (long)len, prot, flags, fd, (long)off);
}

static void mios_exit(int code) {
    sys1(SYS_exit_group, code);
    for (;;) {
    }
}

/* ------------------------------------------------------------------ */
/* estructura ELF64                                                    */
/* ------------------------------------------------------------------ */

typedef struct {
    u8  e_ident[16];
    u16 e_type;
    u16 e_machine;
    u32 e_version;
    u64 e_entry;
    u64 e_phoff;
    u64 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    u32 p_type;
    u32 p_flags;
    u64 p_offset;
    u64 p_vaddr;
    u64 p_paddr;
    u64 p_filesz;
    u64 p_memsz;
    u64 p_align;
} Elf64_Phdr;

typedef struct {
    u64 d_tag;
    u64 d_val;
} Elf64_Dyn;

typedef struct {
    u32 st_name;
    u8  st_info;
    u8  st_other;
    u16 st_shndx;
    u64 st_value;
    u64 st_size;
} Elf64_Sym;

typedef struct {
    u64 r_offset;
    u64 r_info;
    i64 r_addend;
} Elf64_Rela;

#define ELF64_R_SYM(i)  ((u32)((i) >> 32))
#define ELF64_R_TYPE(i) ((u32)((i) & 0xffffffffUL))

#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_TLS     7

#define DT_NULL         0
#define DT_NEEDED       1
#define DT_PLTRELSZ     2
#define DT_HASH         4
#define DT_STRTAB       5
#define DT_SYMTAB       6
#define DT_RELA         7
#define DT_RELASZ       8
#define DT_RELAENT      9
#define DT_STRSZ        10
#define DT_SYMENT       11
#define DT_SONAME       14
#define DT_RPATH        15
#define DT_PLTREL       20
#define DT_JMPREL       23
#define DT_INIT_ARRAY   25
#define DT_INIT_ARRAYSZ 27
#define DT_RUNPATH      29
/* Tabla hash estilo GNU (la que usan por defecto las bibliotecas que enlaza
 * el gcc del sistema, p. ej. GMP/MPFR/MPC: solo llevan esta, no DT_HASH). */
#define DT_GNU_HASH     0x6ffffef5

#define PF_X 1
#define PF_W 2
#define PF_R 4

#define R_X86_64_NONE      0
#define R_X86_64_64        1
#define R_X86_64_PC32      2
#define R_X86_64_COPY      5
#define R_X86_64_GLOB_DAT  6
#define R_X86_64_JUMP_SLOT 7
#define R_X86_64_RELATIVE  8

#define AT_NULL   0
#define AT_PHDR   3
#define AT_PHENT  4
#define AT_PHNUM  5
#define AT_ENTRY  9

#define MAX_OBJS 16
#define MAX_PHDR 64

/* ------------------------------------------------------------------ */
/* modulos cargados                                                    */
/* ------------------------------------------------------------------ */

typedef struct {
    u64         base;        /* load bias                          */
    u64         nsym;        /* numero de simbolos (DT_HASH o GNU)  */
    Elf64_Sym  *symtab;
    const char *strtab;
    Elf64_Dyn  *dyn;         /* tabla dinamica (para DT_NEEDED)     */
    const char *soname;      /* DT_SONAME (nombre canonico)         */
    char        file[64];    /* como se llamaba el fichero cargado  */
    Elf64_Rela *rela;
    u64         relasz;
    Elf64_Rela *jmprel;
    u64         pltrelsz;
    u64        *init_array;
    u64         init_arraysz;
    const char *rpath;       /* DT_RPATH (rutas separadas por ':')  */
    const char *runpath;     /* DT_RUNPATH                          */
    /* TLS (variables __thread). Ver setup_tls(). */
    u64         tls_memsz;   /* 0 = este modulo no usa TLS          */
    u64         tls_align;
    u64         tls_filesz;
    u64         tls_init;    /* donde estan los datos iniciales     */
    i64         tls_offset;  /* inicio del bloque respecto a TP     */
} mod_t;

static mod_t g_objs[MAX_OBJS];
static int   g_nobjs = 0;

/* Simbolos de datos que el ejecutable se queda copiados (R_X86_64_COPY):
 * nombre -> direccion dentro del ejecutable. Mandan sobre las librerias. */
#define MAX_COPIES 256

static struct {
    const char *name;
    u64         addr;
} g_copies[MAX_COPIES];
static int g_ncopies = 0;

static void collect_copies(mod_t *m) {
    u64 i;
    u64 n = (m->relasz > 0) ? (m->relasz / sizeof(Elf64_Rela)) : 0;

    if (m->rela == (Elf64_Rela *)0 || m->symtab == (Elf64_Sym *)0) {
        return;
    }
    for (i = 0; i < n; i++) {
        Elf64_Rela *r = &m->rela[i];
        u32 si;

        if (ELF64_R_TYPE(r->r_info) != R_X86_64_COPY) {
            continue;
        }
        si = ELF64_R_SYM(r->r_info);
        if (si == 0 || g_ncopies >= MAX_COPIES) {
            continue;
        }
        g_copies[g_ncopies].name = m->strtab + m->symtab[si].st_name;
        g_copies[g_ncopies].addr = m->base + r->r_offset;
        g_ncopies++;
    }
}

/* ------------------------------------------------------------------ */
/* utilidades                                                          */
/* ------------------------------------------------------------------ */

static int str_eq(const char *a, const char *b) {
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

static void die(const char *msg) {
    u64 n = 0;
    while (msg[n] != '\0') {
        n++;
    }
    sys3(SYS_write, 2, (long)msg, (long)n);
    mios_exit(127);
}

/* Como die(), pero diciendo QUE simbolo falta.  Sin esto, un simbolo sin
 * resolver se escribe como 0 y el programa salta a la direccion 0 (un
 * segmentation fault sin pistas). */
static void die_sym(const char *name) {
    static const char pre[] = "ld-mios: simbolo sin resolver: ";
    char buf[256];
    int i = 0, k = 0;

    while (pre[i] != '\0' && i < (int)sizeof(buf) - 3) {
        buf[i] = pre[i];
        i++;
    }
    while (name[k] != '\0' && i < (int)sizeof(buf) - 3) {
        buf[i++] = name[k++];
    }
    buf[i++] = '\n';
    buf[i] = '\0';
    die(buf);
}

static int prot_from_flags(u32 f) {
    int p = 0;
    if (f & PF_R) p |= PROT_READ;
    if (f & PF_W) p |= PROT_WRITE;
    if (f & PF_X) p |= PROT_EXEC;
    return p;
}

static void memzero(void *dst, u64 n) {
    u8 *p = (u8 *)dst;
    u64 i;
    for (i = 0; i < n; i++) {
        p[i] = 0;
    }
}

static void memcopy(void *dst, const void *src, u64 n) {
    u8 *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;
    u64 i;
    for (i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

/* Busca la seccion dinamica entre los program headers. */
static Elf64_Dyn *find_dynamic(Elf64_Phdr *ph, int n, u64 base) {
    int i;
    for (i = 0; i < n; i++) {
        if (ph[i].p_type == PT_DYNAMIC) {
            return (Elf64_Dyn *)(base + ph[i].p_vaddr);
        }
    }
    return (Elf64_Dyn *)0;
}

/* Numero de simbolos de una biblioteca que solo lleva DT_GNU_HASH.
 *
 * La tabla GNU es:  [nbuckets][symoffset][bloom_size][bloom_shift]
 *                   [bloom (bloom_size palabras de 64 bits)] [buckets] [chain...]
 * y NO guarda el total de simbolos.  Se saca recorriendo la cadena de cada
 * bucket hasta la entrada marcada con el bit 0 (la ultima de la cadena); el
 * mayor indice encontrado es el ultimo simbolo.
 *
 * Hace falta porque las bibliotecas que enlaza el gcc del sistema (GMP, MPFR,
 * MPC...) llevan --hash-style=gnu por defecto: sin esto su tabla de simbolos
 * quedaba vacia y TODO lo que exportaban salia sin resolver. */
static u64 gnu_hash_nsyms(const u32 *h) {
    u32 nbuckets = h[0];
    u32 symoffset = h[1];
    u32 bloom_size = h[2];
    const u32 *buckets = h + 4 + bloom_size * 2;   /* 1 palabra de bloom = 2 u32 */
    const u32 *chain = buckets + nbuckets;
    u64 nsym = symoffset;
    u32 i;

    for (i = 0; i < nbuckets; i++) {
        u32 b = buckets[i];
        u64 j;

        if (b < symoffset) {
            continue;
        }
        j = b - symoffset;
        while ((chain[j] & 1u) == 0) {
            j++;
        }
        if ((u64)symoffset + j + 1 > nsym) {
            nsym = (u64)symoffset + j + 1;
        }
    }
    return nsym;
}

/* Rellena un modulo a partir de su tabla dinamica. */
static void parse_dynamic(mod_t *m, Elf64_Dyn *dyn) {
    u64 hash = 0, gnu_hash = 0;
    u64 strtab = 0, symtab = 0, syment = 0;
    u64 rela = 0, relasz = 0;
    u64 jmprel = 0, pltrelsz = 0;
    u64 init_array = 0, init_arraysz = 0;
    u64 rpath = 0, runpath = 0, soname = 0;
    Elf64_Dyn *d;

    for (d = dyn; d->d_tag != DT_NULL; d++) {
        switch (d->d_tag) {
        case DT_HASH:        hash = d->d_val; break;
        case DT_GNU_HASH:    gnu_hash = d->d_val; break;
        case DT_STRTAB:      strtab = d->d_val; break;
        case DT_SYMTAB:      symtab = d->d_val; break;
        case DT_SYMENT:      syment = d->d_val; break;
        case DT_RELA:        rela = d->d_val; break;
        case DT_RELASZ:      relasz = d->d_val; break;
        case DT_JMPREL:      jmprel = d->d_val; break;
        case DT_PLTRELSZ:    pltrelsz = d->d_val; break;
        case DT_INIT_ARRAY:  init_array = d->d_val; break;
        case DT_INIT_ARRAYSZ:init_arraysz = d->d_val; break;
        /* Ojo: RPATH/RUNPATH/SONAME guardan un indice dentro de .dynstr (igual
         * que DT_NEEDED), NO una direccion; se resuelve al final, cuando ya se
         * conoce DT_STRTAB. */
        case DT_RPATH:       rpath = d->d_val; break;
        case DT_RUNPATH:     runpath = d->d_val; break;
        case DT_SONAME:      soname = d->d_val; break;
        default: break;
        }
    }

    (void)syment; /* se asume sizeof(Elf64_Sym) */

    m->dyn = dyn;
    m->strtab = (const char *)(m->base + strtab);
    m->symtab = (Elf64_Sym *)(m->base + symtab);

    if (rpath) {
        m->rpath = m->strtab + rpath;
    }
    if (runpath) {
        m->runpath = m->strtab + runpath;
    }
    if (soname) {
        m->soname = m->strtab + soname;
    }

    if (hash) {
        /* DT_HASH -> [nbucket, nchain, ...]; nchain = numero de simbolos. */
        u32 *h = (u32 *)(m->base + hash);
        m->nsym = h[1];
    } else if (gnu_hash) {
        m->nsym = gnu_hash_nsyms((const u32 *)(m->base + gnu_hash));
    }

    if (rela && relasz) {
        m->rela = (Elf64_Rela *)(m->base + rela);
        m->relasz = relasz;
    }
    if (jmprel && pltrelsz) {
        m->jmprel = (Elf64_Rela *)(m->base + jmprel);
        m->pltrelsz = pltrelsz;
    }
    if (init_array && init_arraysz) {
        m->init_array = (u64 *)(m->base + init_array);
        m->init_arraysz = init_arraysz;
    }
}

/* Apunta lo que hace falta del programa header PT_TLS (variables __thread). */
static void mod_set_tls(mod_t *m, Elf64_Phdr *ph, int nph) {
    int i;

    for (i = 0; i < nph; i++) {
        if (ph[i].p_type != PT_TLS) {
            continue;
        }
        m->tls_memsz  = ph[i].p_memsz;
        m->tls_align  = (ph[i].p_align != 0) ? ph[i].p_align : 1;
        m->tls_filesz = ph[i].p_filesz;
        m->tls_init   = m->base + ph[i].p_vaddr;
    }
}

/* Busca un simbolo definido por nombre en un modulo.
 *
 * Solo se aceptan globales y debiles: los locales (STB_LOCAL) no participan en
 * la resolucion entre modulos, y ademas ni siquiera aparecen en las tablas hash
 * cuando el modulo lleva DT_GNU_HASH. */
static u64 mod_lookup(mod_t *m, const char *name) {
    u64 i;
    if (m->symtab == (Elf64_Sym *)0 || m->strtab == (const char *)0) {
        return 0;
    }
    for (i = 0; i < m->nsym; i++) {
        Elf64_Sym *s = &m->symtab[i];
        if (s->st_shndx == 0 || s->st_name == 0) {
            continue;
        }
        if ((s->st_info >> 4) == 0 /*STB_LOCAL*/) {
            continue;
        }
        if (str_eq(m->strtab + s->st_name, name)) {
            return m->base + s->st_value;
        }
    }
    return 0;
}

/* Resuelve un nombre en el orden de busqueda global. */
static u64 resolve(const char *name) {
    int i;
    u64 v;

    /* Los simbolos que el ejecutable recibe por R_X86_64_COPY mandan sobre
     * cualquier definicion de las librerias (interposicion, como glibc): si
     * no, libmios escribiria en su propia copia y el programa leeria otra. */
    for (i = 0; i < g_ncopies; i++) {
        if (str_eq(g_copies[i].name, name)) {
            return g_copies[i].addr;
        }
    }

    for (i = 0; i < g_nobjs; i++) {
        v = mod_lookup(&g_objs[i], name);
        if (v) {
            return v;
        }
    }
    return 0;
}

/* Busca la DEFINICION de un simbolo (con su tamano) saltando un modulo.
 * Se usa para R_X86_64_COPY: el ejecutable reserva el hueco y hay que traer
 * el valor inicial desde la libreria que define el simbolo. */
static u64 resolve_def(mod_t *skip, const char *name, u64 *size) {
    int i;
    for (i = 0; i < g_nobjs; i++) {
        mod_t *m = &g_objs[i];
        u64 j;
        if (m == skip || m->symtab == (Elf64_Sym *)0 || m->strtab == 0) {
            continue;
        }
        for (j = 0; j < m->nsym; j++) {
            Elf64_Sym *s = &m->symtab[j];
            if (s->st_shndx == 0 || s->st_name == 0) {
                continue;
            }
            if ((s->st_info >> 4) == 0 /*STB_LOCAL*/) {
                continue;
            }
            if (str_eq(m->strtab + s->st_name, name)) {
                if (size) {
                    *size = s->st_size;
                }
                return m->base + s->st_value;
            }
        }
    }
    return 0;
}

static void apply_one(mod_t *m, Elf64_Rela *r) {
    u32 type = ELF64_R_TYPE(r->r_info);
    u32 si = ELF64_R_SYM(r->r_info);
    u64 p = m->base + r->r_offset;
    u64 s = 0;

    if (si != 0) {
        Elf64_Sym *sym = &m->symtab[si];

        /* Un simbolo local (o RELATIVE) es siempre del propio modulo. Uno
         * global definido aqui puede quedar "interpuesto" por el ejecutable
         * (es el caso de los datos que el programa se copia con
         * R_X86_64_COPY: stdout, environ...). Si la resolucion global no da
         * nada, se usa la definicion propia. */
        if (sym->st_shndx != 0 && ((sym->st_info >> 4) == 0 /*STB_LOCAL*/ ||
                                   type == R_X86_64_RELATIVE)) {
            s = m->base + sym->st_value;
        } else {
            s = resolve(m->strtab + sym->st_name);
            if (s == 0 && sym->st_shndx != 0) {
                s = m->base + sym->st_value;
            }
            /* Si sigue siendo 0 es que el simbolo no lo define nadie: para un
             * debil (__gmon_start__ y compania) 0 es lo correcto; para un
             * global, seria un salto a la direccion 0. */
            if (s == 0 && sym->st_shndx == 0 &&
                (sym->st_info >> 4) != 2 /*STB_WEAK*/) {
                die_sym(m->strtab + sym->st_name);
            }
        }
    }

    switch (type) {
    case R_X86_64_RELATIVE:
        *(u64 *)p = m->base + (u64)r->r_addend;
        break;
    case R_X86_64_64:
        *(u64 *)p = s + (u64)r->r_addend;
        break;
    case R_X86_64_GLOB_DAT:
    case R_X86_64_JUMP_SLOT:
        *(u64 *)p = s + (u64)r->r_addend;
        break;
    case R_X86_64_PC32:
        *(u32 *)p = (u32)(s + (u64)r->r_addend - p);
        break;
    case R_X86_64_COPY: {
        /* Copia de datos: el ejecutable tiene el almacenamiento en su .bss y
         * hay que traer el contenido inicial de la libreria que lo define. */
        if (si != 0) {
            const char *nm = m->strtab + m->symtab[si].st_name;
            u64 sz = 0;
            u64 src = resolve_def(m, nm, &sz);
            if (src != 0 && sz != 0) {
                memcopy((void *)p, (void *)src, sz);
            }
        }
        break;
    }
    default:
        break; /* R_X86_64_NONE u otros no usados */
    }
}

static void relocate(mod_t *m) {
    u64 i;
    if (m->rela) {
        for (i = 0; i < m->relasz / sizeof(Elf64_Rela); i++) {
            apply_one(m, &m->rela[i]);
        }
    }
    if (m->jmprel) {
        for (i = 0; i < m->pltrelsz / sizeof(Elf64_Rela); i++) {
            apply_one(m, &m->jmprel[i]);
        }
    }
}

/* Carga una libreria ET_DYN desde disco y rellena su base. */
static void load_library(mod_t *m, const char *path) {
    Elf64_Ehdr eh;
    Elf64_Phdr ph[MAX_PHDR];
    u64 want, got;
    u64 min_vaddr = ~0UL, max_end = 0;
    void *res;
    int fd, i, nph;

    fd = (int)sys3(SYS_open, (long)path, O_RDONLY, 0);
    if (fd < 0) {
        die("ld-mios: no puedo abrir la libreria\n");
    }

    got = (u64)sys3(SYS_read, fd, (long)&eh, (long)sizeof(eh));
    if (got != sizeof(eh) || eh.e_ident[0] != 0x7f || eh.e_ident[1] != 'E') {
        die("ld-mios: ELF invalido\n");
    }
    if (eh.e_phentsize != sizeof(Elf64_Phdr) || eh.e_phnum > MAX_PHDR) {
        die("ld-mios: program headers no soportados\n");
    }
    nph = eh.e_phnum;

    sys3(SYS_lseek, fd, (long)eh.e_phoff, SEEK_SET);
    want = (u64)nph * sizeof(Elf64_Phdr);
    got = (u64)sys3(SYS_read, fd, (long)ph, (long)want);
    if (got != want) {
        die("ld-mios: no puedo leer los program headers\n");
    }

    /* Rango total de la libreria. */
    for (i = 0; i < nph; i++) {
        u64 a, b;
        if (ph[i].p_type != PT_LOAD) {
            continue;
        }
        a = align_down(ph[i].p_vaddr);
        b = align_up(ph[i].p_vaddr + ph[i].p_memsz);
        if (a < min_vaddr) min_vaddr = a;
        if (b > max_end)   max_end = b;
    }

    /* Reserva anonima del rango completo (cubre el bss con ceros). */
    res = mios_mmap((void *)0, max_end - min_vaddr, PROT_NONE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if ((long)res < 0 && (long)res > -4096) {
        die("ld-mios: mmap de reserva fallo\n");
    }
    m->base = (u64)res - min_vaddr;

    /* Mapea cada segmento PT_LOAD sobre la reserva. */
    for (i = 0; i < nph; i++) {
        u64 vstart, inpage, dend, fend, mend;
        int prot;
        if (ph[i].p_type != PT_LOAD) {
            continue;
        }
        vstart = m->base + align_down(ph[i].p_vaddr);
        inpage = ph[i].p_vaddr - align_down(ph[i].p_vaddr);
        dend   = inpage + ph[i].p_filesz;          /* fin real de lo que hay en el fichero */
        fend   = align_up(dend);                   /* final de la pagina parcial             */
        mend   = align_up(inpage + ph[i].p_memsz);
        prot   = prot_from_flags(ph[i].p_flags);

        if (fend) {
            if (dend < fend) {
                /* La cola de la ultima pagina del fichero (de dend a fend) NO son datos
                 * del segmento: alli el enlazador deja el .comment, .symtab, etc. El
                 * kernel lo resuelve con padzero(): mapea el fichero y pone a cero el
                 * sobrante. Sin esto, el .bss que cae en esa misma pagina (log_pos,
                 * log_buffer...) arranca con basura en vez de a cero. */
                mios_mmap((void *)vstart, fend, prot | PROT_WRITE,
                          MAP_PRIVATE | MAP_FIXED, fd, align_down(ph[i].p_offset));
                memzero((void *)(vstart + dend), fend - dend);
                if (!(prot & PROT_WRITE)) {
                    sys3(SYS_mprotect, (long)vstart, (long)fend, prot);
                }
            } else {
                mios_mmap((void *)vstart, fend, prot,
                          MAP_PRIVATE | MAP_FIXED, fd, align_down(ph[i].p_offset));
            }
        }
        if (mend > fend) {
            mios_mmap((void *)(vstart + fend), mend - fend, prot,
                      MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
        }
    }

    {
        Elf64_Dyn *dyn = find_dynamic(ph, nph, m->base);
        if (dyn == (Elf64_Dyn *)0) {
            die("ld-mios: la libreria no tiene PT_DYNAMIC\n");
        }
        parse_dynamic(m, dyn);
    }
    mod_set_tls(m, ph, nph);

    sys1(SYS_close, fd);
}

/* Rutas extra donde buscar librerias (de la variable LD_LIBRARY_PATH). */
static const char *g_ld_library_path = (const char *)0;

/* Copia "dir/name" en dst. Devuelve dst. */
static char *path_join(char *dst, int cap, const char *dir, const char *name) {
    int k = 0;
    int i;

    if (dir != (const char *)0) {
        for (i = 0; dir[i] != '\0' && k < cap - 2; i++) {
            dst[k++] = dir[i];
        }
        if (k > 0 && dst[k - 1] != '/') {
            dst[k++] = '/';
        }
    }
    for (i = 0; name[i] != '\0' && k < cap - 1; i++) {
        dst[k++] = name[i];
    }
    dst[k] = '\0';
    return dst;
}

/* 1 si el fichero se puede abrir. */
static int file_opens(const char *path) {
    int fd = (int)sys3(SYS_open, (long)path, O_RDONLY, 0);
    if (fd < 0) {
        return 0;
    }
    sys1(SYS_close, fd);
    return 1;
}

/* Prueba los directorios de una lista separada por ':' (LD_LIBRARY_PATH o
 * RPATH/RUNPATH) buscando 'name'. Devuelve 1 y deja la ruta hallada en out. */
static int try_dir_list(const char *list, const char *name, char *out, int cap) {
    const char *p = list;

    while (*p != '\0') {
        const char *e = p;
        char dir[256];
        int k = 0;

        while (*e != '\0' && *e != ':') {
            e++;
        }
        while (p < e && k < (int)sizeof(dir) - 1) {
            dir[k++] = *p++;
        }
        dir[k] = '\0';
        if (k > 0) {
            path_join(out, cap, dir, name);
            if (file_opens(out)) {
                return 1;
            }
        }
        if (*e == ':') {
            e++;
        }
        p = e;
    }
    return 0;
}

/* Busca 'name' en LD_LIBRARY_PATH, en el RPATH/RUNPATH del ejecutable y, por
 * ultimo, en /lib. Devuelve 1 si la encuentra. */
static int find_library(const char *name, char *out, int cap) {
    if (g_ld_library_path != (const char *)0 &&
        try_dir_list(g_ld_library_path, name, out, cap)) {
        return 1;
    }
    if (g_objs[0].runpath != (const char *)0 &&
        try_dir_list(g_objs[0].runpath, name, out, cap)) {
        return 1;
    }
    if (g_objs[0].rpath != (const char *)0 &&
        try_dir_list(g_objs[0].rpath, name, out, cap)) {
        return 1;
    }
    path_join(out, cap, "/lib", name);
    return file_opens(out);
}

/* ¿Esta ya cargado 'name'?  Se compara con el DT_SONAME de cada modulo y con
 * el nombre del fichero que se cargo (asi libgmp.so.10 y libgmp.so.10.4.1 no
 * se cargan dos veces: una misma biblioteca puede aparecer en las DT_NEEDED de
 * varias). */
static int module_loaded(const char *name) {
    int i;
    for (i = 0; i < g_nobjs; i++) {
        const mod_t *m = &g_objs[i];
        if (m->soname != (const char *)0 && str_eq(m->soname, name)) {
            return 1;
        }
        if (m->file[0] != '\0' && str_eq(m->file, name)) {
            return 1;
        }
    }
    return 0;
}

/* Guarda el nombre del fichero (lo que hay detras de la ultima '/'). */
static void mod_set_file(mod_t *m, const char *path) {
    const char *base = path;
    int k = 0;

    while (*path != '\0') {
        if (*path == '/') {
            base = path + 1;
        }
        path++;
    }
    while (base[k] != '\0' && k < (int)sizeof(m->file) - 1) {
        m->file[k] = base[k];
        k++;
    }
    m->file[k] = '\0';
}

/* Carga las DT_NEEDED de un modulo y, recursivamente, las de las librerias que
 * va cargando: una libreria puede necesitar otra que el ejecutable no nombra
 * (libmpc -> libmpfr -> libgmp, por ejemplo). */
static void load_needed(mod_t *m) {
    Elf64_Dyn *d;

    if (m->dyn == (Elf64_Dyn *)0 || m->strtab == (const char *)0) {
        return;
    }
    for (d = m->dyn; d->d_tag != DT_NULL; d++) {
        const char *name;
        char path[512];

        if (d->d_tag != DT_NEEDED) {
            continue;
        }
        name = m->strtab + d->d_val;
        if (module_loaded(name)) {
            continue;
        }

        if (!find_library(name, path, (int)sizeof(path))) {
            die("ld-mios: no encuentro la libreria\n");
        }

        if (g_nobjs >= MAX_OBJS) {
            die("ld-mios: demasiadas librerias\n");
        }
        g_objs[g_nobjs].base = 0;
        load_library(&g_objs[g_nobjs], path);
        mod_set_file(&g_objs[g_nobjs], path);
        g_nobjs++;
        load_needed(&g_objs[g_nobjs - 1]);
    }
}

/* ------------------------------------------------------------------ */
/* TLS (variables __thread)                                            */
/* ------------------------------------------------------------------ */
/*
 * Un programa con variables __thread tiene un programa header PT_TLS. Con el
 * modelo "local-exec" (el ejecutable, que es el que genera gcc sin -fPIC) el
 * enlazador direcciona la variable que esta en el desplazamiento k del bloque
 * como %fs:(k - S), siendo S el tamano del bloque alineado. Es decir: el
 * bloque del EJECUTABLE tiene que acabar justo en el puntero de hilo (TP) y
 * %fs tiene que apuntar ahi.
 *
 * El reparto es el de glibc: desde TP hacia abajo, primero el ejecutable y
 * despues los bloques de las librerias; por encima de TP va el TCB (cuya
 * primera palabra es el auto-puntero que esperan pthread/tls).
 *
 * MiOS no crea hilos (fork() hereda %fs), asi que con prepararlo una vez,
 * antes de relocalizar y de los constructores, es suficiente.
 */

static u64 g_tls_tp = 0;

/* Alineacion con potencias de dos (lo que garantiza PT_TLS). */
static u64 align_down_pow2(u64 x, u64 a) {
    return x & ~(a - 1);
}

static u64 align_up_pow2(u64 x, u64 a) {
    return align_down_pow2(x + a - 1, a);
}

static void setup_tls(void) {
    u64 need = 0;
    u64 maxalign = 16;              /* %fs bien alineado aunque PT_TLS pida 1 */
    u64 capacity;
    void *page;
    u64 top;
    int i;

    for (i = 0; i < g_nobjs; i++) {
        mod_t *m = &g_objs[i];

        if (m->tls_memsz == 0) {
            continue;
        }
        if (m->tls_align > maxalign) {
            maxalign = m->tls_align;
        }
        need += align_up_pow2(m->tls_memsz, m->tls_align) + m->tls_align;
    }
    if (need == 0) {
        return;                     /* el programa no usa __thread */
    }

    capacity = align_up_pow2(need + 256, MIOS_PAGE);   /* +256: TCB y margen */
    page = mios_mmap((void *)0, capacity, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if ((long)page < 0 && (long)page > -4096) {
        die("ld-mios: no puedo reservar el bloque TLS\n");
    }
    /* tp == fin del bloque del ejecutable: alineado a maxalign, con lo que
     * (tp - S) tambien queda alineado a la alineacion de cada segmento. */
    g_tls_tp = align_down_pow2((u64)page + capacity - 256, maxalign);
    top = g_tls_tp;

    for (i = 0; i < g_nobjs; i++) {
        mod_t *m = &g_objs[i];
        u64 size;
        u64 start;

        if (m->tls_memsz == 0) {
            continue;
        }
        size = align_up_pow2(m->tls_memsz, m->tls_align);
        start = align_down_pow2(top - size, m->tls_align);
        m->tls_offset = (i64)start - (i64)g_tls_tp;
        if (m->tls_filesz != 0) {
            memcopy((void *)start, (void *)m->tls_init, m->tls_filesz);
        }
        top = start;
    }

    /* TCB: la primera palabra de %fs es el auto-puntero (pthread_self). */
    *(u64 *)g_tls_tp = g_tls_tp;

    if (sys3(SYS_arch_prctl, ARCH_SET_FS, (long)g_tls_tp, 0) != 0) {
        die("ld-mios: no puedo poner %fs (arch_prctl)\n");
    }
}

/* ------------------------------------------------------------------ */
/* arranque del cargador                                               */
/* ------------------------------------------------------------------ */

static void run_init_array(mod_t *m) {
    u64 i, n;
    if (m->init_array == (u64 *)0) {
        return;
    }
    n = m->init_arraysz / sizeof(u64);
    for (i = 0; i < n; i++) {
        void (*fn)(void) = (void (*)(void))m->init_array[i];
        if (fn) {
            fn();
        }
    }
}

__attribute__((naked, noreturn)) static void jump_to(void *entry __attribute__((unused)),
                                                     void *sp __attribute__((unused))) {
    __asm__ volatile(
        "mov %rsi, %rsp\n"
        "jmp *%rdi\n");
}

/* No es static: _start lo invoca por nombre desde asm y debe existir como
 * simbolo (si fuera static, el compilador lo descartaria por no "usarse"). */
__attribute__((used)) void loader_main(void *stack) {
    u64 *sp = (u64 *)stack;
    u64 argc = sp[0];
    char **envp = (char **)&sp[1] + argc + 1; /* -> envp[0] */
    char **env = envp;                        /* envp[0], antes de avanzar  */
    u64 *auxv;
    u64 at_phdr = 0, at_phnum = 0, at_entry = 0;
    int i;

    /* LD_LIBRARY_PATH: solo se usa para dar una ruta alternativa a libmios
     * fuera de MiOS (pruebas con chroot o en el propio VPS); en MiOS la
     * libreria esta en /lib y no hace falta. */
    for (i = 0; env[i] != (char *)0; i++) {
        const char *e = env[i];
        const char *p = "LD_LIBRARY_PATH=";
        int k = 0;
        while (p[k] != '\0' && e[k] == p[k]) {
            k++;
        }
        if (p[k] == '\0') {
            g_ld_library_path = e + k;
            break;
        }
    }

    /* auxv va justo despues del NULL que cierra envp. */
    while (*envp != (char *)0) {
        envp++;
    }
    auxv = (u64 *)(envp + 1);
    for (i = 0; auxv[i] != AT_NULL; i += 2) {
        switch (auxv[i]) {
        case AT_PHDR:  at_phdr = auxv[i + 1]; break;
        case AT_PHNUM: at_phnum = auxv[i + 1]; break;
        case AT_ENTRY: at_entry = auxv[i + 1]; break;
        default: break;
        }
    }
    if (!at_phdr || !at_entry) {
        die("ld-mios: auxv incompleto\n");
    }

    /* El ejecutable es ET_EXEC: su base es 0. */
    g_objs[0].base = 0;
    {
        Elf64_Phdr *ph = (Elf64_Phdr *)at_phdr;
        Elf64_Dyn *dyn = find_dynamic(ph, (int)at_phnum, 0);
        if (dyn == (Elf64_Dyn *)0) {
            die("ld-mios: el ejecutable no tiene PT_DYNAMIC\n");
        }
        parse_dynamic(&g_objs[0], dyn);
        g_nobjs = 1;
        mod_set_tls(&g_objs[0], ph, (int)at_phnum);

        /* Antes de nada: que datos copia el ejecutable (R_X86_64_COPY). */
        collect_copies(&g_objs[0]);

        /* Carga las librerias necesarias (busqueda global incluye a todas). */
        load_needed(&g_objs[0]);
    }

    /* TLS: tiene que estar listo antes de las relocalizaciones (por si alguna
     * lo necesita) y, desde luego, antes de los constructores. */
    setup_tls();

    /* Relocaliza: primero librerias, luego el ejecutable. */
    for (i = 1; i < g_nobjs; i++) {
        relocate(&g_objs[i]);
    }
    relocate(&g_objs[0]);

    /* Constructores: librerias antes que el ejecutable. */
    for (i = 1; i < g_nobjs; i++) {
        run_init_array(&g_objs[i]);
    }
    run_init_array(&g_objs[0]);

    jump_to((void *)at_entry, stack);
    for (;;) {
    }
}

__attribute__((naked, noreturn)) void _start(void) {
    __asm__ volatile(
        "mov %rsp, %rdi\n"
        "call loader_main\n"
        "ud2\n");
}
