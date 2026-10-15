#ifndef MIOS_LIB_CONVERT_H
#define MIOS_LIB_CONVERT_H

#include "lib/types.h"

int mios_digit(int c);
int mios_atoi(const char *s);
long mios_atol(const char *s);
unsigned long mios_strtoul(const char *s, int base);

#endif /* MIOS_LIB_CONVERT_H */
