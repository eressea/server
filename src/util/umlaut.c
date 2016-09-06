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

#include <platform.h>
#include "umlaut.h"

#include "assert.h"
#include "log.h"
#include "unicode.h"

#include <ctype.h>
#include <wctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct tref {
    struct tref *nexthash;
    ucs4_t ucs;
    struct tnode *node;
} tref;

#define LEAF 1                  /* leaf node for a word. always matches */
#define SHARED 2                /* at least two words share the node */

typedef struct tnode {
    struct tref *next[NODEHASHSIZE];
    unsigned char flags;
    variant id;
    int refcount;
} tnode;

char * transliterate(char * out, size_t size, const char * in)
{
    const char *src = in;
    char *dst = out;

    assert(in && size > 0);
    --size; /* need space for a final 0-byte */
    while (*src && size) {
        size_t len;
        const char * p = src;
        while ((p + size > src) && *src && (~*src & 0x80)) {
            *dst++ = (char)tolower(*src++);
        }
        len = src - p;
        size -= len;
        while (size > 0 && *src && (*src & 0x80)) {
            unsigned int advance = 2;
            if (src[0] == '\xc3') {
                if (src[1] == '\xa4' || src[1] == '\x84') {
                    memcpy(dst, "ae", 2);
                }
                else if (src[1] == '\xb6' || src[1] == '\x96') {
                    memcpy(dst, "oe", 2);
                }
                else if (src[1] == '\xbc' || src[1] == '\x9c') {
                    memcpy(dst, "ue", 2);
                }
                else if (src[1] == '\x9f') {
                    memcpy(dst, "ss", 2);
                }
                else {
                    advance = 0;
                }
            }
            else if (src[0] == '\xe1') {
                if (src[1] == '\xba' && src[2] == '\x9e') {
                    memcpy(dst, "ss", 2);
                    ++src;
                }
                else {
                    advance = 0;
                }
            }
            else {
                advance = 0;
            }

            if (advance && advance <= size) {
                src += advance;
                dst += advance;
                size -= advance;
            }
            else {
                ucs4_t ucs;
                int ret = unicode_utf8_to_ucs4(&ucs, src, &len);
                if (ret != 0) {
                    /* encoding is broken. yikes */
                    log_error("transliterate | encoding error in '%s'\n", src);
                    return NULL;
                }
                src += len;
                *dst++ = '?';
                --size;
            }
        }
    }
    *dst = 0;
    return *src ? 0 : out;
}

tnode * mknode(void) {
    tnode * node = calloc(1, sizeof(tnode));
    node->refcount = 1;
    return node;
}

void addtoken(tnode ** root, const char *str, variant id)
{
    tnode * tk;
    static const struct replace {
        ucs4_t ucs;
        const char str[3];
    } replace[] = {
        /* match lower-case (!) umlauts and others to transcriptions */
        { 228, "AE" }, { 246, "OE" }, { 252, "UE" }, { 223, "SS" },
        { 230, "AE" }, { 248, "OE" }, { 229, "AA" }, { 0, "" }
    };

    assert(root && str);
    if (!*root) {
        tk = *root = mknode();
    }
    else {
        tk = *root;
    }
    assert(tk && tk == *root);
    if (!*str) {
        tk->id = id;
        tk->flags |= LEAF;
    }
    else {
        tref *next;
        int ret, index, i = 0;
        ucs4_t ucs, lcs;
        size_t len;

        ret = unicode_utf8_to_ucs4(&ucs, str, &len);
        assert(ret == 0 || !"invalid utf8 string");
        lcs = ucs;

#if NODEHASHSIZE == 8
        index = ucs & 7;
#else
        index = ucs % NODEHASHSIZE;
#endif
        assert(index >= 0);
        next = tk->next[index];
        if (!(tk->flags & LEAF))
            tk->id = id;
        while (next && next->ucs != ucs)
            next = next->nexthash;
        if (!next) {
            tref *ref;
            tnode *node = mknode(); // TODO: what is the reason for this empty node to exist?

            if (ucs < 'a' || ucs > 'z') {
                lcs = towlower((wint_t)ucs);
            }
            if (ucs == lcs) {
                ucs = towupper((wint_t)ucs);
            }

            ref = (tref *)malloc(sizeof(tref));
            ref->ucs = ucs;
            ref->node = node;
            ref->nexthash = tk->next[index];
            tk->next[index] = ref;

            /* try lower/upper casing the character, and try again */
            if (ucs != lcs) {
#if NODEHASHSIZE == 8
                index = lcs & 7;
#else
                index = lcs % NODEHASHSIZE;
#endif
                ref = (tref *)malloc(sizeof(tref));
                assert_alloc(ref);
                ref->ucs = lcs;
                ref->node = node;
                ++node->refcount;
                ref->nexthash = tk->next[index];
                tk->next[index] = ref;
            }
            next = ref;
        }
        else {
            tnode * next_node = (tnode *)next->node;
            next_node->flags |= SHARED;
            if ((next_node->flags & LEAF) == 0)
                next_node->id.v = NULL;        /* why? */
        }
        addtoken(&next->node, str + len, id);
        while (replace[i].str[0]) {
            if (lcs == replace[i].ucs) {
                char zText[1024];
                memcpy(zText, replace[i].str, 3);
                strcpy(zText + 2, (const char *)str + len);
                addtoken(root, zText, id);
                break;
            }
            ++i;
        }
    }
}

void freetokens(tnode * root)
{
    tnode * node = root;
    int i;
    for (i = 0; node && i != NODEHASHSIZE; ++i) {
        if (node->next[i]) {
            tref * ref = node->next[i];
            while (ref) {
                tref * next = ref->nexthash;
                freetokens(ref->node);
                free(ref);
                ref = next;
            }
            node->next[i] = 0;
        }
    }
    if (--node->refcount == 0) {
        free(node);
    }
}

int findtoken(const void * root, const char *key, variant * result)
{
    const char * str = key;
    const tnode * tk = (const tnode *)root;

    if (!tk || !str || *str == 0) {
        return E_TOK_NOMATCH;
    }
    do {
        int index;
        const tref *ref;
        ucs4_t ucs;
        size_t len;
        int ret = unicode_utf8_to_ucs4(&ucs, str, &len);

        if (ret != 0) {
            /* encoding is broken. youch */
            log_error("findtoken | encoding error in '%s'\n", key);
            return E_TOK_NOMATCH;
        }
#if NODEHASHSIZE == 8
        index = ucs & 7;
#else
        index = ucs % NODEHASHSIZE;
#endif
        ref = tk->next[index];
        while (ref && ref->ucs != ucs)
            ref = ref->nexthash;
        str += len;
        if (!ref) {
            log_debug("findtoken | token not found '%s'\n", key);
            return E_TOK_NOMATCH;
        }
        tk = ref->node;
    } while (*str);
    if (tk) {
        *result = tk->id;
        return E_TOK_SUCCESS;
    }
    log_debug("findtoken | token not found '%s'\n", key);
    return E_TOK_NOMATCH;
}
