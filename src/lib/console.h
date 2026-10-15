#ifndef MIOS_LIB_CONSOLE_H
#define MIOS_LIB_CONSOLE_H

#include "lib/types.h"

void cons_printf(const char *fmt, ...);
void cons_puts(const char *s);
void cons_putc(char c);
void cons_flush(void);
void cons_log_dump(void);
int  cons_read_line(char *buf, mios_size_t size);

#endif /* MIOS_LIB_CONSOLE_H */
