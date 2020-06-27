#ifndef _UMLAUT_H
#define _UMLAUT_H

#include "variant.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define E_TOK_NOMATCH (-1)
#define E_TOK_SUCCESS 0
#define NODEHASHSIZE 8
    struct tnode;

    int findtoken(const void *tk, const char *str, variant * result);
    void addtoken(struct tnode **root, const char *str, variant id);
    void freetokens(struct tnode *root);

    char * transliterate(char * out, size_t size, const char * in);

    typedef struct local_names {
        struct local_names *next;
        const struct locale *lang;
        void * names;
    } local_names;

#ifdef __cplusplus
}
#endif
#endif
