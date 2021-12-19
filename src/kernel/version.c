#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "version.h"

#include <assert.h>
#include <stdio.h>

#ifndef ERESSEA_VERSION
/* the version number, if it was not passed to make with -D */
#define ERESSEA_VERSION "26.3.0"
#endif

const char *eressea_version(void) {
#ifdef ERESSEA_BUILDNO
    return ERESSEA_VERSION "-" ERESSEA_BUILDNO;
#else
    return ERESSEA_VERSION;
#endif
}

int version_no(const char *str) {
    int c, maj = 0, min = 0, pat = 0;
    c = sscanf(str, "%4d.%4d.%4d", &maj, &min, &pat);
    assert(c == 3);
    return (maj << 16) | (min << 8) | pat;
}
