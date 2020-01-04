#ifndef H_UTIL_NRMESSAGE
#define H_UTIL_NRMESSAGE

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct locale;
    struct message;
    struct message_type;
    struct nrmessage_type;

    void free_nrmesssages(void);

    void nrt_register(const struct message_type *mtype);
    const char *nrt_string(const struct message_type *mtype,
            const struct locale *lang);

    size_t nr_render(const struct message *msg, const struct locale *lang,
        char *buffer, size_t size, const void *userdata);

#ifdef __cplusplus
}
#endif
#endif
