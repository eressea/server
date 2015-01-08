/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
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

#ifdef __cplusplus
extern "C" {
#endif

    struct gamedata;
    struct storage;
    typedef void(*afun) (void);

    typedef struct attrib {
        const struct attrib_type *type;
        union {
            afun f;
            void *v;
            int i;
            float flt;
            char c;
            short s;
            short sa[2];
            char ca[4];
        } data;
        /* internal data, do not modify: */
        struct attrib *next;        /* next attribute in the list */
        struct attrib *nexttype;    /* skip to attribute of a different type */
    } attrib;

#define ATF_UNIQUE   (1<<0)     /* only one per attribute-list */
#define ATF_PRESERVE (1<<1)     /* preserve order in list. append to back */
#define ATF_USER_DEFINED (1<<2) /* use this to make udf */

    typedef struct attrib_type {
        const char *name;
        void(*initialize) (struct attrib *);
        void(*finalize) (struct attrib *);
        int(*age) (struct attrib *);
        /* age returns 0 if the attribute needs to be removed, !=0 otherwise */
        void(*write) (const struct attrib *, const void *owner, struct storage *);
        int(*read) (struct attrib *, void *owner, struct storage *);       /* return AT_READ_OK on success, AT_READ_FAIL if attrib needs removal */
        unsigned int flags;
        /* ---- internal data, do not modify: ---- */
        struct attrib_type *nexthash;
        unsigned int hashkey;
    } attrib_type;

    extern void at_register(attrib_type * at);
    extern void at_deprecate(const char * name, int(*reader)(attrib *, void *, struct storage *));

    extern attrib *a_select(attrib * a, const void *data,
        bool(*compare) (const attrib *, const void *));
    extern attrib *a_find(attrib * a, const attrib_type * at);
    extern const attrib *a_findc(const attrib * a, const attrib_type * at);
    extern attrib *a_add(attrib ** pa, attrib * at);
    extern int a_remove(attrib ** pa, attrib * at);
    extern void a_removeall(attrib ** a, const attrib_type * at);
    extern attrib *a_new(const attrib_type * at);

    extern int a_age(attrib ** attribs);
    extern int a_read(struct storage *store, attrib ** attribs, void *owner);
    extern void a_write(struct storage *store, const attrib * attribs,
        const void *owner);

    void free_attribs(void);

#define DEFAULT_AGE NULL
#define DEFAULT_INIT NULL
#define DEFAULT_FINALIZE NULL
#define NO_WRITE NULL
#define NO_READ NULL

#define AT_READ_OK 0
#define AT_READ_FAIL -1

#define AT_AGE_REMOVE 0         /* remove the attribute after calling age() */
#define AT_AGE_KEEP 1           /* keep the attribute for another turn */

#ifdef __cplusplus
}
#endif
#endif
