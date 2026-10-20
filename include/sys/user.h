#ifndef MIOS_SYS_USER_H
#define MIOS_SYS_USER_H

/*
 * MiOS - sys/user.h (cabecera publica).
 *
 * Registros del proceso tal y como los ve el fichero "core" (y ptrace).  Las
 * guardas permiten incluir esta cabecera y <sys/procfs.h> en cualquier orden.
 */

#ifndef MIOS_DEF_USER_REGS_STRUCT
#define MIOS_DEF_USER_REGS_STRUCT
struct user_regs_struct {
    unsigned long long r15, r14, r13, r12, rbp, rbx, r11, r10, r9, r8;
    unsigned long long rax, rcx, rdx, rsi, rdi, orig_rax;
    unsigned long long rip, cs, eflags, rsp, ss;
    unsigned long long fs_base, gs_base, ds, es, fs, gs;
};
#endif

#ifndef MIOS_DEF_USER_FPREGS_STRUCT
#define MIOS_DEF_USER_FPREGS_STRUCT
struct user_fpregs_struct {
    unsigned short cwd, swd, ftw, fop;
    unsigned long long rip, rdp;
    unsigned int mxcsr, mxcr_mask;
    unsigned int st_space[32];
    unsigned int xmm_space[64];
    unsigned int padding[24];
};
#endif

#endif /* MIOS_SYS_USER_H */
