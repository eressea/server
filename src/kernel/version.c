#include "version.h"

#ifndef ERESSEA_VERSION
// the version number, if it was not passed to make with -D
#define ERESSEA_VERSION "3.10.0-devel"
#endif

const char *eressea_version(void) {
    return ERESSEA_VERSION;
}
