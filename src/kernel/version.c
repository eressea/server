#include <platform.h>
#include "version.h"

#include <stdio.h>

#ifndef ERESSEA_VERSION
/* the version number, if it was not passed to make with -D */
#define ERESSEA_VERSION "3.12.0"
#endif

const char *eressea_version(void) {
#ifdef ERESSEA_BUILDNO
    return ERESSEA_VERSION "-" ERESSEA_BUILDNO;
#else
    return ERESSEA_VERSION;
#endif
}

int version_no(const char *str) {
    int maj = 0, min = 0, pat = 0;
    sscanf(str, "%d.%d.%d", &maj, &min, &pat);
    return (maj << 16) | (min << 8) | pat;
}
