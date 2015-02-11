/* 
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */

#ifndef H_UTIL_CRMESSAGE
#define H_UTIL_CRMESSAGE

#include "variant.h"
#ifdef __cplusplus
extern "C" {
#endif

    struct locale;
    struct message;
    struct message_type;

    typedef int(*tostring_f) (variant data, char *buffer, const void *userdata);
    void tsf_register(const char *name, tostring_f fun);
    /* registers a new type->string-function */

    int cr_string(variant v, char *buffer, const void *userdata);
    int cr_int(variant v, char *buffer, const void *userdata);
    int cr_ignore(variant v, char *buffer, const void *userdata);

    void crt_register(const struct message_type *mtype);
    int cr_render(const struct message *msg, char *buffer,
        const void *userdata);

#ifdef __cplusplus
}
#endif
#endif
