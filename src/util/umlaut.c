#include "umlaut.h"

#include "assert.h"
#include "log.h"
#include "strings.h"
#include "unicode.h"

#include <ctype.h>
#include <wctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct tref {
    struct tref *nexthash;
    wchar_t wc;
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
                wchar_t wc;
                int ret = utf8_decode(&wc, src, &len);
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
    tnode * node = (tnode *)calloc(1, sizeof(tnode));
    if (!node) abort();
    node->refcount = 1;
    return node;
}

void addtoken(tnode ** root, const char *str, variant id)
{
    tnode * tk;
    static const struct replace {
        wchar_t wc;
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
        int index, i = 0;
        wchar_t ucs, lcs;
        size_t len;

        utf8_decode(&ucs, str, &len);
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
        while (next && next->wc != ucs)
            next = next->nexthash;
        if (!next) {
            tref *ref;
            tnode *node = mknode(); /* TODO: what is the reason for this empty node to exist? */

            if (ucs < 'a' || ucs > 'z') {
                lcs = towlower((wchar_t)ucs);
            }
            if (ucs == lcs) {
                ucs = towupper((wchar_t)ucs);
            }

            ref = (tref *)malloc(sizeof(tref));
            if (!ref) abort();
            ref->wc = ucs;
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
                assert(ref);
                ref->wc = lcs;
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
            if (lcs == replace[i].wc) {
                char zText[1024];
                memcpy(zText, replace[i].str, 3);
                str_strlcpy(zText + 2, (const char *)str + len, sizeof(zText)-2);
                addtoken(root, zText, id);
                break;
            }
            ++i;
        }
    }
}

void freetokens(tnode * node)
{
    int i;
    for (i = 0; i != NODEHASHSIZE; ++i) {
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
    /* TODO: warning C6011: Dereferencing NULL pointer 'node'. */
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
        wchar_t wc;
        size_t len;
        int ret = utf8_decode(&wc, str, &len);

        if (ret != 0) {
            /* encoding is broken. youch */
            log_error("findtoken | encoding error in '%s'\n", key);
            return E_TOK_NOMATCH;
        }
#if NODEHASHSIZE == 8
        index = wc & 7;
#else
        index = wc % NODEHASHSIZE;
#endif
        ref = tk->next[index];
        while (ref && ref->wc != wc)
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
