#ifndef H_KRNL_NAMES
#define H_KRNL_NAMES

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct unit;
    void register_names(void);
    const char *abkz(const char *s, char *buf, size_t size, size_t maxchars);

#ifdef __cplusplus
}
#endif
#endif
