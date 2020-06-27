#ifndef H_UTIL_BASE36
#define H_UTIL_BASE36

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

    const char *itoa36(int i);
    int atoi36(const char *s);

    const char *itoa36_r(int i, char *result, size_t len);
    const char *itoab_r(int i, int base, char *result, size_t len);
    const char *itoab(int i, int base);
    const char *itoa10(int i);

#ifdef __cplusplus
}
#endif
#endif
