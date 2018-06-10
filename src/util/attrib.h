/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#ifndef ATTRIB_H
#define ATTRIB_H

#include <stdbool.h>
#include "variant.h"
#ifdef __cplusplus
extern "C" {
#endif

    union variant;
    struct gamedata;
    struct storage;
    typedef void(*afun) (void);

    typedef struct attrib {
        const struct attrib_type *type;
        union variant data;
        /* internal data, do not modify: */
        struct attrib *next;        /* next attribute in the list */
        struct attrib *nexttype;    /* skip to attribute of a different type */
    } attrib;

#define ATF_UNIQUE   (1<<0)     /* only one per attribute-list */
#define ATF_PRESERVE (1<<1)     /* preserve order in list. append to back */
#define ATF_USER_DEFINED (1<<2) /* use this to make udf */

    typedef struct attrib_type {
        const char *name;
        void(*initialize) (union variant *);
        void(*finalize) (union variant *);
        int(*age) (struct attrib *, void *owner);
        /* age returns 0 if the attribute needs to be removed, !=0 otherwise */
        void(*write) (const union variant *, const void *owner, struct storage *);
        int(*read) (union variant *, void *owner, struct gamedata *);       /* return AT_READ_OK on success, AT_READ_FAIL if attrib needs removal */
        void(*upgrade) (struct attrib **alist, struct attrib *a);
        unsigned int flags;
        /* ---- internal data, do not modify: ---- */
        struct attrib_type *nexthash;
        unsigned int hashkey;
    } attrib_type;

    void at_register(attrib_type * at);
    void at_deprecate(const char * name, int (*reader)(variant *, void *, struct gamedata *));
    struct attrib_type *at_find(const char *name);

    void write_attribs(struct storage *store, struct attrib *alist, const void *owner);
    int read_attribs(struct gamedata *store, struct attrib **alist, void *owner);

    attrib *a_select(attrib * a, const void *data, bool(*compare) (const attrib *, const void *));
    attrib *a_find(attrib * a, const attrib_type * at);
    attrib *a_add(attrib ** pa, attrib * at);
    int a_remove(attrib ** pa, attrib * at);
    void a_removeall(attrib ** a, const attrib_type * at);
    attrib *a_new(const attrib_type * at);
    int a_age(attrib ** attribs, void *owner);

    void a_free_voidptr(union variant *v);
    int a_read_orig(struct gamedata *data, attrib ** attribs, void *owner);
    int a_read(struct gamedata *data, attrib ** attribs, void *owner);
    void a_write(struct storage *store, const attrib * attribs, const void *owner);

    int a_readint(union variant *v, void *owner, struct gamedata *);
    void a_writeint(const union variant *v, const void *owner,
        struct storage *store);
    int a_readshorts(union variant *v, void *owner, struct gamedata *);
    void a_writeshorts(const union variant *v, const void *owner,
        struct storage *store);
    int a_readchars(union variant *v, void *owner, struct gamedata *);
    void a_writechars(const union variant *v, const void *owner,
        struct storage *store);
    int a_readstring(union variant *v, void *owner, struct gamedata *);
    void a_writestring(const union variant *v, const void *owner,
        struct storage *);
    void a_finalizestring(union variant *v);

    void attrib_done(void);

#define DEFAULT_AGE NULL
#define DEFAULT_INIT NULL
#define DEFAULT_FINALIZE NULL
#define NO_WRITE NULL
#define NO_READ NULL

#define AT_READ_OK 0
#define AT_READ_FAIL -1
#define AT_READ_DEPR 1 /* a deprecated attribute was read, should run a_upgrade */

#define AT_AGE_REMOVE 0         /* remove the attribute after calling age() */
#define AT_AGE_KEEP 1           /* keep the attribute for another turn */

#ifdef __cplusplus
}
#endif
#endif
