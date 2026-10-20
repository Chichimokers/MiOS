#ifndef MIOS_ASSERT_H
#define MIOS_ASSERT_H

/*
 * MiOS - assert.h (cabecera publica).
 */

void __assert_fail(const char *expr, const char *file, unsigned int line,
                   const char *func) __attribute__((noreturn));

#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
#define assert(expr)                                                     \
    ((expr) ? (void)0                                                    \
            : __assert_fail(#expr, __FILE__, __LINE__, __func__))
#endif

#endif /* MIOS_ASSERT_H */
