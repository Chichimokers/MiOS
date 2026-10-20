#ifndef MIOS_SYS_PROCFS_H
#define MIOS_SYS_PROCFS_H

/*
 * MiOS - sys/procfs.h (cabecera publica).
 *
 * El formato de los ficheros "core" de ELF en Linux: la parte que comparten
 * GDB y el soporte de core de bfd (bfd/elfcore.h).  En glibc vive en esta
 * cabecera; en MiOS tambien, porque binutils no compila su backend de x86_64
 * sin ella (bfd/hosts/x86-64linux.h la incluye).
 *
 * Las estructuras son las de Linux x86_64 (BFD comprueba sus tamanos: 336
 * bytes para elf_prstatus y 136 para elf_prpsinfo).
 */

#include "types.h"
#include "user.h"
#include "../time.h"

/* Registros de proposito general (27 en x86_64). */
typedef unsigned long long elf_greg_t;

#define ELF_NGREG (sizeof(struct user_regs_struct) / sizeof(elf_greg_t))
typedef elf_greg_t elf_gregset_t[ELF_NGREG];

/* Registros de coma flotante: solo se usa el puntero. */
struct _libc_fpstate;
typedef struct _libc_fpstate *elf_fpregset_t;

/* Informacion de la senal que provoco el volcado.  (Con tag: el codigo de
 * bfd escribe 'struct elf_siginfo', como en glibc.) */
typedef struct elf_siginfo {
    int si_signo;
    int si_code;
    int si_errno;
} elf_siginfo;

/* Estado del proceso en el momento del volcado. */
typedef struct elf_prstatus {
    elf_siginfo    pr_info;
    short int      pr_cursig;
    unsigned long  pr_sigpend;
    unsigned long  pr_sighold;
    pid_t          pr_pid;
    pid_t          pr_ppid;
    pid_t          pr_pgrp;
    pid_t          pr_sid;
    struct timeval pr_utime;
    struct timeval pr_stime;
    struct timeval pr_cutime;
    struct timeval pr_cstime;
    elf_gregset_t  pr_reg;
    int            pr_fpvalid;
} elf_prstatus;

/* Argumentos (recortados) del programa volcado. */
#define ELF_PRARGSZ (80)

typedef struct elf_prpsinfo {
    char           pr_state;
    char           pr_sname;
    char           pr_zomb;
    char           pr_nice;
    unsigned long  pr_flag;
    unsigned int   pr_uid;
    unsigned int   pr_gid;
    int            pr_pid, pr_ppid, pr_pgrp, pr_sid;
    char           pr_fname[16];
    char           pr_psargs[ELF_PRARGSZ];
} elf_prpsinfo;

/* Nombres cortos que espera el codigo generico (bfd/elfcore.h). */
typedef elf_greg_t     greg_t;
typedef elf_gregset_t  gregset_t;
typedef elf_fpregset_t fpregset_t;
typedef elf_prstatus   prstatus_t;
typedef elf_prpsinfo   prpsinfo_t;

#define PRARGSZ ELF_PRARGSZ

#endif /* MIOS_SYS_PROCFS_H */
