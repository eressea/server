#ifdef _MSC_VER
#include <platform.h>
#endif

#include "bind_locale.h"
#include "util/language.h"
#include "direction.h"

void locale_create(const char *lang) {
    get_or_create_locale(lang);
}

void locale_set(const char *lang, const char *key, const char *str) {
    struct locale *loc = get_locale(lang);
    if (loc) {
        locale_setstring(loc, key, str);
    }
}

const char * locale_get(const char *lang, const char *key) {
    struct locale *loc = get_locale(lang);
    if (loc) {
        return locale_getstring(loc, key);
    }
    return 0;
}

int locale_direction(const char *lang, const char *str) {
    struct locale *loc = get_locale(lang);
    if (loc) {
        return get_direction(str, loc);
    }
    return NODIRECTION;
}
