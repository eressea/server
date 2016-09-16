#include "version.h"

#include <stdio.h>

#ifndef ERESSEA_VERSION
// the version number, if it was not passed to make with -D
#define ERESSEA_VERSION "3.10.0-devel"
#endif

const char *eressea_version(void) {
    return ERESSEA_VERSION;
}

int version_no(const char *str) {
    int maj = 0, min = 0, bld = 0;
    sscanf(str, "%d.%d.%d", &maj, &min, &bld);
    return (maj << 16) | (min << 8) | bld;
}
