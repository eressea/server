#ifndef H_UTIL_CRMESSAGE
#define H_UTIL_CRMESSAGE

#include "variant.h"
#ifdef __cplusplus
extern "C" {
#endif

    struct locale;
    struct message;
    struct message_type;

    void crmessage_done(void);

    typedef int(*tostring_f) (variant data, const char *name, char *buffer, const void *userdata);
    void tsf_register(const char *name, tostring_f fun);
    /* registers a new type->string-function */

    int cr_string(variant v, const char *name, char *buffer, const void *userdata);
    int cr_int(variant v, const char *name, char *buffer, const void *userdata);
    int cr_ignore(variant v, const char *name, char *buffer, const void *userdata);

    void crt_register(const struct message_type *mtype);
    size_t cr_render(const struct message *msg, char *buffer,
        const void *userdata);

#ifdef __cplusplus
}
#endif
#endif
